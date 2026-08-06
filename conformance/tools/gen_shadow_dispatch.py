#!/usr/bin/env python3
"""Generate D2Common_ShadowDispatch.gen.h from shadow_manifest.json.

Full-D2Common 1:1 shadow-conformance (conformance/D2COMMON_FULL_SHADOW_PLAN.md):
each manifest entry becomes a live shadow dispatcher (atomic mode + Detours
trampoline slot + swappable reimpl ptr + typed Thunk) hooked by VERIFIED OFFSET.
Emits the dispatcher namespaces, the LiveDispatchGen bridge table + accessors
(the coord header's C exports delegate here), and Install(ctx) -- the
ApplyPatchAction calls placed in DllPreLoadHook.

Emits:
  CLASS A -- return-value integer (<=32-bit), comparison = masked return value;
            reimpl pure or read-only-through-pointer.
  CLASS B -- void with an out-param buffer, comparison = the written bytes. The
            reimpl runs on an INDEPENDENT copy of the input buffer, so the game's
            own buffer only ever receives the ORIGINAL's write (no double-mutation).
Other classes (C mutation of live objects / D register-explicit / E u64) are
future work; see conformance/D2COMMON_FULL_SHADOW_PLAN.md.

Arg schema: a string ("i32"/"ptr") is a 4-byte scalar; an object
{"kind":"outbuf","bytes":N} is a class-B out-param buffer.

D2Client (and any future module with no hand-written coord family) is a second
config in MODULES below: same manifest schema, standalone=True so the header
emits its own D2MOO_LiveDispatch_* bridge exports (D2Common's coord header
already owns that for D2Common).

Usage:  python gen_shadow_dispatch.py
"""
import json
import os

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))

CC = {"fastcall": "__fastcall", "stdcall": "__stdcall", "cdecl": "__cdecl"}

# One config per patch module. `base`/`program` feed validate_ret_bits (which
# queries Ghidra for the REAL return type at each offset -- the check that
# would have caught the 8 ret_bits bugs found 2026-07-29 before they ever
# produced a false divergence).
MODULES = [
    {
        "program": "D2Common.dll",
        "base": 0x6FD50000,
        "manifest": os.path.join(ROOT, "conformance", "shadow_manifest.json"),
        "out": os.path.join(ROOT, "D2.Detours.patches", "1.13c",
                            "D2Common_ShadowDispatch.gen.h"),
        "standalone": False,   # LiveDispatch_CoordFamily.h owns the bridge exports
    },
    {
        "program": "D2Client.dll",
        "base": 0x6FAB0000,
        "manifest": os.path.join(ROOT, "conformance", "shadow_manifest.D2Client.json"),
        "out": os.path.join(ROOT, "D2.Detours.patches", "1.13c",
                            "D2Client_ShadowDispatch.gen.h"),
        "standalone": True,    # no coord family -- this header owns its own exports
    },
    {
        # Conformance-lab subject. `program` is the GHIDRA program (the validators
        # query it); `module` is what the game actually LOADS and is what lands in
        # kModuleName, dispatcher records and battletest_promoter's IMAGE_BASES.
        # They differ here because the stale 2023 upstream build already occupies
        # the program name SGD2FreeRes.dll in that project and switch_program
        # matches by NAME -- so the fork was imported as SGD2FreeRes-GDI.dll.
        # For D2's own DLLs the two coincide and `module` may be omitted.
        "program": "SGD2FreeRes-GDI.dll",
        "module": "SGD2FreeRes.dll",
        "base": 0x10000000,
        "manifest": os.path.join(ROOT, "conformance", "shadow_manifest.SGD2FreeRes.json"),
        "out": os.path.join(ROOT, "D2.Detours.patches", "1.13c",
                            "SGD2FreeRes_ShadowDispatch.gen.h"),
        "standalone": True,    # no coord family -- this header owns its own exports
    },
]

# Back-compat: existing callers (validate_ret_bits' own default, ad-hoc scripts)
# expect a single MANIFEST/OUT/_D2COMMON_BASE -- keep them pointed at D2Common.
MANIFEST = MODULES[0]["manifest"]
OUT = MODULES[0]["out"]


def ns_of(name):
    return name + "Dispatch"


def norm_args(args):
    """Normalize each arg to {kind, bytes}. String -> 4-byte scalar."""
    out = []
    for a in args:
        if isinstance(a, str):
            out.append({"kind": a, "bytes": 4})
        else:
            out.append({"kind": a["kind"], "bytes": int(a.get("bytes", 4))})
    return out



# Ghidra return type -> real width. A sub-dword return leaves STALE GARBAGE in
# the upper EAX bits (the callee only writes AL/AX), so comparing 32 bits
# produces FALSE divergences -- which is exactly what refuted
# ITEMS_GetItemDataByte45 on 2026-07-29 (orig 0x0E32__05 vs reimpl 0x00000005;
# identical in the low byte that actually IS the return value). 8 of 97 entries
# were wrong. Validate at generation time so it cannot recur silently.
_RET_WIDTH = {"byte": 8, "undefined1": 8, "bool": 8, "char": 8, "uchar": 8,
              "word": 16, "undefined2": 16, "ushort": 16, "short": 16}
_D2COMMON_BASE = 0x6FD50000

import re as _re

# Word boundaries are spelled out rather than using the \b escape: a \b in the
# generating script collapsed to a literal BACKSPACE byte (0x08), so every
# pattern matched a character that never appears and the guard silently
# validated nothing while reporting success. Anchoring on the operand
# separator is unambiguous and survives any quoting.
_RE_RET = _re.compile(r"^RET(\s|$)", _re.I)
# Same instruction, but capturing the callee-cleans byte count (`RET 0x8`).
_RE_RET_N = _re.compile(r"^RET\s*(0x[0-9a-f]+|\d+)?\s*$", _re.I)
_RE_JMP = _re.compile(r"^JMP(\s|$)", _re.I)
_RE_W8 = _re.compile(r"^(MOV|XOR|OR|AND|ADD|SUB|SETN?[A-Z]{1,2})\s+AL(\s*,|\s*$)", _re.I)
_RE_W16 = _re.compile(r"^(MOV|XOR|OR|AND|ADD|SUB)\s+AX(\s*,|\s*$)", _re.I)
_RE_W32 = _re.compile(r"^(MOV|MOVZX|MOVSX|XOR|OR|AND|ADD|SUB|SBB|NEG|IMUL|LEA|POP|INC|DEC|SHL|SHR|SAR|NOT|CDQ)\s+EAX(\s*,|\s*$)", _re.I)


def validate_unique_offsets(entries):
    """Reject two entries hooking the SAME offset.

    Detours installs one hook per address: the first ApplyPatchAction wins and
    the duplicate silently fails, leaving a null trampoline and a dispatcher
    that reports 0 hits FOREVER while looking perfectly healthy. Found live
    2026-07-29 -- ITEMS_GetItemDataField34/Byte44 were stale aliases for
    ITEMS_GetItemDataRareSuffix/BodyLoc at the same offsets, and only the new
    `hooked` field in /dispatchers revealed it. Cheap to check, invisible
    otherwise.
    """
    seen = {}
    dupes = []
    for e in entries:
        off = e["offset"] if isinstance(e["offset"], int) else int(str(e["offset"]), 0)
        if off in seen:
            dupes.append((off, seen[off], e["name"]))
        else:
            seen[off] = e["name"]
    for off, first, second in dupes:
        print(f"  !! DUPLICATE HOOK OFFSET 0x{off:x}: {first!r} and {second!r} -- Detours "
              f"hooks the first only; the second gets a null trampoline and 0 hits forever")
    return dupes


_RE_MOVZX_SRC = _re.compile(r"^MOVZX\s+EAX,\s*(byte|word)\s+ptr\b", _re.I)

# A reimpl declared `unsigned short` returns in AX; the upper half of EAX is
# unspecified by the ABI. Comparing it compares register residue.
_RE_REIMPL_DECL = _re.compile(
    r'extern\s+"C"\s+(?:__declspec\([^)]*\)\s*)?'
    r'(unsigned\s+char|signed\s+char|char|unsigned\s+short|short|uint8_t|int8_t|'
    r'uint16_t|int16_t|bool|BYTE|WORD)\s+__(?:stdcall|fastcall|cdecl)\s+(\w+)\s*\(',
    _re.I)
_REIMPL_WIDTH = {
    "unsigned char": 8, "signed char": 8, "char": 8, "uint8_t": 8, "int8_t": 8,
    "bool": 8, "byte": 8,
    "unsigned short": 16, "short": 16, "uint16_t": 16, "int16_t": 16, "word": 16,
}


def _reimpl_return_widths():
    """name -> declared sub-dword return width, from the reimpl sources."""
    out = {}
    cdir = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                        "reimpl_provider", "candidates")
    if not os.path.isdir(cdir):
        return out
    for fn in os.listdir(cdir):
        if not fn.endswith(".cpp"):
            continue
        try:
            with open(os.path.join(cdir, fn), "r", encoding="utf-8", errors="replace") as f:
                txt = f.read()
        except OSError:
            continue
        for m in _RE_REIMPL_DECL.finditer(txt):
            out[m.group(2)] = _REIMPL_WIDTH[m.group(1).lower()]
    return out


def _observed_ret_width(ins):
    """Narrowest return-register DATUM width across all return paths.

    DATUM width, not write width -- for a zero-extending load the two differ.
    `MOVZX EAX, word ptr [EAX+6]` writes 32 bits, but 16 of them are zeros the
    instruction manufactured, so masking to 16 discards nothing the original
    computed while tolerating the reimpl's ABI-permitted upper-half residue.

    Treating MOVZX as a 32-bit datum is what over-widened 19 entries on
    2026-07-29; the hottest of them, DATATBLS_GetLevelRecordBitfield06, logged
    166,565 false divergences inside a day, every one agreeing perfectly in the
    low 16 bits. MOVSX is deliberately not narrowed: sign extension makes the
    upper bits carry information.

    AUTHORITY IS THE DISASSEMBLY, not Ghidra's declared type. This replaced a
    type-based heuristic that was wrong in both directions:

      * It MISSED PATH_GetDirection -- Ghidra declares `uint`, but the body is
        `MOV EAX,[ESP+4]; MOV AL,[EAX+0x65]`, so the upper 24 bits returned are
        leftover POINTER bytes. Declared 32 would have diverged on every call.
      * It FALSE-POSITIVED on ~20 entries -- Ghidra declares `byte`/`ushort`,
        but the body is `MOVZX EAX, byte ptr [...]`, which writes the FULL
        register zero-extended, so 32 is correct and safe.

    A declared C type says what the value MEANS; only the instruction says how
    many bits were actually written, and that is what the mask must match.
    """
    widths = []
    for idx, cur in enumerate(ins):
        if not _RE_RET.match(cur.get("instruction", "")):
            continue
        for j in range(idx - 1, -1, -1):
            t = ins[j].get("instruction", "")
            if _RE_RET.match(t) or _RE_JMP.match(t):
                break
            mz = _RE_MOVZX_SRC.match(t)
            if mz:
                widths.append(8 if mz.group(1).lower() == "byte" else 16); break
            if _RE_W32.match(t):
                widths.append(32); break
            if _RE_W16.match(t):
                widths.append(16); break
            if _RE_W8.match(t):
                widths.append(8); break
    return min(widths) if widths else None


def validate_ret_bits(entries, base=_D2COMMON_BASE, program="D2Common.dll"):
    """Cross-check declared ret_bits against the ACTUAL return-write width.

    Best-effort: skipped when Ghidra is unreachable (the generator must still
    work offline), LOUD on every mismatch, and loud when it could not check.
    """
    try:
        import requests
    except ImportError:
        return []
    sess = requests.Session()
    reimpl_bits = _reimpl_return_widths()
    bad = []
    unreadable = 0
    for e in entries:
        if str(e.get("class", "A")).upper() == "B":
            continue            # void + out-param: ret_bits does not apply
        raw = e["offset"]
        off = raw if isinstance(raw, int) else int(str(raw), 0)
        addr = base + off
        try:
            ins = sess.get("http://127.0.0.1:8089/disassemble_function",
                           params={"address": f"0x{addr:x}", "program": program},
                           timeout=15).json().get("instructions") or []
        except Exception:
            return bad          # Ghidra down -> stop, don't spam
        if not ins:
            unreadable += 1
            continue
        obs = _observed_ret_width(ins)
        if obs is None:
            unreadable += 1
            continue
        # The comparison spans two implementations, so the mask has to be legal
        # for BOTH. Only the original was ever consulted; the reimpl's declared
        # return type is the other half of the contract.
        rb = reimpl_bits.get(e["name"])
        eff = obs if rb is None else min(obs, rb)
        declared = int(e.get("ret_bits", 32))
        if eff != declared:
            bad.append((e["name"], obs, declared, eff))
    if unreadable:
        print(f"  !! RET-WIDTH CHECK INCOMPLETE: {unreadable}/{len(entries)} entries could "
              f"not be disassembled -- they were NOT validated")
    for name, _rt, declared, real in bad:
        direction = ("FALSE divergences guaranteed" if real < declared
                     else "masks the upper bits -- may HIDE a real divergence")
        print(f"  !! RET-WIDTH MISMATCH {name}: code writes {real} bits but manifest says "
              f"ret_bits={declared} -> {direction}")
    return bad


def _observed_cleanup(ins):
    """Bytes the callee pops on return, or None if undeterminable.

    Every return path in one function pops the same amount -- the convention is
    a property of the function, not the path -- so disagreement means the
    disassembly is being misread and we must not guess.
    """
    pops = set()
    for i in ins:
        m = _RE_RET_N.match(i.get("instruction", "").strip())
        if m:
            pops.add(int(m.group(1), 0) if m.group(1) else 0)
    return pops.pop() if len(pops) == 1 else None


def _expected_cleanup(callconv, argc):
    if callconv == "cdecl":
        return 0                          # caller cleans
    if callconv == "fastcall":
        return 4 * max(0, argc - 2)       # first two dwords in ECX/EDX
    return 4 * argc                       # stdcall: callee cleans


def validate_argc(entries, base=_D2COMMON_BASE, program="D2Common.dll"):
    """Cross-check declared arg count against the callee's ACTUAL stack cleanup.

    THIS ONE IS FATAL, unlike the ret-width and duplicate-offset checks.

    A wrong ret_bits produces false divergences: noisy and misleading, but the
    game keeps running and the evidence is recoverable. A wrong arg count on a
    callee-cleans convention produces a CRASHED PROCESS. The generated thunk
    emits its own `RET n`; if n disagrees with what the caller pushed, ESP is
    permanently skewed for the rest of that call chain and the next return goes
    to whatever now sits under the stack pointer.

    That is not hypothetical -- it is the 2026-07-30 outage. Two of thirty
    harvested entries carried an argc taken from Ghidra's inferred signature:
    INV_CanItemFitInStoragePage declared 3 args against a real `RET 8`, and
    ITEMS_TestItemFlags declared 2 against a real `RET 0x10`. Both are on the
    inventory path, so the game access-violated the instant a save's items were
    populated, with `eip=0x00000140` -- a small nonsense value in no loaded
    module, the signature of a return through a skewed stack. Shipping a build
    that cannot load a save is strictly worse than shipping no build, so this
    refuses to generate rather than warn.

    Authority is the disassembly. Ghidra's parameter list on an undocumented
    leaf is inference; `RET 0x8` is what the compiler emitted.
    """
    try:
        import requests
    except ImportError:
        return []
    sess = requests.Session()
    bad, unreadable = [], 0
    for e in entries:
        raw = e["offset"]
        off = raw if isinstance(raw, int) else int(str(raw), 0)
        try:
            ins = sess.get("http://127.0.0.1:8089/disassemble_function",
                           params={"address": f"0x{base + off:x}", "program": program},
                           timeout=15).json().get("instructions") or []
        except Exception:
            return bad          # Ghidra down -> stop, don't spam
        if not ins:
            unreadable += 1
            continue
        obs = _observed_cleanup(ins)
        if obs is None:
            unreadable += 1
            continue
        argc = len(e.get("args", []))
        cc = e.get("callconv", "stdcall")
        exp = _expected_cleanup(cc, argc)
        if obs != exp:
            bad.append((e["name"], off, cc, argc, exp, obs))
    if unreadable:
        print(f"  !! ARGC CHECK INCOMPLETE: {unreadable}/{len(entries)} entries could not "
              f"be disassembled -- they were NOT validated")
    for name, off, cc, argc, exp, obs in bad:
        print(f"  !! ARGC MISMATCH {name} (0x{off:x}, {cc}): manifest declares {argc} arg(s) "
              f"so the thunk pops {exp}, but the callee's RET pops {obs} (real argc "
              f"{obs // 4 if cc != 'fastcall' else obs // 4 + 2}) -> ESP skew on every call")
    return bad


def emit_dispatcher(e, idx):
    cls = e.get("class", "A").upper()
    if cls == "B":
        return emit_dispatcher_b(e)
    if cls == "D":
        return emit_dispatcher_d(e, idx)
    return emit_dispatcher_a(e)


# A dispatcher's mode is process-local state that resets to Original on every
# launch, so a function whose ONLY calls happen during startup can never be
# observed: by the time anything can POST /dispatcher/N/mode, its calls are
# already over. MEASURED on Sgd2fr_D2Client_SetTileCullingBound (SGD2FreeRes), which fires
# exactly twice at startup and never again -- hits stayed at 2 through a world
# load and 46,000 frames. An entry may therefore declare the mode it STARTS in.
# Opt-in per entry, never a global default: shadow mode runs the reimpl on every
# real call, and turning that on corpus-wide at startup would change the
# startup path of every dispatched function at once.
_STARTUP_MODES = {"original": "Original", "shadow": "Shadow", "reimpl": "Reimpl"}


def startup_mode_for(e):
    raw = str(e.get("startup_mode", "original")).strip().lower()
    if raw not in _STARTUP_MODES:
        raise SystemExit(
            "[gen_shadow_dispatch] REFUSING: %s has startup_mode=%r; expected one of %s"
            % (e.get("name", "<unnamed>"), raw, sorted(_STARTUP_MODES)))
    return _STARTUP_MODES[raw]


def emit_dispatcher_d(e, idx):
    # NOTE: class D thunks are __declspec(naked) raw assembly, so they do NOT
    # call LiveDispatchGen::EnsureArmed() the way classes A and B do -- injecting
    # a C++ call into a naked thunk means hand-writing the save/restore around it,
    # and getting that wrong corrupts the register-explicit ABI this class exists
    # to preserve. Consequence, recorded rather than hidden: a class-D dispatcher
    # is not lazily armed, so a class-D function that only fires during STARTUP
    # still cannot be compared. Classes A and B cover every entry that has needed
    # it so far.
    """Class D: register-explicit ABI (v1: single arg in EAX, u32/pointer return,
    no stack args, plain RET -- the DATATBLS_* accessor pattern). The game enters
    the hooked original with the arg in EAX; a NAKED stub can't be a normal typed
    thunk, so it captures EAX + this entry's index and tail-calls the shared C
    dispatcher LiveDispatchGen_RegDispatch, which runs original (via a set-EAX
    inline-asm trampoline call) + reimpl (SEH-guarded fastcall) and compares."""
    name = e["name"]
    ns = ns_of(name)
    startup_mode = startup_mode_for(e)
    return f"""// {name} -- class D (register-explicit: arg in EAX, u32/ptr ret) -- off 0x{e['offset']:x}, entry #{idx}
namespace {ns} {{
	static std::atomic<int32_t> mode{{ (int32_t)LiveDispatchGen::Mode::{startup_mode} }};
	static void* trampoline = nullptr;
	static uint64_t hits = 0, divergences = 0;
	static LiveDispatchGen::DistinctSampler distinct;   // distinct arg tuples seen in shadow
	static void* reimpl = nullptr;   // bound from the provider DLL BY NAME (D2Debugger)
	// NAKED stub: game calls the original with the arg in EAX, no stack args, RET 0.
	// Push (this entry index, EAX) to the shared C dispatcher; its u32 return comes
	// back in EAX; clean the 2 cdecl args; RET to the game's caller.
	__declspec(naked) void Thunk() {{
		__asm {{
			push eax
			push {idx}
			call LiveDispatchGen_RegDispatch
			add esp, 8
			ret
		}}
	}}
}}
"""


def emit_dispatcher_b(e):
    """Class B: void out-param. Compare the written buffer; reimpl runs on a copy."""
    name = e["name"]
    ns = ns_of(name)
    cc = CC[e["callconv"]]
    args = norm_args(e["args"])
    argc = len(args)
    outs = [i for i, a in enumerate(args) if a["kind"] == "outbuf"]
    if len(outs) != 1:
        raise ValueError(f"{name}: class B needs exactly one outbuf arg, got {len(outs)}")
    oi = outs[0]
    nbytes = args[oi]["bytes"]
    params = ", ".join(f"uint32_t a{i}" for i in range(argc))
    fnptr_args = ", ".join(["uint32_t"] * argc) if argc else ""
    orig_call = ", ".join(f"a{i}" for i in range(argc))
    # reimpl call: substitute the outbuf arg with a pointer to the local copy.
    reimpl_call = ", ".join(
        (f"(uint32_t)(uintptr_t)local" if i == oi else f"a{i}") for i in range(argc))
    logargs = ", ".join(f"a{i}" for i in range(argc))
    call_expr = f"((void({cc}*)({fnptr_args}))fn)({', '.join(f'a{i}' for i in range(argc))})"
    startup_mode = startup_mode_for(e)
    return f"""// {name} -- class B (void out-param, {e['callconv']}, out arg a{oi} = {nbytes} bytes) -- off 0x{e['offset']:x}
namespace {ns} {{
	static std::atomic<int32_t> mode{{ (int32_t)LiveDispatchGen::Mode::{startup_mode} }};
	static void* trampoline = nullptr;
	static uint64_t hits = 0, divergences = 0;
	static LiveDispatchGen::DistinctSampler distinct;   // distinct arg tuples seen in shadow
	static void* reimpl = nullptr;   // bound from the provider DLL BY NAME (D2Debugger)
	// SEH-isolated reimpl call (POD-only + __try/__except -> no C2712). A faulting
	// reimpl is caught here so a buggy reimpl can never crash the game.
	static void SafeReimpl(void* fn, {params}, int* faulted) {{
		__try {{ {call_expr}; }}
		__except (EXCEPTION_EXECUTE_HANDLER) {{ *faulted = 1; }}
	}}
	static void {cc} Thunk({params}) {{
		LiveDispatchGen::EnsureArmed();
		++hits;
		using Fn = void({cc}*)({fnptr_args});
		const Fn orig = (Fn)trampoline;
		void* rfn = reimpl;
		const LiveDispatchGen::Mode m = LiveDispatchGen::tl_inDispatch
			? LiveDispatchGen::Mode::Original
			: (LiveDispatchGen::Mode)mode.load(std::memory_order_relaxed);
		if (m == LiveDispatchGen::Mode::Reimpl && rfn) {{
			int f = 0;
			LiveDispatchGen::tl_inDispatch = true; ++LiveDispatchGen::g_inFlight;
			SafeReimpl(rfn, {orig_call}, &f);
			--LiveDispatchGen::g_inFlight; LiveDispatchGen::tl_inDispatch = false;
			if (f) {{ ++divergences; LiveDispatchGen::LogFault("{name}"); if (orig) orig({orig_call}); }}
			return;
		}}
		if (m != LiveDispatchGen::Mode::Shadow || !orig || !rfn) {{ if (orig) orig({orig_call}); return; }}
		// Shadow: snapshot the out-buffer's INPUT bytes, let ORIGINAL write the
		// game's buffer (it wins), then run reimpl on an INDEPENDENT copy of the
		// input and compare -- the game's memory is never double-mutated.
		unsigned char inbuf[{nbytes}], origOut[{nbytes}], local[{nbytes}];
		memcpy(inbuf, (const void*)(uintptr_t)a{oi}, {nbytes});
		orig({orig_call});
		memcpy(origOut, (const void*)(uintptr_t)a{oi}, {nbytes});
		memcpy(local, inbuf, {nbytes});
		int f = 0;
		LiveDispatchGen::tl_inDispatch = true; ++LiveDispatchGen::g_inFlight;
		SafeReimpl(rfn, {reimpl_call}, &f);
		--LiveDispatchGen::g_inFlight; LiveDispatchGen::tl_inDispatch = false;
		const uint32_t av[] = {{ {logargs} }};
		LiveDispatchGen::NoteInputs(distinct, av, {argc});
		if (f) {{ ++divergences; LiveDispatchGen::LogFault("{name}"); }}
		else if (memcmp(local, origOut, {nbytes}) != 0) {{
			++divergences;
			LiveDispatchGen::LogDivergenceBuf("{name}", av, {argc}, origOut, local, {nbytes});
		}}
		else {{ LiveDispatchGen::LogMatchBuf("{name}", av, {argc}, origOut, {nbytes}); }}
	}}
}}
"""


def emit_dispatcher_a(e):
    name = e["name"]
    ns = ns_of(name)
    cc = CC[e["callconv"]]
    argc = len(e["args"])
    retbits = int(e.get("ret_bits", 32))
    params = ", ".join(f"uint32_t a{i}" for i in range(argc))
    argnames = ", ".join(f"a{i}" for i in range(argc))
    fnptr_args = ", ".join(["uint32_t"] * argc) if argc else ""
    safe_params = (f"void* fn, {params}, int* faulted" if argc else "void* fn, int* faulted")
    safe_args = (f"rfn, {argnames}, &f" if argc else "rfn, &f")
    call_expr = f"((uint32_t({cc}*)({fnptr_args}))fn)({argnames})"
    if argc:
        av_decl = f"const uint32_t av[] = {{ {argnames} }};"
        av_ptr = "av"
        logdiv = f'LiveDispatchGen::LogDivergence("{name}", av, {argc}, ro, rr);'
    else:
        av_decl = ""
        av_ptr = "nullptr"
        logdiv = f'LiveDispatchGen::LogDivergence("{name}", nullptr, 0, ro, rr);'
    startup_mode = startup_mode_for(e)
    return f"""// {name} -- class A (return-value integer, {e['callconv']}, {argc} arg(s), ret {retbits}-bit) -- off 0x{e['offset']:x}
namespace {ns} {{
	static std::atomic<int32_t> mode{{ (int32_t)LiveDispatchGen::Mode::{startup_mode} }};
	static void* trampoline = nullptr;
	static uint64_t hits = 0, divergences = 0;
	static LiveDispatchGen::DistinctSampler distinct;   // distinct arg tuples seen in shadow
	static void* reimpl = nullptr;   // bound from the provider DLL BY NAME (D2Debugger)
	// SEH-isolated reimpl call: POD-only body + __try/__except (no C++ unwind object
	// -> no C2712). A faulting reimpl is CAUGHT here so a buggy reimpl can never crash
	// the game -- it degrades to a logged fault and the ORIGINAL's result is used.
	static uint32_t SafeReimpl({safe_params}) {{
		__try {{ return {call_expr}; }}
		__except (EXCEPTION_EXECUTE_HANDLER) {{ *faulted = 1; return 0u; }}
	}}
	static uint32_t {cc} Thunk({params}) {{
		LiveDispatchGen::EnsureArmed();
		++hits;
		using Fn = uint32_t({cc}*)({fnptr_args});
		const Fn orig = (Fn)trampoline;
		void* rfn = reimpl;
		const LiveDispatchGen::Mode m = LiveDispatchGen::tl_inDispatch
			? LiveDispatchGen::Mode::Original
			: (LiveDispatchGen::Mode)mode.load(std::memory_order_relaxed);
		if (m == LiveDispatchGen::Mode::Reimpl && rfn) {{
			int f = 0;
			LiveDispatchGen::tl_inDispatch = true; ++LiveDispatchGen::g_inFlight;
			uint32_t r = SafeReimpl({safe_args});
			--LiveDispatchGen::g_inFlight; LiveDispatchGen::tl_inDispatch = false;
			if (f) {{ ++divergences; LiveDispatchGen::LogFault("{name}"); return orig ? orig({argnames}) : 0u; }}
			return r;
		}}
		if (m != LiveDispatchGen::Mode::Shadow || !orig || !rfn) return orig ? orig({argnames}) : 0u;
		const uint32_t ro = orig({argnames});
		int f = 0;
		LiveDispatchGen::tl_inDispatch = true; ++LiveDispatchGen::g_inFlight;
		uint32_t rr = SafeReimpl({safe_args});
		--LiveDispatchGen::g_inFlight; LiveDispatchGen::tl_inDispatch = false;
		const uint32_t mask = LiveDispatchGen::RetMask({retbits});
		{av_decl}
		LiveDispatchGen::NoteInputs(distinct, {av_ptr}, {argc});
		if (f) {{ ++divergences; LiveDispatchGen::LogFault("{name}"); }}
		else if ((ro & mask) != (rr & mask)) {{
			++divergences;
			{logdiv}
		}}
		else {{ LiveDispatchGen::LogMatch("{name}", {av_ptr}, {argc}, ro & mask); }}
		return ro;
	}}
}}
"""


def emit(manifest, module_name="D2Common.dll", out_name="D2Common_ShadowDispatch.gen.h",
         standalone=False):
    """Emit one module's generated dispatcher header.

    `standalone` is the D2Client-vs-D2Common distinction: D2Common's dispatchers
    sit behind LiveDispatch_CoordFamily.h's hand-written coord family, which
    already owns the extern "C" D2MOO_LiveDispatch_* bridge exports and
    delegates indices past its own kCount into LiveDispatchGen. A module with NO
    coord family (D2Client, and eventually D2Game/Fog) has nothing to delegate
    FROM -- so when standalone=True, this header emits that export block
    itself, unindexed (index i maps straight to LiveDispatchGen::*, since there
    is no coord family taking the low indices)."""
    entries = manifest["entries"]
    parts = []
    # Dynamic header comment ONLY -- kept as its own small f-string, separate
    # from the huge plain-string C++ block below (which is full of literal
    # `{`/`}` braces that an f-string would try to interpret as format fields).
    parts.append(f"""#pragma once
// {out_name} -- GENERATED by conformance/tools/gen_shadow_dispatch.py
// from the {module_name} shadow manifest. DO NOT EDIT BY HAND; edit the manifest +
// regenerate. Full-D2Common 1:1 shadow-conformance (D2COMMON_FULL_SHADOW_PLAN.md).
//
// Include AFTER DetoursPatch.h (Install() uses HookContext/PatchAction) and after
// LiveDispatch_Generic.h. Single-TU home ({module_name.replace(".dll", "")}.patch.cpp),
// same model as the coord header.
""")
    parts.append("""#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include "LiveDispatch_Generic.h"

// One-time shared definitions (declared extern in LiveDispatch_Generic.h).
namespace LiveDispatchGen {
	thread_local bool tl_inDispatch = false;
	std::atomic<int> g_inFlight{ 0 };

	// --- lazy arming -------------------------------------------------------
	// The reimpl provider is loaded on demand (POST /reimpl/reload), which
	// happens long after launch. A function that only runs during STARTUP has
	// therefore already fired by the time anything can arm it, so it can never
	// be compared. Measured 2026-08-05: SGD2FreeRes's Sgd2fr_D2Client_SetTileCullingBound fires
	// exactly twice per process, both during init, and stayed at hits=2 across a
	// world load, 46,000 frames and every arming attempt of a long session.
	//
	// Arming from DllPreLoadHook would be the obvious fix and is the WRONG one:
	// that hook runs inside LoadLibrary, holding the loader lock, and
	// ReloadProvider loads a DLL and quiesces threads. That is the classic
	// deadlock, and it would hang the game at startup with no oracle left to
	// diagnose it.
	//
	// So arm on the FIRST DISPATCHED CALL instead. By then LoadLibrary has
	// returned and the lock is released, and for a startup-fired function the
	// first call is precisely the moment the provider needs to be bound. Cost
	// after the first call is one relaxed atomic load, alongside the ++hits and
	// mode.load() every thunk already does.
	//
	// D2Debugger owns the provider and is loaded well before any patch that
	// needs arming (measured: D2Debugger at module index 60, SGD2FreeRes at
	// 78/79), so resolving it by name here is safe. If it is absent we simply
	// stay unarmed -- never a fault, never a retry storm.
	std::atomic<bool> g_armAttempted{ false };
	void EnsureArmed()
	{
		bool expected = false;
		if (!g_armAttempted.compare_exchange_strong(expected, true,
				std::memory_order_acq_rel))
			return;                       // already tried exactly once
		if (HMODULE h = GetModuleHandleA("D2Debugger.dll"))
			if (auto fn = (void(__cdecl*)())GetProcAddress(h, "D2Dbg_EnsureProviderLoaded"))
				fn();
	}
""")
    # Provenance in the shared divergence/capture files -- D2Client and D2Common
    # each get their OWN copy of this whole namespace (separate DLLs, no ODR
    # issue), but both append to the SAME behavioral/*.jsonl files, so a bare
    # function name is no longer enough to tell which binary produced a record.
    parts.append(f'\tstatic const char* kModuleName = "{module_name}";\n')
    parts.append("""	static const char* kDivergencePath =
		"C:\\\\Users\\\\benam\\\\source\\\\cpp\\\\D2MOO\\\\conformance\\\\behavioral\\\\live_shadow_divergences.jsonl";
	static void WriteArgsJson(FILE* f, const uint32_t* args, int nargs) {
		fprintf(f, "\\"args\\":[");
		for (int i = 0; i < nargs; ++i) fprintf(f, "%s%u", i ? "," : "", args[i]);
		fprintf(f, "]");
	}
	// --- CONF_REGRESSION vector capture (opt-in) ----------------------------
	// Shadow mode already runs BOTH original and reimpl on every real call and
	// diffs them. On a MATCH that is a golden {real input -> real game output}
	// pair -- exactly an offline test vector. When the game is launched with
	// D2MOO_CAPTURE_VECTORS=1, matching calls record DISTINCT samples (first-seen,
	// capped per fn so a 100M-hit path costs a hash-set probe, not 100M lines) to
	// captured_vectors.jsonl. conformance/tools/capture_to_corpus.py folds those
	// into a CONF_REGRESSION corpus for the offline suite. Default OFF => zero
	// behavior/perf change for normal shadow proving.
	static const char* kCapturePath =
		"C:\\\\Users\\\\benam\\\\source\\\\cpp\\\\D2MOO\\\\conformance\\\\behavioral\\\\captured_vectors.jsonl";
	static bool CaptureEnabled() {
		static const bool on = [] {
			char b[8] = { 0 }; size_t n = 0;
			return getenv_s(&n, b, sizeof(b), "D2MOO_CAPTURE_VECTORS") == 0 && n > 0 && b[0] == '1';
		}();
		return on;
	}
	static uint64_t HashU32s(const uint32_t* a, int n) {
		uint64_t h = 1469598103934665603ull;   // FNV-1a
		for (int i = 0; i < n; ++i) { h ^= a[i]; h *= 1099511628211ull; }
		return h;
	}
	// First-seen distinct key per fn, capped -- a small golden set, not every hit.
	static bool CaptureNovel(const char* fn, uint64_t key) {
		static std::mutex mtx;
		static std::unordered_map<std::string, std::unordered_set<uint64_t>> seen;
		std::lock_guard<std::mutex> lk(mtx);
		auto& s = seen[fn];
		if (s.size() >= 64 || s.count(key)) return false;
		s.insert(key); return true;
	}
	// Return-value ABI (class A / class D): key on inputs, record ret.
	void LogMatch(const char* fn, const uint32_t* args, int nargs, uint32_t ret) {
		if (!CaptureEnabled() || nargs <= 0) return;   // no inputs => nothing to vary
		if (!CaptureNovel(fn, HashU32s(args, nargs))) return;
		FILE* f = nullptr;
		if (fopen_s(&f, kCapturePath, "a") == 0 && f) {
			fprintf(f, "{\\"fn\\":\\"%s\\",\\"module\\":\\"%s\\",", fn, kModuleName);
			WriteArgsJson(f, args, nargs);
			fprintf(f, ",\\"ret\\":%u,\\"src\\":\\"real\\",\\"note\\":\\"live SHADOW match captured vs PD2-S12\\"}\\n", ret);
			fclose(f);
		}
	}
	// Out-param ABI (class B): key on inputs + output bytes, record the out buffer.
	void LogMatchBuf(const char* fn, const uint32_t* args, int nargs,
		const unsigned char* out, int nbytes) {
		if (!CaptureEnabled()) return;
		uint64_t key = HashU32s(args, nargs);
		for (int i = 0; i < nbytes; ++i) { key ^= out[i]; key *= 1099511628211ull; }
		if (!CaptureNovel(fn, key)) return;
		FILE* f = nullptr;
		if (fopen_s(&f, kCapturePath, "a") == 0 && f) {
			fprintf(f, "{\\"fn\\":\\"%s\\",\\"module\\":\\"%s\\",", fn, kModuleName);
			WriteArgsJson(f, args, nargs);
			fprintf(f, ",\\"out\\":\\"");
			for (int i = 0; i < nbytes; ++i) fprintf(f, "%02x", out[i]);
			fprintf(f, "\\",\\"src\\":\\"real\\",\\"note\\":\\"live SHADOW match captured vs PD2-S12\\"}\\n");
			fclose(f);
		}
	}
	void LogDivergence(const char* fn, const uint32_t* args, int nargs, uint32_t o, uint32_t r) {
		FILE* f = nullptr;
		if (fopen_s(&f, kDivergencePath, "a") == 0 && f) {
			fprintf(f, "{\\"fn\\":\\"%s\\",\\"module\\":\\"%s\\",", fn, kModuleName);
			WriteArgsJson(f, args, nargs);
			fprintf(f, ",\\"orig_ret\\":%u,\\"reimpl_ret\\":%u,\\"note\\":\\"live SHADOW divergence vs PD2-S12\\"}\\n", o, r);
			fclose(f);
		}
		char buf[256];
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
			"[LiveDispatchGen] SHADOW DIVERGENCE %s: orig=%u reimpl=%u\\n", fn, o, r);
		OutputDebugStringA(buf);
	}
	void LogDivergenceBuf(const char* fn, const uint32_t* args, int nargs,
		const unsigned char* origOut, const unsigned char* reimplOut, int nbytes) {
		FILE* f = nullptr;
		if (fopen_s(&f, kDivergencePath, "a") == 0 && f) {
			fprintf(f, "{\\"fn\\":\\"%s\\",\\"module\\":\\"%s\\",", fn, kModuleName);
			WriteArgsJson(f, args, nargs);
			fprintf(f, ",\\"orig_out\\":\\"");
			for (int i = 0; i < nbytes; ++i) fprintf(f, "%02x", origOut[i]);
			fprintf(f, "\\",\\"reimpl_out\\":\\"");
			for (int i = 0; i < nbytes; ++i) fprintf(f, "%02x", reimplOut[i]);
			fprintf(f, "\\",\\"note\\":\\"live SHADOW out-param divergence vs PD2-S12\\"}\\n");
			fclose(f);
		}
		OutputDebugStringA("[LiveDispatchGen] SHADOW OUT-PARAM DIVERGENCE (see live_shadow_divergences.jsonl)\\n");
	}
	void LogFault(const char* fn) {
		FILE* f = nullptr;
		if (fopen_s(&f, kDivergencePath, "a") == 0 && f) {
			fprintf(f, "{\\"fn\\":\\"%s\\",\\"module\\":\\"%s\\",\\"fault\\":true,\\"note\\":\\"reimpl ACCESS VIOLATION caught by shadow-thunk SEH; original used, game safe\\"}\\n", fn, kModuleName);
			fclose(f);
		}
		char buf[192];
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
			"[LiveDispatchGen] REIMPL FAULT (caught) %s -- see live_shadow_divergences.jsonl\\n", fn);
		OutputDebugStringA(buf);
	}
}
""")

    has_d = any(e.get("class", "A").upper() == "D" for e in entries)
    if has_d:
        # Class-D shared runtime: the register-explicit trampoline call + SEH-guarded
        # reimpl call. The naked stubs (emitted per class-D entry) tail-call
        # LiveDispatchGen_RegDispatch, DEFINED after g_entries (it indexes them).
        parts.append("""// --- Class D (register-explicit, EAX-input) shared runtime ---
extern "C" unsigned int __cdecl LiveDispatchGen_RegDispatch(int idx, unsigned int inEax);
namespace LiveDispatchGen {
	// Call the ORIGINAL (Detours trampoline) with the arg in EAX, capture EAX. The
	// original takes no stack args and RET 0s, so a plain call after setting EAX is
	// the exact ABI. (x86 inline asm; the patch DLL is x86-only.)
	static unsigned int CallOrigEax(void* tramp, unsigned int inEax) {
		unsigned int r;
		__asm {
			mov eax, inEax
			mov edx, tramp
			call edx
			mov r, eax
		}
		return r;
	}
	// SEH-isolated reimpl call (POD-only body -> no C2712). A faulting reimpl is
	// caught so a buggy reimpl can never crash the game. Reimpl is a normal
	// __fastcall (arg in ECX) -- the oracle proved it in that convention.
	static unsigned int SafeReimplEax(void* fn, unsigned int inEax, int* faulted) {
		__try { return ((unsigned int(__fastcall*)(unsigned int))fn)(inEax); }
		__except (EXCEPTION_EXECUTE_HANDLER) { *faulted = 1; return 0u; }
	}
}
""")

    for i, e in enumerate(entries):
        parts.append(emit_dispatcher(e, i))

    # Bridge table + accessors.
    parts.append("namespace LiveDispatchGen {\n")
    if entries:
        parts.append("\tstatic GenEntry g_entries[] = {\n")
        for e in entries:
            ns = ns_of(e["name"])
            parts.append(
                f'\t\t{{ "{e["name"]}", 0x{e["offset"]:x}, &{ns}::mode, &{ns}::hits, '
                f'&{ns}::divergences, (void**)&{ns}::reimpl, &{ns}::trampoline, '
                f'&{ns}::distinct, {len(e.get("args", []))} }},\n')
        parts.append("\t};\n")
        parts.append("\tstatic const int kGenCount = (int)(sizeof(g_entries) / sizeof(g_entries[0]));\n")
    else:
        parts.append("\tstatic GenEntry* g_entries = nullptr;\n")
        parts.append("\tstatic const int kGenCount = 0;\n")
    parts.append("""	int Count() { return kGenCount; }
	const char* Name(int i) { return (i >= 0 && i < kGenCount) ? g_entries[i].name : ""; }
	uint32_t Offset(int i) { return (i >= 0 && i < kGenCount) ? g_entries[i].offset : 0xFFFFFFFFu; }
	int GetMode(int i) { return (i >= 0 && i < kGenCount) ? g_entries[i].mode->load(std::memory_order_relaxed) : 0; }
	void SetMode(int i, int m) { if (i >= 0 && i < kGenCount) g_entries[i].mode->store(m, std::memory_order_relaxed); }
	unsigned long long Hits(int i) { return (i >= 0 && i < kGenCount) ? *g_entries[i].hits : 0ull; }
	unsigned long long DistinctInputs(int i) { return (i >= 0 && i < kGenCount)
		? (unsigned long long)g_entries[i].distinct->count.load(std::memory_order_relaxed) : 0ull; }
	int ArgCount(int i) { return (i >= 0 && i < kGenCount) ? g_entries[i].argc : -1; }
	// which: 0 = arg0, 1 = arg1, 2 = the slot's hash (0 == slot unused).
	// The slot array is SPARSE -- hashes land wherever they probe to -- so a
	// caller must sweep all kSlots and use which==2 to tell occupied from
	// empty. Iterating 0..DistinctInputs()-1 would read unwritten slots.
	uint32_t SampleValue(int i, int slot, int which) {
		if (i < 0 || i >= kGenCount) return 0u;
		if (slot < 0 || (uint32_t)slot >= DistinctSampler::kSlots) return 0u;
		DistinctSampler& s = *g_entries[i].distinct;
		if (which == 2) return s.slots[slot].load(std::memory_order_acquire);
		if (which == 1) return s.v1[slot].load(std::memory_order_relaxed);
		return s.v0[slot].load(std::memory_order_relaxed);
	}
	int SampleSlotCount() { return (int)DistinctSampler::kSlots; }
	unsigned long long Divergences(int i) { return (i >= 0 && i < kGenCount) ? *g_entries[i].divergences : 0ull; }
	void* Trampoline(int i) { return (i >= 0 && i < kGenCount) ? *g_entries[i].trampolineSlot : nullptr; }
	void* Reimpl(int i) { return (i >= 0 && i < kGenCount) ? *g_entries[i].reimplSlot : nullptr; }
	void SetReimpl(int i, void* fn) { if (i >= 0 && i < kGenCount) *g_entries[i].reimplSlot = fn; }
	void QuiesceModes() { for (int i = 0; i < kGenCount; ++i) g_entries[i].mode->store(0, std::memory_order_seq_cst); }
	int InFlight() { return g_inFlight.load(std::memory_order_seq_cst); }
}
""")

    if has_d:
        # The Class-D C dispatcher: same shadow semantics as the typed thunks
        # (original wins; reimpl runs last SEH-guarded on the same input; original's
        # EAX is returned to the game), but reached from a naked stub by entry index.
        parts.append("""extern "C" unsigned int __cdecl LiveDispatchGen_RegDispatch(int idx, unsigned int inEax) {
	using namespace LiveDispatchGen;
	GenEntry& e = g_entries[idx];
	++(*e.hits);
	void* tramp = *e.trampolineSlot;
	void* rfn = *e.reimplSlot;
	const Mode m = tl_inDispatch ? Mode::Original
		: (Mode)e.mode->load(std::memory_order_relaxed);
	if (m == Mode::Reimpl && rfn) {
		int f = 0;
		tl_inDispatch = true; ++g_inFlight;
		unsigned int r = SafeReimplEax(rfn, inEax, &f);
		--g_inFlight; tl_inDispatch = false;
		if (f) { ++(*e.divergences); LogFault(e.name); return tramp ? CallOrigEax(tramp, inEax) : 0u; }
		return r;
	}
	if (m != Mode::Shadow || !tramp || !rfn) return tramp ? CallOrigEax(tramp, inEax) : 0u;
	const unsigned int ro = CallOrigEax(tramp, inEax);
	int f = 0;
	tl_inDispatch = true; ++g_inFlight;
	const unsigned int rr = SafeReimplEax(rfn, inEax, &f);
	--g_inFlight; tl_inDispatch = false;
	{ const uint32_t sv[] = { inEax }; NoteInputs(*e.distinct, sv, 1); }
	if (f) { ++(*e.divergences); LogFault(e.name); }
	else if (ro != rr) { ++(*e.divergences); const uint32_t av[] = { inEax }; LogDivergence(e.name, av, 1, ro, rr); }
	else { const uint32_t av[] = { inEax }; LogMatch(e.name, av, 1, ro); }
	return ro;
}
""")

    # Install -- ApplyPatchAction per entry (called from DllPreLoadHook).
    parts.append("namespace LiveDispatchGen {\n\tinline void Install(HookContext* ctx) {\n")
    for e in entries:
        ns = ns_of(e["name"])
        parts.append(
            f"\t\tctx->ApplyPatchAction(ctx, 0x{e['offset']:x}, (void*)&{ns}::Thunk, "
            f"PatchAction::FunctionReplaceOriginalByPatch, (void**)&{ns}::trampoline);\n")
    parts.append("\t}\n}\n")

    if standalone:
        # No coord family exists in this module to own the D2MOO_LiveDispatch_*
        # bridge exports (LiveDispatch_CoordFamily.h is D2Common-specific: it
        # owns indices < its own kCount and delegates the rest into
        # LiveDispatchGen). With no coord family, index i maps straight through
        # -- there is nothing to subtract.
        parts.append("""
// --- D2MOO_LiveDispatch_* bridge exports (standalone: no coord family) ---
// Same shape as LiveDispatch_CoordFamily.h's block, unindexed. D2Debugger's
// multi-bridge ResolveBridge() collects every module exporting GetCount into
// its own slice of the global dispatcher index space.
extern "C" {
	__declspec(dllexport) int __cdecl D2MOO_LiveDispatch_GetCount()
	{
		return LiveDispatchGen::Count();
	}
	__declspec(dllexport) const char* __cdecl D2MOO_LiveDispatch_GetName(int i)
	{
		return LiveDispatchGen::Name(i);
	}
	__declspec(dllexport) unsigned int __cdecl D2MOO_LiveDispatch_GetOffset(int i)
	{
		return LiveDispatchGen::Offset(i);
	}
	__declspec(dllexport) int __cdecl D2MOO_LiveDispatch_GetMode(int i)
	{
		return LiveDispatchGen::GetMode(i);
	}
	__declspec(dllexport) void __cdecl D2MOO_LiveDispatch_SetMode(int i, int m)
	{
		LiveDispatchGen::SetMode(i, m);
	}
	__declspec(dllexport) unsigned long long __cdecl D2MOO_LiveDispatch_GetHits(int i)
	{
		return LiveDispatchGen::Hits(i);
	}
	__declspec(dllexport) unsigned long long __cdecl D2MOO_LiveDispatch_GetDivergences(int i)
	{
		return LiveDispatchGen::Divergences(i);
	}
	// input diversity (SHIPPING_PROMOTION_PLAN.md) -- battletest_promoter fails
	// closed without these, so an older build here correctly stalls promotion
	// for this module rather than promoting on volume alone.
	__declspec(dllexport) unsigned long long __cdecl D2MOO_LiveDispatch_GetDistinctInputs(int i)
	{
		return LiveDispatchGen::DistinctInputs(i);
	}
	__declspec(dllexport) int __cdecl D2MOO_LiveDispatch_GetArgCount(int i)
	{
		return LiveDispatchGen::ArgCount(i);
	}
	// Recorded input VALUES, so coverage guidance can name the ids already seen
	// instead of guessing at an activity. which: 0=arg0, 1=arg1, 2=slot hash
	// (0 == unused). Sweep slot in [0, GetSampleSlotCount()).
	__declspec(dllexport) unsigned int __cdecl D2MOO_LiveDispatch_GetSampleValue(
		int i, int slot, int which)
	{
		return LiveDispatchGen::SampleValue(i, slot, which);
	}
	__declspec(dllexport) int __cdecl D2MOO_LiveDispatch_GetSampleSlotCount(void)
	{
		return LiveDispatchGen::SampleSlotCount();
	}
	// WS-5 direct-call oracle support: raw original + currently-bound reimpl.
	__declspec(dllexport) void* __cdecl D2MOO_LiveDispatch_GetTrampoline(int i)
	{
		return LiveDispatchGen::Trampoline(i);
	}
	__declspec(dllexport) void* __cdecl D2MOO_LiveDispatch_GetReimpl(int i)
	{
		return LiveDispatchGen::Reimpl(i);
	}
	// WS-1 hot-reload bridge: repoint dispatcher i's reimpl to a function from
	// the (re)loaded provider DLL. Call ONLY between QuiesceForReload() and
	// re-enabling modes (D2Debugger's ReloadProvider already does this across
	// EVERY resolved module, this one included).
	__declspec(dllexport) void __cdecl D2MOO_LiveDispatch_SetReimpl(int i, void* fn)
	{
		LiveDispatchGen::SetReimpl(i, fn);
	}
	// Quiesce for a provider hot-reload: force every dispatcher to Original,
	// then wait for in-flight reimpl calls to drain (own drain counter -- no
	// coord family to share LiveDispatch::g_reimplInFlight with here). Mirrors
	// the coord family's exact (lenient) semantics: fast path on InFlight==0,
	// else a grace period once every mode is already Original (no NEW reimpl
	// call can start; an in-flight one finishes in microseconds), so this
	// ALWAYS eventually returns 1 -- never signals failure. Matching that is
	// required: D2Debugger's ReloadProvider treats `quiesce() != 1` as fatal
	// and aborts the reload for every resolved module.
	__declspec(dllexport) int __cdecl D2MOO_LiveDispatch_QuiesceForReload()
	{
		LiveDispatchGen::QuiesceModes();
		const int kGraceMs = 300;
		for (int spin = 0; spin < 5000; ++spin)
		{
			if (LiveDispatchGen::InFlight() == 0) return 1;
			if (spin >= kGraceMs) return 1;
			Sleep(1);
		}
		return 1;
	}
}
""")

    return "".join(parts)


def generate_module(cfg):
    """Generate one MODULES[] entry. Missing manifest = SKIP, not an error --
    a module config can exist (so IMAGE_BASES/validate_ret_bits know about it)
    before its manifest is authored."""
    if not os.path.exists(cfg["manifest"]):
        print(f"[gen_shadow_dispatch] {cfg['program']}: no manifest at "
              f"{cfg['manifest']} -- skipping")
        return
    with open(cfg["manifest"], "r", encoding="utf-8") as f:
        manifest = json.load(f)
    # normalize offset (accept hex string or int)
    for e in manifest["entries"]:
        if isinstance(e["offset"], str):
            e["offset"] = int(e["offset"], 0)
    # Validate BEFORE emitting: a wrong ret_bits compiles fine and only shows up
    # later as thousands of false divergences that refute a correct reimpl.
    dupes = validate_unique_offsets(manifest["entries"])
    if dupes:
        print(f"[gen_shadow_dispatch] {cfg['program']}: {len(dupes)} duplicate hook "
              f"offset(s) above -- the later entry will NEVER hook. Remove it.")
    bad = validate_ret_bits(manifest["entries"], base=cfg["base"], program=cfg["program"])
    if bad:
        print(f"[gen_shadow_dispatch] {cfg['program']}: {len(bad)} ret-width "
              f"mismatch(es) above -- fix the manifest before trusting any "
              f"divergence from these.")
    # FATAL, unlike the two checks above: those degrade the evidence, this one
    # crashes the game. See validate_argc's docstring for the 2026-07-30 outage.
    argc_bad = validate_argc(manifest["entries"], base=cfg["base"], program=cfg["program"])
    if argc_bad:
        raise SystemExit(
            f"[gen_shadow_dispatch] {cfg['program']}: {len(argc_bad)} ARGC mismatch(es) "
            f"above -- REFUSING to generate. A thunk that pops the wrong number of "
            f"bytes skews ESP and access-violates the game; a build that cannot load "
            f"a save is worse than no build. Correct `args` to the callee's real "
            f"arity (its RET n) and re-run.")
    # kModuleName must be the name the GAME loads, which is not always the name
    # the Ghidra program carries -- see the SGD2FreeRes MODULES entry. Defaults to
    # `program`, so D2's own DLLs are unaffected.
    text = emit(manifest, module_name=cfg.get("module", cfg["program"]),
               out_name=os.path.basename(cfg["out"]), standalone=cfg["standalone"])
    with open(cfg["out"], "w", encoding="utf-8") as f:
        f.write(text)
    print(f"[gen_shadow_dispatch] wrote {cfg['out']} ({len(manifest['entries'])} dispatcher(s))")
    for e in manifest["entries"]:
        print(f"    - {e['name']} @ 0x{e['offset']:x} ({e['callconv']}, class {e.get('class','A')})")


def main():
    for cfg in MODULES:
        generate_module(cfg)


if __name__ == "__main__":
    main()
