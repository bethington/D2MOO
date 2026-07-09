"""gen_resolve_table.py -- WS-1.5 (GRADUATED_CONFORMANCE_PIPELINE_PLAN.md detail A2).

Emits a C++ header mapping D2Common function NAME -> VERIFIED runtime address,
from corrected_maps/D2Common.tsv (the ground-truth ordinal->address->ghidra_name).
The patch exposes it via D2MOO_ResolveGameFn(name) so a reimpl can resolve its
game dependencies by VERIFIED ADDRESS -- never by the scrambled export table
(ORDINAL_RECONCILIATION.md). This is the shared foundation for both the
MemoryModule custom import resolver and the dependency-injection fallback.

Run: python gen_resolve_table.py
Output: D2.Detours.patches/1.13c/D2Common_ResolveTable.gen.h
"""
import os

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
TSV = os.path.join(ROOT, "conformance", "corrected_maps", "D2Common.tsv")
OUT = os.path.join(ROOT, "D2.Detours.patches", "1.13c", "D2Common_ResolveTable.gen.h")


def main():
    rows = []
    seen = set()
    with open(TSV, encoding="utf-8") as f:
        next(f)  # header
        for line in f:
            p = line.rstrip("\n").split("\t")
            if len(p) < 3:
                continue
            addr, ghidra_name = p[1], p[2]
            if not ghidra_name or ghidra_name in seen:
                continue  # keep first occurrence of each name
            try:
                a = int(addr, 16)
            except ValueError:
                continue
            # only real code names (skip data ptrs / unnamed placeholders)
            if ghidra_name.startswith((";", "?", "-")) or " " in ghidra_name:
                continue
            seen.add(ghidra_name)
            rows.append((ghidra_name, a))

    # Curated DATA globals (name -> verified address). Ghidra data symbols aren't
    # in corrected_maps (which is function-oriented), so reimpls that read REAL
    # game globals BY NAME via the injected resolver (provider_runtime.h) get the
    # addresses from here. Extend as reimpls need more globals; verify each with
    # Ghidra list_globals. (Stateful rung 2 of GRADUATED_CONFORMANCE_PIPELINE_PLAN.)
    DATA_GLOBALS = {
        "g_dwDataTableBase": 0x6fdee2cc,  # bounded data-table base (GetDataTableRowEntryCount)
        "g_anTownLevelIds":  0x6fde4084,  # 5 act town level ids (DUNGEON_GetTownLevelIdFromActNo)
        # Item record array + its count (GetItemDataRecord @0x6fdc19a0, stride 0x1a8).
        # Addresses read straight off the disassembly operands (CMP/MOV), 2026-07-08.
        "g_dwItemRecordCount":      0x6fdefb94,  # CMP bound in GetItemDataRecord
        "g_pItemRecords":           0x6fdefb98,  # base ptr (pointer variable -> deref once)
        # Objects/anim-sequence table + its count (GetAnimSequenceRecord @0x6fd8e980, stride 0x1c0).
        "g_pObjectsTxtTable":       0x6fdf0b94,  # base ptr (pointer variable -> deref once)
        "g_dwObjectsTxtRecordCount":0x6fdf0b98,  # CMP bound in GetAnimSequenceRecord
        # Sorted item-code search table (ITEMS_LookupItemRecordByCode @0x6fdc1960:
        # MOV ECX,[0x6fdeff6c] -> pointer variable, deref once).
        "g_pItemDataBuffer":        0x6fdeff6c,
        # Item TYPE code table (ushort per item record) -- GetItemTypeCodeByIndex @0x6fdc19f0:
        # MOV EAX,[0x6fdefbb4] -> pointer variable, deref once; count reuses g_dwItemRecordCount.
        "g_pItemTypeCodeTable":     0x6fdefbb4,
        # Experience/levels table (stride 0x20) -- GetItemLevelCapByIndex @0x6fdae840:
        # MOV ECX,[0x6fdf0b50] -> pointer variable, deref once; count at record[0]+0, cap at +0x3c.
        "g_pExperienceTxtRecords":  0x6fdf0b50,
        # DELEGATE FUNCTION (not data): Fog.dll binary-search IAT thunk, plain
        # __stdcall(pTable, pKey, nFlag) w/ 3 stack args. The resolver is
        # name->address agnostic, so reimpls resolve it and call through a normal
        # fn pointer (the delegate-rung pattern). Not in corrected_maps, so wired here.
        "BinarySearchInSortedArray": 0x6fd59240,
    }
    for name, a in DATA_GLOBALS.items():
        if name not in seen:
            seen.add(name)
            rows.append((name, a))

    # pending_globals.json -- the port planner's staging file (port_targets.py
    # --plan --wire-globals). It collects EVERY missing global a batch needs so
    # one rebuild+restart activates all of them (the July batch restarted 3x
    # discovering these serially). Format: {"name": "0xADDR" | int}. Auto-staged
    # names are g_dat_<hex> placeholders -- rename them here (or in the file)
    # once the semantic name is known; curated DATA_GLOBALS entries win on clash.
    pending_path = os.path.join(HERE, "pending_globals.json")
    if os.path.exists(pending_path):
        import json
        with open(pending_path, encoding="utf-8") as f:
            pending = json.load(f)
        added = 0
        for name, a in pending.items():
            if name not in seen:
                seen.add(name)
                rows.append((name, int(a, 0) if isinstance(a, str) else int(a)))
                added += 1
        if added:
            print(f"[gen_resolve_table] merged {added} pending global(s) from {pending_path}")

    rows.sort(key=lambda r: r[0])  # sorted by name for readability / future bsearch
    with open(OUT, "w", encoding="utf-8") as f:
        f.write("// AUTO-GENERATED by conformance/tools/gen_resolve_table.py -- do not edit.\n")
        f.write("// D2Common function NAME -> VERIFIED absolute address (game D2Common @ 0x6fd50000).\n")
        f.write("// Source of truth: conformance/corrected_maps/D2Common.tsv (Ghidra names).\n")
        f.write("#pragma once\n\n")
        f.write("struct D2MOO_ResolveEntry { const char* name; unsigned int addr; };\n\n")
        f.write("static const D2MOO_ResolveEntry g_d2moo_resolve_table[] = {\n")
        for name, a in rows:
            f.write('\t{ "%s", 0x%xu },\n' % (name, a))
        f.write("};\n")
        f.write("static const int g_d2moo_resolve_count = %d;\n" % len(rows))

    print(f"Wrote {OUT} ({len(rows)} entries)")


if __name__ == "__main__":
    main()
