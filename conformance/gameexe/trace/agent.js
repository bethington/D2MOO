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
};

let seq = 0;
let stopped = false;

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
        try {
            const s = p.readUtf8String(260);
            // Keep only plausible text: printable, non-empty, not a lone byte.
            if (s && s.length > 1 && /^[\x20-\x7e\\\/:.]+$/.test(s)) {
                out.str = s;
            }
        } catch (e) { /* unreadable: raw value stands on its own */ }
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

/* ---- hook installation -------------------------------------------------- */

function install() {
    // Frida 17 moved import enumeration onto the Module INSTANCE;
    // `Module.enumerateImports(name)` is gone and fails with a bare
    // "TypeError: not a function". Keep the old call as a fallback so this
    // agent works against whichever frida the venv happens to hold.
    const main = Process.mainModule || Process.enumerateModules()[0];
    const imports = (typeof main.enumerateImports === 'function')
        ? main.enumerateImports()
        : Module.enumerateImports(main.name);
    let hooked = 0, failed = 0;
    const failures = [];

    imports.forEach(function (imp) {
        if (!imp.address) return;
        const label = imp.module ? imp.module + '!' + imp.name : imp.name;
        try {
            Interceptor.attach(imp.address, {
                onEnter: function (args) {
                    this.label = label;
                    // Four arguments is enough for every launcher import that
                    // matters and keeps the log readable; RegEnumValueA's
                    // eight are still distinguished by their first four.
                    this.args = [renderArg(args[0]), renderArg(args[1]),
                                 renderArg(args[2]), renderArg(args[3])];
                },
                onLeave: function (retval) {
                    emit({ type: 'call', fn: this.label,
                           args: this.args, ret: retval.toString() });

                    // Handoff: Game.exe resolved the module's entry point.
                    // Its own behaviour ends here.
                    if (CONFIG.stopOnHandoff && /GetProcAddress/.test(this.label)) {
                        const a1 = this.args[1];
                        if (a1 && a1.str === 'QueryInterface') {
                            stopped = true;
                            send({ type: 'handoff', after: seq });
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
           imports: imports.length, hooked: hooked, failed: failed,
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
