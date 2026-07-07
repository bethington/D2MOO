"""build_profiler_tables.py -- Phase 1 of ../DYNAMIC_PROFILER_PLAN.md.

Emits profiler/<binary>.functions.tsv: the address/name/category table the
D2Debugger dynamic profiler reads at startup to mass-hook functions for
count-only profiling.

Phase 1 source = corrected_maps/*.tsv (EXPORTS only; ordinal->address->
ghidra_name, ground truth). Later phases add Ghidra's full internal-function
lists per binary; this script's output format is the same so the profiler
doesn't change.

Output columns (tab-separated, one header line):
    pref_addr   pref_base   ordinal   category   name
- pref_addr : absolute address at the DLL's PREFERRED base (from corrected_maps).
- pref_base : the DLL's preferred image base, so the profiler can rebase to the
              runtime module base (addr - pref_base + runtime_base).
- ordinal   : export ordinal, or -1 for (future) non-exported internals.
- category  : subsystem = the name prefix before the first '_' (DRLG, ITEMS,
              DUNGEON, ...), or "Unnamed" for FUN_/blank, or "Misc".
- name      : ghidra_name (the trustworthy one for D2Common/D2Game per
              ORDINAL_RECONCILIATION.md).

Run: python build_profiler_tables.py            # all corrected_maps
     python build_profiler_tables.py D2Common   # one binary
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
CONFORMANCE = os.path.dirname(HERE)
MAPS_DIR = os.path.join(CONFORMANCE, "corrected_maps")
OUT_DIR = os.path.join(CONFORMANCE, "profiler")

# DLL preferred image bases (from the PE headers / conformance dumps).
PREFERRED_BASE = {
    "D2Common": 0x6FD50000,
    "D2Game": 0x6FC20000,
    "D2Client": 0x6FAB0000,
    "D2Win": 0x6F8E0000,
    "Storm": 0x6FBF0000,
    "Fog": 0x6FF50000,
    "D2CMP": 0x6FE10000,
    "D2Lang": 0x6FC10000,
    "D2Net": 0x6FA90000,
    "D2Sound": 0x6F9B0000,
    "D2MCPClient": 0x6FA20000,
    "D2Gfx": 0x6FA80000,
}


def categorize(name):
    """Subsystem = prefix before the first underscore, when it looks like one."""
    if not name:
        return "Unnamed"
    if name.startswith(("FUN_", "SUB_", "Ordinal_", "lab_", "switchD")):
        return "Unnamed"
    m = re.match(r"^([A-Z][A-Z0-9]{1,})_", name)
    if m:
        return m.group(1)
    # No SUBSYSTEM_ prefix (e.g. camelCase analyst names like
    # GetDrlgDataRecordOffset) -> one "Misc" bucket rather than a singleton
    # category per name, so the tree stays navigable.
    return "Misc"


def build_one(binary):
    tsv = os.path.join(MAPS_DIR, f"{binary}.tsv")
    if not os.path.exists(tsv):
        print(f"  skip {binary}: no {tsv}")
        return 0
    base = PREFERRED_BASE.get(binary)
    if base is None:
        print(f"  skip {binary}: no preferred base known")
        return 0

    rows = []
    cats = {}
    with open(tsv, encoding="utf-8") as f:
        next(f)  # header
        for line in f:
            p = line.rstrip("\n").split("\t")
            if len(p) < 3:
                continue
            ordinal, addr, ghidra_name = p[0], p[1], p[2]
            try:
                addr_i = int(addr, 16)
                ord_i = int(ordinal)
            except ValueError:
                continue
            cat = categorize(ghidra_name)
            cats[cat] = cats.get(cat, 0) + 1
            rows.append((addr_i, base, ord_i, cat, ghidra_name or f"Ordinal_{ord_i}"))

    os.makedirs(OUT_DIR, exist_ok=True)
    outp = os.path.join(OUT_DIR, f"{binary}.functions.tsv")
    with open(outp, "w", encoding="utf-8") as f:
        f.write("pref_addr\tpref_base\tordinal\tcategory\tname\n")
        for addr_i, b, ord_i, cat, name in sorted(rows):
            f.write(f"0x{addr_i:x}\t0x{b:x}\t{ord_i}\t{cat}\t{name}\n")

    top = sorted(cats.items(), key=lambda kv: -kv[1])[:12]
    print(f"  {binary}: {len(rows)} functions -> {outp}")
    print(f"    top subsystems: " + ", ".join(f"{c}({n})" for c, n in top))
    return len(rows)


def main():
    targets = sys.argv[1:] or [
        f[:-4] for f in os.listdir(MAPS_DIR) if f.endswith(".tsv")
    ]
    total = 0
    for b in targets:
        total += build_one(b)
    print(f"Total: {total} functions across {len(targets)} binary table(s)")


if __name__ == "__main__":
    main()
