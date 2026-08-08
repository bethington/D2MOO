'use strict';
/* Game.exe behavioural tracer -- the CONF_TRACE evidence source.
 *
 * WHY THIS EXISTS. Game.exe's functions run ONCE at startup, and its error
 * paths (bad registry, missing ini, absent renderer DLL) may run never. No
 * amount of live observation accumulates the way CONF_BATTLETESTED assumes,
 * and only 4 of its 18 launcher functions can be proven by byte identity.
 * The other 14 are verified by making the rare paths run and diffing every
 * observable side effect between the original binary and ours.
 *
 * WHAT COUNTS AS OBSERVABLE. Game.exe reaches the outside world through its
 * IMPORT TABLE -- registry, files, LoadLibrary, CreateProcess, windows. That
 * is a small enumerable set (35 thunks), so the import table IS the
 * observable surface and hooking it is complete rather than best-effort.
 *
 * WHY FRIDA AND NOT AN INJECTED DETOURS DLL. Same mechanism -- Frida's
 * Interceptor overwrites a prologue and trampolines, exactly as DetourAttach
 * does -- but this repo already proved PD2 tolerates it
 * (conformance/behavioral/pd2_phase0_hook_spike.js) and already drives it
 * from Python. The external-debugger route is a known dead end here: PD2's
 * anti-debug INT3 flood beat dbgeng (see pd2_frida_capture.py's header).
 *
 * STOP POINT. Game.exe's whole job is to pick a module, LoadLibrary it,
 * GetProcAddress("QueryInterface") and hand off. Everything after that call
 * belongs to D2Client/D2Launch, so tracing past it would diff code we are
 * not reimplementing. We stop when the handoff is resolved.
 */

const CONFIG = {
    // Overridden from the driver via rpc.
    stopOnHandoff: true,
    maxEvents: 20000,
    // Game.exe applies a DENY-ALL DACL to its OWN process early in startup
    // (ApplyProcessSecurityRestrictions @ 0x408120: AllocateAndInitializeSid,
    // InitializeAcl, AddAccessDeniedAce with mask 0xF01FFFFE, then
    // SetSecurityInfo on its own handle). After that the process cannot be
    // killed or even queried by a normal same-user caller -- measured: a
    // traced run left an orphan that Stop-Process refused with "Access is
    // denied" and whose ExecutablePath read back empty.
    //
    // We still LOG the call, because making it is part of the behaviour
    // being verified -- we only stop it taking EFFECT, by zeroing the
    // SecurityInfo mask so the call applies nothing and returns success.
    // Both the original and our build are traced identically, so the
    // comparison stays fair.
    neutraliseSelfProtection: true,
    // Diablo II allows one instance at a time: Fog.dll creates a NAMED EVENT
    // (CreateEventA) and treats ERROR_ALREADY_EXISTS as "already running",
    // which is the "Only one copy of Diablo II may run at a time" dialog.
    // A traced run therefore dies early whenever a real game is open --
    // measured: the trace stopped after 244 events having never reached the
    // handoff, with a modal dialog left on screen.
    //
    // Rather than ask anyone to close their game, give the traced process
    // its own PRIVATE event namespace by suffixing every named event it
    // creates. Deliberately chosen over the two alternatives: clearing
    // ERROR_ALREADY_EXISTS would leave our process sharing the live game's
    // event object, and patching Fog would change the binary under test.
    // Suffixing touches nothing outside the traced process. The ORIGINAL
    // name is logged, so the check remains visible as behaviour.
    isolateNamedEvents: true,
    eventSuffix: '#gameexe-trace',
};

let seq = 0;
let stopped = false;
let neutralised = 0;
const isolatedEvents = [];
const eventNameKeepAlive = [];

/* ---- argument rendering -------------------------------------------------
 * Deliberately schema-free. Rather than maintain a signature table for every
 * Win32 import (which would silently under-report the moment a new one
 * appears), render each argument three ways and let the differ decide:
 * raw value, an ASCII string when the pointer is readable, and a
 * well-known-constant name when one applies. Unreadable pointers render as
 * their raw value -- never as an error, never dropped.
 */
const WELL_KNOWN = {
    '0x80000000': 'HKEY_CLASSES_ROOT',
    '0x80000001': 'HKEY_CURRENT_USER',
    '0x80000002': 'HKEY_LOCAL_MACHINE',
    '0x80000003': 'HKEY_USERS',
};

function renderArg(p) {
    const raw = p.toString();
    const out = { raw: raw };
    if (WELL_KNOWN[raw]) {
        out.konst = WELL_KNOWN[raw];
        return out;
    }
    // Small values are almost never pointers; do not probe them.
    const v = parseInt(raw, 16);
    if (v > 0x10000) {
        // ANSI, not UTF-8. Game.exe is an ANSI-API binary throughout
        // (RegOpenKeyA, GetPrivateProfileIntA, LoadLibraryA); readUtf8String
        // rejects any byte >= 0x80 and returned nothing for the registry
        // paths and filenames that are the whole point of the trace.
        let s = null;
        try { s = p.readAnsiString(260); } catch (e) { s = null; }
        if (s === null) {
            try { s = p.readUtf8String(260); } catch (e) { s = null; }
        }
        // Keep plausible text only: printable, more than one character.
        // Strings are the evidence a wrong registry key or filename shows up
        // in, so the filter stays permissive -- it excludes binary noise, not
        // punctuation.
        if (s && s.length > 1 && /^[\x20-\x7e]+$/.test(s)) {
            out.str = s;
        }
    }
    return out;
}

function emit(ev) {
    if (stopped) return;
    if (seq >= CONFIG.maxEvents) {
        if (seq === CONFIG.maxEvents) {
            send({ type: 'truncated', at: seq });
            seq++;
        }
        return;
    }
    ev.seq = seq++;
    send(ev);
}

/* ---- export resolution --------------------------------------------------
 * Frida 17 REMOVED the static `Module.getExportByName(module, name)` -- it
 * throws a bare "TypeError: not a function", which is how all 90 hooks came
 * back "unresolved" on the first attempt, including KERNEL32!GetTickCount.
 * Probed rather than guessed (a third guess would have been careless):
 *
 *     Module.getExportByName(mod, name)     ERR not a function
 *     Module.getGlobalExportByName(name)    works
 *     module.getExportByName(name)          works   <- instance method
 *
 * A DLL that is not loaded yet cannot be resolved at all. That matters here:
 * Game.exe LoadLibrary's advapi32 itself, so at spawn time the security APIs
 * it uses for its self-protection DACL do not exist to hook. Force the
 * library in rather than silently skipping it.
 */
function resolveExport(dll, name) {
    try {
        if (!dll) return Module.getGlobalExportByName(name);
        let m = Process.findModuleByName(dll);
        if (!m) {
            try { Module.load(dll); } catch (e) { /* genuinely absent */ }
            m = Process.findModuleByName(dll);
        }
        if (m) return m.getExportByName(name);
        return Module.getGlobalExportByName(name);
    } catch (e) {
        return null;
    }
}

/* ---- hook installation -------------------------------------------------- */

function install() {
    // Frida 17 moved import enumeration onto the Module INSTANCE
    // (`Module.enumerateImports(name)` is gone and dies with a bare
    // "TypeError: not a function"), and its records carry only
    // {type, name, module, slot} -- NO `address`, and the `slot` values
    // repeat, so neither field can be attached to directly.
    //
    // So resolve each API by NAME in its owning DLL and hook the real
    // implementation, then keep only calls whose RETURN ADDRESS lies inside
    // Game.exe. That is not a workaround, it is a better instrument: it also
    // catches calls made through GetProcAddress-resolved pointers, which
    // never touch the IAT and which an import hook would silently miss --
    // and this launcher resolves and calls exactly such a pointer at its
    // handoff.
    const main = Process.mainModule || Process.enumerateModules()[0];
    const lo = main.base;
    const hi = main.base.add(main.size);
    const imports = (typeof main.enumerateImports === 'function')
        ? main.enumerateImports()
        : Module.enumerateImports(main.name);

    let hooked = 0, failed = 0, skipped = 0;
    const failures = [];
    const seen = {};

    // APIs Game.exe reaches WITHOUT an import-table entry, by LoadLibrary +
    // GetProcAddress. They are invisible to import enumeration but are
    // absolutely part of the observable behaviour -- the self-protection
    // DACL is built entirely from this set. The return-address filter makes
    // hooking them safe: other modules' calls are still discarded.
    const EXTRA = [
        ['advapi32.dll', 'SetSecurityInfo'],
        ['advapi32.dll', 'AllocateAndInitializeSid'],
        ['advapi32.dll', 'InitializeAcl'],
        ['advapi32.dll', 'AddAccessDeniedAce'],
        ['advapi32.dll', 'FreeSid'],
    ];

    // The single-instance event is created by Fog.dll, NOT by Game.exe, so
    // it is invisible to the return-address filter and has to be handled on
    // its own. Hooked process-wide and deliberately un-filtered.
    if (CONFIG.isolateNamedEvents) {
        const ce = resolveExport('KERNEL32.dll', 'CreateEventA');
        if (ce) {
            Interceptor.attach(ce, {
                onEnter: function (args) {
                    const namePtr = args[3];
                    if (namePtr.isNull()) return;
                    let name = null;
                    try { name = namePtr.readAnsiString(200); } catch (e) { return; }
                    if (!name || name.indexOf(CONFIG.eventSuffix) >= 0) return;
                    const priv = name + CONFIG.eventSuffix;
                    // Keep a reference: a freed allocation would leave the
                    // callee reading released memory.
                    const buf = Memory.allocAnsiString(priv);
                    eventNameKeepAlive.push(buf);
                    args[3] = buf;
                    isolatedEvents.push(name);
                    emit({ type: 'event_isolated', original: name, used: priv });
                }
            });
            hooked++;
        } else {
            failures.push('KERNEL32.dll!CreateEventA: unresolved (event isolation OFF)');
        }
    }
    const targets = imports.map(function (i) { return i; });
    EXTRA.forEach(function (e) {
        targets.push({ type: 'function', module: e[0], name: e[1] });
    });

    targets.forEach(function (imp) {
        if (imp.type && imp.type !== 'function') { skipped++; return; }
        const label = imp.module ? imp.module + '!' + imp.name : imp.name;
        if (seen[label]) return;            // one hook per API, not per thunk
        seen[label] = true;

        const target = resolveExport(imp.module, imp.name);
        if (!target) { failed++; failures.push(label + ': unresolved'); return; }

        try {
            Interceptor.attach(target, {
                onEnter: function (args) {
                    // Only OUR module's calls. The CRT and the loaded D2 DLLs
                    // call these same APIs; including them would diff code we
                    // are not reimplementing.
                    const ra = this.returnAddress;
                    this.mine = ra.compare(lo) >= 0 && ra.compare(hi) < 0;
                    if (!this.mine) return;
                    this.label = label;
                    // Four arguments keeps the log readable and is enough to
                    // distinguish every launcher import that matters;
                    // RegEnumValueA's eight still differ in their first four.
                    this.args = [renderArg(args[0]), renderArg(args[1]),
                                 renderArg(args[2]), renderArg(args[3])];

                    // Record the request, then defuse it. args[2] of
                    // SetSecurityInfo is the SECURITY_INFORMATION mask;
                    // zeroing it makes the call apply nothing and return
                    // success, leaving the traced process killable.
                    if (CONFIG.neutraliseSelfProtection &&
                        /SetSecurityInfo/.test(label)) {
                        args[2] = ptr(0);
                        neutralised++;
                    }
                },
                onLeave: function (retval) {
                    if (!this.mine) return;
                    emit({ type: 'call', fn: this.label,
                           args: this.args, ret: retval.toString() });

                    // Handoff: Game.exe resolved the module's entry point.
                    // Its own behaviour ends here.
                    if (CONFIG.stopOnHandoff && /GetProcAddress/.test(this.label)) {
                        const a1 = this.args[1];
                        if (a1 && a1.str === 'QueryInterface') {
                            stopped = true;
                            send({ type: 'handoff', after: seq,
                                   neutralised: neutralised,
                                   isolatedEvents: isolatedEvents });
                        }
                    }
                }
            });
            hooked++;
        } catch (e) {
            failed++;
            failures.push(label + ': ' + e.message);
        }
    });

    // Report coverage explicitly. A tracer that silently hooks fewer imports
    // than it thinks would make two different binaries look identical for the
    // dullest possible reason.
    send({ type: 'ready', module: main.name, base: main.base.toString(),
           imports: imports.length, candidates: targets.length,
           unique: Object.keys(seen).length,
           hooked: hooked, failed: failed, skipped: skipped,
           failures: failures.slice(0, 20) });
}

rpc.exports = {
    start: function (cfg) {
        if (cfg) Object.keys(cfg).forEach(function (k) { CONFIG[k] = cfg[k]; });
        install();
        return true;
    },
    stopped: function () { return stopped; }
};
