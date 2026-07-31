"""gen_resolve_table.py -- WS-1.5 (GRADUATED_CONFORMANCE_PIPELINE_PLAN.md detail A2).

Emits a C++ header mapping NAME -> (MODULE, RVA), from corrected_maps/D2Common.tsv
(the ground-truth ordinal->address->ghidra_name) plus curated + staged globals.
The patch exposes it via D2MOO_ResolveGameFn(name) so a reimpl can resolve its
game dependencies by VERIFIED IDENTITY -- never by the scrambled export table
(ORDINAL_RECONCILIATION.md). This is the shared foundation for both the
MemoryModule custom import resolver and the dependency-injection fallback.

MODULE+RVA, NOT ABSOLUTE (2026-07-30). This table used to emit the Ghidra
ABSOLUTE address, which silently assumed every module loads at its preferred
base. D2Common (0x6fd50000) and D2Game (0x6fc20000) do; D2Client does NOT -- the
live process maps it at 0x03600000 and leaves 0x6fab0000 unmapped. 3,392 of the
3,420 staged globals are D2Client-range, so 75% of this table handed reimpls
pointers that fault on first dereference. Emitting (module, rva) and adding the
runtime base in D2MOO_ResolveGameFn makes the table correct for a relocated
module and keeps it correct across relaunches.

Run: python gen_resolve_table.py
Output: D2.Detours.patches/1.13c/D2Common_ResolveTable.gen.h
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
TSV = os.path.join(ROOT, "conformance", "corrected_maps", "D2Common.tsv")
OUT = os.path.join(ROOT, "D2.Detours.patches", "1.13c", "D2Common_ResolveTable.gen.h")

# Ghidra image base -> module, for attributing an absolute address to the module
# that owns it. Values are each DLL's PE OptionalHeader.ImageBase + SizeOfImage
# (1.13c / PD2-S12 set, read off C:\Diablo2\ProjectD2), which is exactly the base
# Ghidra loads at -- so every absolute address authored from Ghidra falls in
# exactly one range. Verify with --verify-ranges <game_dir> after a version bump.
#
# Storm.dll and D2Net.dll BOTH prefer 0x6fbf0000, so that window is genuinely
# ambiguous and one of the two is always relocated. Attribution refuses to guess
# there (see _module_for_address); no current entry lands in it.
MODULE_RANGES = (
    ("D2Glide.dll",     0x6F850000, 0x6F86A000),
    ("D2Gdi.dll",       0x6F870000, 0x6F87E000),
    ("D2Direct3D.dll",  0x6F880000, 0x6F8B6000),
    ("D2DDraw.dll",     0x6F8C0000, 0x6F8D3000),
    ("D2Win.dll",       0x6F8E0000, 0x6F9AF000),
    ("D2sound.dll",     0x6F9B0000, 0x6F9C9000),
    ("D2Multi.dll",     0x6F9D0000, 0x6FA0F000),
    ("D2MCPClient.dll", 0x6FA20000, 0x6FA34000),
    ("D2Launch.dll",    0x6FA40000, 0x6FA6D000),
    ("D2gfx.dll",       0x6FA80000, 0x6FAA1000),
    ("D2Client.dll",    0x6FAB0000, 0x6FBE5000),
    ("D2Net.dll",       0x6FBF0000, 0x6FBFD000),
    ("Storm.dll",       0x6FBF0000, 0x6FC50000),
    ("D2Lang.dll",      0x6FC00000, 0x6FC14000),
    ("D2Game.dll",      0x6FC20000, 0x6FD42000),
    ("D2Common.dll",    0x6FD50000, 0x6FDF9000),
    ("D2CMP.dll",       0x6FE10000, 0x6FF18000),
    ("Bnclient.dll",    0x6FF20000, 0x6FF44000),
    ("Fog.dll",         0x6FF50000, 0x6FFAC000),
)


class AttributionError(Exception):
    """An address that no single module owns. Fatal by design: guessing a module
    reintroduces exactly the bad-pointer class this rewrite exists to remove."""


def _module_for_address(addr):
    """The module whose Ghidra image range contains addr.

    Ranges OVERLAP, because several DLLs declare an image larger than the gap to
    the next one's preferred base -- Storm.dll spans 0x6fbf0000-0x6fc50000 and so
    covers D2Net's, D2Lang's and D2Game's bases outright. Only one of a colliding
    set can actually get its preferred base; the rest are relocated. Ghidra,
    however, loads every DLL at its OWN preferred base as a separate program, so
    an address authored from Ghidra belongs to the module whose base is CLOSEST
    BELOW it -- the tightest containing range. 0x6fc30000 is D2Game (base
    0x6fc20000), not Storm (base 0x6fbf0000).

    Raises when nothing contains the address (the range table needs the module)
    and when two modules share the SAME base, which is a true tie (Storm and
    D2Net both prefer 0x6fbf0000). Both are fatal on purpose: attributing to the
    wrong module rebases against the wrong live base, which is the same silent
    bad-pointer failure this rewrite exists to remove.

    (An address alone is a lossy key. If pending_globals.json ever records the
    source binary alongside the address, prefer that provenance over this
    geometric inference.)
    """
    hits = [(lo, m) for m, lo, hi in MODULE_RANGES if lo <= addr < hi]
    if not hits:
        raise AttributionError(
            f"0x{addr:08x} falls in no known module range -- add the module to "
            f"MODULE_RANGES (its PE ImageBase + SizeOfImage)")
    best = max(lo for lo, _m in hits)
    tied = sorted(m for lo, m in hits if lo == best)
    if len(tied) == 1:
        return tied[0]
    raise AttributionError(
        f"0x{addr:08x} is claimed by {len(tied)} modules sharing base 0x{best:08x} "
        f"({', '.join(tied)}) -- the entry must name its module explicitly")


def verify_ranges(game_dir):
    """Re-derive MODULE_RANGES from a game directory's PE headers and diff.

    Run after a game-version bump: a stale range table mis-attributes addresses,
    and mis-attribution is indistinguishable from correct output until a reimpl
    dereferences the result in-game.
    """
    import glob
    import struct

    actual = {}
    for path in sorted(glob.glob(os.path.join(game_dir, "*.dll"))):
        try:
            head = open(path, "rb").read(0x400)
            e_lfanew = struct.unpack_from("<I", head, 0x3C)[0]
            opt = e_lfanew + 0x18
            if struct.unpack_from("<H", head, opt)[0] != 0x10B:  # PE32 only
                continue
            base = struct.unpack_from("<I", head, opt + 0x1C)[0]
            size = struct.unpack_from("<I", head, opt + 0x38)[0]
        except (OSError, struct.error):
            continue
        if base < 0x6F000000:  # relocatable helpers (0x10000000) aren't in the table
            continue
        actual[os.path.basename(path)] = (base, base + size)

    declared = {m: (lo, hi) for m, lo, hi in MODULE_RANGES}
    bad = 0
    for mod, (lo, hi) in sorted(actual.items()):
        if mod not in declared:
            print(f"[MISSING] {mod}: 0x{lo:08x}-0x{hi:08x} not in MODULE_RANGES")
            bad += 1
        elif declared[mod] != (lo, hi):
            d_lo, d_hi = declared[mod]
            print(f"[STALE]   {mod}: table 0x{d_lo:08x}-0x{d_hi:08x} "
                  f"actual 0x{lo:08x}-0x{hi:08x}")
            bad += 1
    for mod in sorted(set(declared) - set(actual)):
        print(f"[EXTRA]   {mod}: in MODULE_RANGES but not found in {game_dir}")
        bad += 1
    print(f"{'FAIL' if bad else 'OK'}: {len(actual)} module(s) checked, {bad} discrepancy(ies)")
    return 1 if bad else 0


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

    # Attribute each absolute address to its owning module and convert to an RVA.
    # An unattributable address is FATAL: emitting it anyway is how 3,392 D2Client
    # globals shipped as addresses that fault on first dereference.
    entries, failures = [], []
    for name, addr in rows:
        try:
            entries.append((name, _module_for_address(addr), addr))
        except AttributionError as exc:
            failures.append(f"  {name}: {exc}")
    if failures:
        print(f"[gen_resolve_table] FATAL: {len(failures)} address(es) could not be "
              f"attributed to a module:", file=sys.stderr)
        print("\n".join(failures[:40]), file=sys.stderr)
        if len(failures) > 40:
            print(f"  ... and {len(failures) - 40} more", file=sys.stderr)
        return 1

    by_module = {}
    for _, mod, _a in entries:
        by_module[mod] = by_module.get(mod, 0) + 1

    with open(OUT, "w", encoding="utf-8") as f:
        f.write("// AUTO-GENERATED by conformance/tools/gen_resolve_table.py -- do not edit.\n")
        f.write("// NAME -> (MODULE, RVA). D2MOO_ResolveGameFn adds the module's RUNTIME base,\n")
        f.write("// so an entry stays correct for a module that did NOT get its preferred base\n")
        f.write("// (D2Client loads at 0x03600000 live, not Ghidra's 0x6fab0000).\n")
        f.write("// Source of truth: conformance/corrected_maps/D2Common.tsv (Ghidra names)\n")
        f.write("// + curated DATA_GLOBALS + conformance/tools/pending_globals.json.\n")
        f.write("#pragma once\n\n")
        f.write("struct D2MOO_ResolveEntry { const char* name; const char* module; unsigned int rva; };\n\n")
        f.write("static const D2MOO_ResolveEntry g_d2moo_resolve_table[] = {\n")
        for name, mod, addr in entries:
            ghidra_base = next(lo for m, lo, _hi in MODULE_RANGES if m == mod)
            f.write('\t{ "%s", "%s", 0x%xu },\n' % (name, mod, addr - ghidra_base))
        f.write("};\n")
        f.write("static const int g_d2moo_resolve_count = %d;\n" % len(entries))

    print(f"Wrote {OUT} ({len(entries)} entries)")
    for mod, n in sorted(by_module.items(), key=lambda kv: -kv[1]):
        print(f"  {mod:<18} {n}")
    return 0


if __name__ == "__main__":
    if "--verify-ranges" in sys.argv:
        i = sys.argv.index("--verify-ranges")
        game_dir = sys.argv[i + 1] if len(sys.argv) > i + 1 else r"C:\Diablo2\ProjectD2"
        sys.exit(verify_ranges(game_dir))
    sys.exit(main() or 0)
