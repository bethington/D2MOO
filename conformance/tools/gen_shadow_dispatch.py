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

Usage:  python gen_shadow_dispatch.py
"""
import json
import os

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
MANIFEST = os.path.join(ROOT, "conformance", "shadow_manifest.json")
OUT = os.path.join(ROOT, "D2.Detours.patches", "1.13c", "D2Common_ShadowDispatch.gen.h")

CC = {"fastcall": "__fastcall", "stdcall": "__stdcall", "cdecl": "__cdecl"}


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


def emit_dispatcher(e):
    if e.get("class", "A").upper() == "B":
        return emit_dispatcher_b(e)
    return emit_dispatcher_a(e)


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
    return f"""// {name} -- class B (void out-param, {e['callconv']}, out arg a{oi} = {nbytes} bytes) -- off 0x{e['offset']:x}
namespace {ns} {{
	static std::atomic<int32_t> mode{{ (int32_t)LiveDispatchGen::Mode::Original }};
	static void* trampoline = nullptr;
	static uint64_t hits = 0, divergences = 0;
	static void* reimpl = nullptr;   // bound from the provider DLL BY NAME (D2Debugger)
	static void {cc} Thunk({params}) {{
		++hits;
		using Fn = void({cc}*)({fnptr_args});
		const Fn orig = (Fn)trampoline;
		const Fn rfn  = (Fn)reimpl;
		const LiveDispatchGen::Mode m = LiveDispatchGen::tl_inDispatch
			? LiveDispatchGen::Mode::Original
			: (LiveDispatchGen::Mode)mode.load(std::memory_order_relaxed);
		if (m == LiveDispatchGen::Mode::Reimpl && rfn) {{ LiveDispatchGen::Guard g; rfn({orig_call}); return; }}
		if (m != LiveDispatchGen::Mode::Shadow || !orig || !rfn) {{ if (orig) orig({orig_call}); return; }}
		// Shadow: snapshot the out-buffer's INPUT bytes, let ORIGINAL write the
		// game's buffer (it wins), then run reimpl on an INDEPENDENT copy of the
		// input and compare -- the game's memory is never double-mutated.
		unsigned char inbuf[{nbytes}], origOut[{nbytes}], local[{nbytes}];
		memcpy(inbuf, (const void*)(uintptr_t)a{oi}, {nbytes});
		orig({orig_call});
		memcpy(origOut, (const void*)(uintptr_t)a{oi}, {nbytes});
		memcpy(local, inbuf, {nbytes});
		{{ LiveDispatchGen::Guard g; rfn({reimpl_call}); }}
		if (memcmp(local, origOut, {nbytes}) != 0) {{
			++divergences;
			const uint32_t av[] = {{ {logargs} }};
			LiveDispatchGen::LogDivergenceBuf("{name}", av, {argc}, origOut, local, {nbytes});
		}}
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
    if argc:
        av = "; ".join([f"const uint32_t av[] = {{ {argnames} }}"]) + ";"
        logcall = f'LiveDispatchGen::LogDivergence("{name}", av, {argc}, ro, rr);'
    else:
        av = ""
        logcall = f'LiveDispatchGen::LogDivergence("{name}", nullptr, 0, ro, rr);'
    return f"""// {name} -- class A (return-value integer, {e['callconv']}, {argc} arg(s), ret {retbits}-bit) -- off 0x{e['offset']:x}
namespace {ns} {{
	static std::atomic<int32_t> mode{{ (int32_t)LiveDispatchGen::Mode::Original }};
	static void* trampoline = nullptr;
	static uint64_t hits = 0, divergences = 0;
	static void* reimpl = nullptr;   // bound from the provider DLL BY NAME (D2Debugger)
	static uint32_t {cc} Thunk({params}) {{
		++hits;
		using Fn = uint32_t({cc}*)({fnptr_args});
		const Fn orig = (Fn)trampoline;
		const Fn rfn  = (Fn)reimpl;
		const LiveDispatchGen::Mode m = LiveDispatchGen::tl_inDispatch
			? LiveDispatchGen::Mode::Original
			: (LiveDispatchGen::Mode)mode.load(std::memory_order_relaxed);
		if (m == LiveDispatchGen::Mode::Reimpl && rfn) {{ LiveDispatchGen::Guard g; return rfn({argnames}); }}
		if (m != LiveDispatchGen::Mode::Shadow || !orig || !rfn) return orig ? orig({argnames}) : 0u;
		const uint32_t ro = orig({argnames});
		uint32_t rr;
		{{ LiveDispatchGen::Guard g; rr = rfn({argnames}); }}
		const uint32_t mask = LiveDispatchGen::RetMask({retbits});
		if ((ro & mask) != (rr & mask)) {{
			++divergences;
			{av}
			{logcall}
		}}
		return ro;
	}}
}}
"""


def emit(manifest):
    entries = manifest["entries"]
    parts = []
    parts.append("""#pragma once
// D2Common_ShadowDispatch.gen.h -- GENERATED by conformance/tools/gen_shadow_dispatch.py
// from conformance/shadow_manifest.json. DO NOT EDIT BY HAND; edit the manifest +
// regenerate. Full-D2Common 1:1 shadow-conformance (D2COMMON_FULL_SHADOW_PLAN.md).
//
// Include AFTER DetoursPatch.h (Install() uses HookContext/PatchAction) and after
// LiveDispatch_Generic.h. Single-TU home (D2Common.patch.cpp), same model as the
// coord header.
#include <cstdio>
#include <cstring>
#include "LiveDispatch_Generic.h"

// One-time shared definitions (declared extern in LiveDispatch_Generic.h).
namespace LiveDispatchGen {
	thread_local bool tl_inDispatch = false;
	std::atomic<int> g_inFlight{ 0 };
	static const char* kDivergencePath =
		"C:\\\\Users\\\\benam\\\\source\\\\cpp\\\\D2MOO\\\\conformance\\\\behavioral\\\\live_shadow_divergences.jsonl";
	static void WriteArgsJson(FILE* f, const uint32_t* args, int nargs) {
		fprintf(f, "\\"args\\":[");
		for (int i = 0; i < nargs; ++i) fprintf(f, "%s%u", i ? "," : "", args[i]);
		fprintf(f, "]");
	}
	void LogDivergence(const char* fn, const uint32_t* args, int nargs, uint32_t o, uint32_t r) {
		FILE* f = nullptr;
		if (fopen_s(&f, kDivergencePath, "a") == 0 && f) {
			fprintf(f, "{\\"fn\\":\\"%s\\",", fn);
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
			fprintf(f, "{\\"fn\\":\\"%s\\",", fn);
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
}
""")
    for e in entries:
        parts.append(emit_dispatcher(e))

    # Bridge table + accessors.
    parts.append("namespace LiveDispatchGen {\n")
    if entries:
        parts.append("\tstatic GenEntry g_entries[] = {\n")
        for e in entries:
            ns = ns_of(e["name"])
            parts.append(
                f'\t\t{{ "{e["name"]}", 0x{e["offset"]:x}, &{ns}::mode, &{ns}::hits, '
                f'&{ns}::divergences, (void**)&{ns}::reimpl, &{ns}::trampoline }},\n')
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
	unsigned long long Divergences(int i) { return (i >= 0 && i < kGenCount) ? *g_entries[i].divergences : 0ull; }
	void* Trampoline(int i) { return (i >= 0 && i < kGenCount) ? *g_entries[i].trampolineSlot : nullptr; }
	void* Reimpl(int i) { return (i >= 0 && i < kGenCount) ? *g_entries[i].reimplSlot : nullptr; }
	void SetReimpl(int i, void* fn) { if (i >= 0 && i < kGenCount) *g_entries[i].reimplSlot = fn; }
	void QuiesceModes() { for (int i = 0; i < kGenCount; ++i) g_entries[i].mode->store(0, std::memory_order_seq_cst); }
	int InFlight() { return g_inFlight.load(std::memory_order_seq_cst); }
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
    return "".join(parts)


def main():
    with open(MANIFEST, "r", encoding="utf-8") as f:
        manifest = json.load(f)
    # normalize offset (accept hex string or int)
    for e in manifest["entries"]:
        if isinstance(e["offset"], str):
            e["offset"] = int(e["offset"], 0)
    text = emit(manifest)
    with open(OUT, "w", encoding="utf-8") as f:
        f.write(text)
    print(f"[gen_shadow_dispatch] wrote {OUT} ({len(manifest['entries'])} dispatcher(s))")
    for e in manifest["entries"]:
        print(f"    - {e['name']} @ 0x{e['offset']:x} ({e['callconv']}, class {e.get('class','A')})")


if __name__ == "__main__":
    main()
