"""Verify build_import_libs.ORDINALS against the ORIGINAL Game.exe.

    python link/verify_ordinals.py            # check
    python link/verify_ordinals.py --list     # also dump what the original imports

Why this exists. The D2 DLLs export BY ORDINAL ONLY, so nothing in our build can
tell us a name-to-ordinal mapping is right -- a wrong ordinal LINKS FINE and then
calls a different function. We got that wrong once by trusting D2MOO's
`*.1.13c.def` name tables, which do not describe this binary (file version
1.0.13.60): the resulting import set was disjoint from the original's on every
DLL, and because static imports run DllMain at load, it corrupted D2Win/D2Lang
init before a line of our code ran.

The check is a SET comparison against ground truth: the exact multiset of
(dll, ordinal) pairs the original Game.exe imports must equal the set our
ORDINALS table would produce, ignoring ordinals the original reaches through
code we have not reconstructed yet (RECONSTRUCTED_ONLY below). It does not try
to re-derive the name of each ordinal -- names are our labels, the ordinals are
the fact.
"""
import struct, sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
from build_import_libs import ORDINALS, DLL_FILE          # noqa: E402

ORIGINAL = Path(r"C:\pd2\ProjectD2\Game.exe")

# Ordinals the original imports that our reconstruction does not call yet.
# Keep this list SHORT and explain each one -- it is the honest record of what
# is still unreconstructed, not a place to silence a mismatch.
RECONSTRUCTED_ONLY = {
    # (dll_file, ordinal): why we don't import it yet
}


def imports(path):
    d = path.read_bytes()
    pe = struct.unpack_from("<I", d, 0x3c)[0]
    base = struct.unpack_from("<I", d, pe + 0x34)[0]
    nsec = struct.unpack_from("<H", d, pe + 6)[0]
    opt = struct.unpack_from("<H", d, pe + 0x14)[0]
    secs = []
    for i in range(nsec):
        o = pe + 0x18 + opt + i * 40
        vsz, va, rsz, raw = struct.unpack_from("<IIII", d, o + 8)
        secs.append((va, vsz, raw, rsz))

    def r2o(rva):
        for va, vsz, raw, rsz in secs:
            if va <= rva < va + max(vsz, rsz):
                return raw + (rva - va)
        return None

    def rdstr(rva):
        o = r2o(rva)
        return d[o:d.index(b"\0", o)].decode("latin1")

    magic = struct.unpack_from("<H", d, pe + 0x18)[0]
    ddoff = pe + 0x18 + (0x60 if magic == 0x10b else 0x70)
    imp_rva, _ = struct.unpack_from("<II", d, ddoff + 8)
    out = set()
    o = r2o(imp_rva)
    while True:
        olt, _ts, _fc, namerva, first = struct.unpack_from("<IIIII", d, o)
        if namerva == 0:
            break
        dll = rdstr(namerva).lower()
        ao = r2o(olt or first)
        i = 0
        while True:
            v, = struct.unpack_from("<I", d, ao + i * 4)
            if v == 0:
                break
            if v & 0x80000000:
                out.add((dll, v & 0xffff))
            i += 1
        o += 20
    return out


def main():
    if not ORIGINAL.exists():
        print(f"!! original not found: {ORIGINAL}")
        return 2
    have = imports(ORIGINAL)
    d2 = {(dll, o) for dll, o in have
          if not dll.startswith(("kernel32", "user32", "advapi32"))}
    ours = {(DLL_FILE[dll].lower(), ordv) for dll, ordv in ORDINALS.values()}
    skip = {(f.lower(), o) for f, o in RECONSTRUCTED_ONLY}

    wrong = ours - d2                       # we import something the original never does
    todo = d2 - ours - skip                 # original imports it, we don't yet

    if "--list" in sys.argv:
        print("=== ordinals the ORIGINAL imports (non-Win32) ===")
        for dll, o in sorted(d2):
            mark = "" if (dll, o) in ours else "   <- not reconstructed"
            print(f"   {dll} @{o}{mark}")
        print()

    ok = True
    if wrong:
        ok = False
        print("!! WE IMPORT ORDINALS THE ORIGINAL DOES NOT -- these call the wrong function:")
        for dll, o in sorted(wrong):
            names = [n for n, (dd, oo) in ORDINALS.items()
                     if DLL_FILE[dd].lower() == dll and oo == o]
            print(f"   {dll} @{o}   (ours: {', '.join(names)})")
    if todo:
        print(f"-- {len(todo)} ordinal(s) the original imports are not reconstructed yet:")
        for dll, o in sorted(todo):
            print(f"   {dll} @{o}")
    if ok:
        print(f"OK: all {len(ours)} imported ordinals are ones the original imports too.")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
