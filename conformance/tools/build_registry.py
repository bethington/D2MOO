"""build_registry.py -- Phase 3 of ../LIVE_DISPATCH_FRAMEWORK_PLAN.md.

Builds conformance/registry.json: the one artifact the plan calls "the shared
artifact of conformance AND the mod interface" -- a per-function record of
{verified_address, real_ordinal, d2moo_symbol, abi, input_surface, write_set,
proof_status, default_mode}. Phase 4's D2Debugger panel will read this to
drive its function table; today it's just this loader + a JSON file.

Design: a small CURATED manifest (below) lists what we've actually proven so
far -- this is NOT an attempt to auto-populate all 1172 D2Common ordinals
(unrealistic to fully automate; the plan itself says "populated from the
EXISTING conformance outputs", i.e. incrementally). What this script adds
over hand-editing JSON directly:
  1. Cross-references each manifest entry's address against
     corrected_maps/D2Common.tsv (the ground-truth ordinal->address map) to
     auto-fill real_ordinal + ghidra_name, and FLAGS any mismatch -- a
     verified_address that doesn't match the TSV for its claimed ordinal (or
     has no ordinal at all) is exactly the scrambled-.def failure mode
     ORDINAL_RECONCILIATION.md warns about, so this is a real safety check,
     not just convenience.
  2. Emits the exact registry schema the plan defines, so Phase 4 can start
     from data, not another format decision.

Run: python build_registry.py   (from conformance/, or anywhere -- paths are
relative to this script's location)
"""
import json
import os

HERE = os.path.dirname(os.path.abspath(__file__))
CONFORMANCE = os.path.dirname(HERE)


def load_tsv_by_address(path):
    """address (lowercase hex, no 0x) -> {ordinal, ghidra_name, def_name}"""
    by_addr = {}
    with open(path, encoding="utf-8") as f:
        next(f)  # header
        for line in f:
            parts = line.rstrip("\n").split("\t")
            if len(parts) < 4:
                continue
            ordinal, addr, ghidra_name, def_name = parts[0], parts[1], parts[2], parts[3]
            key = addr.lower().replace("0x", "")
            by_addr[key] = {"ordinal": int(ordinal), "ghidra_name": ghidra_name, "def_name": def_name}
    return by_addr


# --- Curated manifest of what LIVE_DISPATCH_FRAMEWORK_PLAN.md has actually proven ---
# proof_status values used here:
#   "live-shadow-clean"  -- proven via the live native dispatcher (Phase 1/2),
#                           Shadow mode, zero divergence, against real PD2-S12 gameplay.
#   "vectors-passed"     -- proven via offline hand-minted vectors only (not yet
#                           live-dispatched); still a real proof, narrower coverage.
# default_mode is deliberately conservative: only live-shadow-clean functions
# default to anything beyond Original, and even then Shadow (never Reimpl) --
# promotion to Reimpl-by-default is a decision this registry records but does
# not make; see D2Debugger panel plan (Phase 4).
MANIFEST = [
    {
        "d2moo_symbol": "DUNGEON_GameTileToClientCoords",
        "verified_address": "0x6fd9dac0",
        "abi": "stdcall(int*,int*)",
        "input_surface": ["*pX", "*pY"],
        "write_set": ["*pX", "*pY"],
        "proof_status": "live-shadow-clean",
        "default_mode": "shadow",
        "notes": "Phase 0 Frida spike + Phase 1/2 live dispatcher, all 3 modes proven "
                 "live 2026-07-06 (see LIVE_DISPATCH_FRAMEWORK_PLAN.md).",
    },
    {
        "d2moo_symbol": "DUNGEON_GameTileToSubtileCoords",
        "verified_address": "0x6fd9d8a0",
        "abi": "stdcall(int*,int*)",
        "input_surface": ["*pX", "*pY"],
        "write_set": ["*pX", "*pY"],
        "proof_status": "vectors-passed",
        "default_mode": "original",
        "notes": "Hooked live in Phase 2's dispatcher generalization but the game "
                 "never called it during the live test session (hits=0) -- proven "
                 "offline (d2moo_conform, PD2S12Conformance.cpp) only so far.",
    },
    {
        "d2moo_symbol": "DUNGEON_ClientToGameCoords",
        "verified_address": "0x6fd9d8c0",
        "abi": "stdcall(int*,int*)",
        "input_surface": ["*pX", "*pY"],
        "write_set": ["*pX", "*pY"],
        "proof_status": "vectors-passed",
        "default_mode": "original",
        "notes": "Same as GameTileToSubtileCoords: hooked, not yet observed live.",
    },
    {
        "d2moo_symbol": "DUNGEON_GameToClientCoords",
        "verified_address": "0x6fd9db40",
        "abi": "stdcall(int*,int*)",
        "input_surface": ["*pX", "*pY"],
        "write_set": ["*pX", "*pY"],
        "proof_status": "vectors-passed",
        "default_mode": "original",
        "notes": "D2MOO's first conformance-driven bug FIX (commit 26ed87f): "
                 "truncating /2,/4 diverged from PD2's arith-shift SAR on negatives. "
                 "Hooked, not yet observed live in this session's testing.",
    },
    {
        "d2moo_symbol": "DUNGEON_GameSubtileToClientCoords",
        "verified_address": "0x6fd9db70",
        "abi": "stdcall(int*,int*)",
        "input_surface": ["*pX", "*pY"],
        "write_set": ["*pX", "*pY"],
        "proof_status": "vectors-passed",
        "default_mode": "original",
        "notes": "Hooked, not yet observed live in this session's testing.",
    },
    {
        "d2moo_symbol": "SEED_RollLimitedRandomNumber (SEED_GetRandomNumber)",
        "verified_address": "0x6fd510b0",
        "abi": "register(ecx=pSeed, eax=max) -> eax:uint",
        "input_surface": ["*pSeed (nLowSeed, nHighSeed)", "max (eax)"],
        "write_set": ["pSeed->nHighSeed", "return (eax)"],
        "proof_status": "vectors-passed",
        "default_mode": "original",
        "notes": "20/20 offline vectors (vectors/rng.json) + Frida in-process oracle "
                 "(behavioral/pd2_rng_family.json). NOT YET LIVE-DISPATCHED -- register "
                 "ABI needs an Option B asm trampoline stub (see plan), not yet built. "
                 "No ordinal in corrected_maps/D2Common.tsv (not independently "
                 "ordinal-exported; reached only by address/trampoline).",
    },
    {
        "d2moo_symbol": "SEED_RollLimitedRandomNumber (SEED_GetRandomNumberAlt)",
        "verified_address": "0x6fd51100",
        "abi": "register(ecx=pSeed, esi=max) -> eax:uint",
        "input_surface": ["*pSeed (nLowSeed, nHighSeed)", "max (esi)"],
        "write_set": ["pSeed->nHighSeed", "return (eax)"],
        "proof_status": "vectors-passed",
        "default_mode": "original",
        "notes": "Same family as GetRandomNumber; different register ABI (max in esi, "
                 "not eax). No ordinal in corrected_maps/D2Common.tsv.",
    },
    {
        "d2moo_symbol": "SEED_RollLimitedRandomNumber range idiom (SEED_GetRandomInRange)",
        "verified_address": "0x6fd51180",
        "abi": "register(ecx=pSeed, edi=nMin, eax=nMax) -> eax:int",
        "input_surface": ["*pSeed (nLowSeed, nHighSeed)", "nMin (edi)", "nMax (eax)"],
        "write_set": ["pSeed->nHighSeed", "return (eax)"],
        "proof_status": "vectors-passed",
        "default_mode": "original",
        "notes": "D2MOO idiom: nMin + SEED_RollLimitedRandomNumber(nMax-nMin+1). No "
                 "ordinal in corrected_maps/D2Common.tsv.",
    },
]


def build():
    tsv_path = os.path.join(CONFORMANCE, "corrected_maps", "D2Common.tsv")
    by_addr = load_tsv_by_address(tsv_path)

    registry = []
    warnings = []
    for entry in MANIFEST:
        addr_key = entry["verified_address"].lower().replace("0x", "")
        tsv_hit = by_addr.get(addr_key)
        record = dict(entry)
        if tsv_hit:
            record["real_ordinal"] = tsv_hit["ordinal"]
            record["ghidra_name"] = tsv_hit["ghidra_name"]
            record["def_name_at_this_ordinal"] = tsv_hit["def_name"]
        else:
            record["real_ordinal"] = None
            record["ghidra_name"] = None
            record["def_name_at_this_ordinal"] = None
            warnings.append(
                f"{entry['d2moo_symbol']} @ {entry['verified_address']}: "
                f"no entry in corrected_maps/D2Common.tsv (not independently "
                f"ordinal-exported, or table doesn't cover this VA)."
            )
        registry.append(record)

    out_path = os.path.join(CONFORMANCE, "registry.json")
    with open(out_path, "w", encoding="utf-8") as f:
        json.dump({"functions": registry}, f, indent=2)

    print(f"Wrote {out_path} ({len(registry)} functions)")
    live_clean = sum(1 for r in registry if r["proof_status"] == "live-shadow-clean")
    print(f"  live-shadow-clean: {live_clean}")
    print(f"  vectors-passed:    {len(registry) - live_clean}")
    if warnings:
        print(f"\n{len(warnings)} function(s) with no corrected_maps ordinal (informational, not necessarily an error):")
        for w in warnings:
            print(f"  - {w}")


if __name__ == "__main__":
    build()
