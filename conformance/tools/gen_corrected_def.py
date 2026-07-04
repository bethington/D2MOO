"""Emit a trustworthy .def from a corrected_maps TSV.

For binaries where Ghidra's names are verified-correct but the D2MOO .def ordinals
are scrambled (D2Common, D2Game), rebuild the .def as `ghidra_name @ordinal` using
the REAL PE ordinal for each function. Duplicate Ghidra names (GetLastError,
StubNoOp, …) are disambiguated with an ordinal suffix so the export table stays
valid. Purely additive names — the ordinal is what the drop-in resolves by.

Usage: python gen_corrected_def.py <LIBRARYNAME> <corrected_maps/X.tsv> <out.def>
"""
import sys, re

libname, tsv, outp = sys.argv[1:4]
rows = []
with open(tsv, encoding="utf-8") as f:
    next(f)  # header
    for line in f:
        p = line.rstrip("\n").split("\t")
        if len(p) < 3:
            continue
        ordinal, addr, gname = int(p[0]), p[1], p[2]
        rows.append((ordinal, addr, gname))

# Sanitize + collect name frequency for disambiguation.
def sane(n):
    n = re.sub(r"[^A-Za-z0-9_]", "_", n or "")
    if not n or n[0].isdigit():
        n = "_" + n
    return n

counts = {}
for _, _, g in rows:
    counts[sane(g)] = counts.get(sane(g), 0) + 1

rows.sort(key=lambda r: r[0])
lines, used = [], set()
dupes = 0
for ordinal, addr, gname in rows:
    nm = sane(gname)
    if counts[nm] > 1 or nm in used:      # disambiguate any collision by ordinal
        nm = f"{nm}_{ordinal}"
        dupes += 1
    used.add(nm)
    lines.append(f"\t{nm:<52} @{ordinal}")

with open(outp, "w", encoding="utf-8") as f:
    f.write(f"; Auto-generated from {tsv.split('/')[-1]} — REAL PD2-S12 export ordinals\n")
    f.write("; (ghidra_name @ real-ordinal). Supersedes the scrambled D2MOO .def.\n")
    f.write(f"LIBRARY\t{libname}\nEXPORTS\n")
    f.write("\n".join(lines) + "\n")

print(f"{libname}: {len(rows)} exports written to {outp} ({dupes} names ordinal-suffixed)")
