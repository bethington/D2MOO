"""Splice a reimplemented function into a copy of Game.exe, in place.

    python patch_function.py --exe C:\\gxs\\ProjectD2\\Game.exe \\
        --obj build/parse.obj --symbol _TrimWhitespaceDelimiters@4 \\
        --address 0x004079d0 --out C:\\gxs\\ProjectD2\\Game_patched.exe

WHY PATCH RATHER THAN LINK. Verifying our code needs a runnable binary, and
linking a whole Game.exe needs all 18 launcher functions to exist first --
which would mean no verification at all until the very end, letting defects
pile up unnoticed. Patching one function into the original isolates it
completely: nothing else in the binary changed, so a divergence in the trace
names its cause.

IN PLACE, SAME-OR-SMALLER, BY CHOICE. Our function must fit the original's
extent; the remainder is filled with 0xCC (int3), the padding byte the
original linker itself uses between functions. Appending a section and
jumping to it would lift the size limit, but the goal here is byte-identical
reconstruction, so code that comes out LARGER than the original is itself
evidence the reconstruction is wrong -- and a limit that surfaces it beats a
mechanism that hides it.

RELOCATIONS ARE RESOLVED BY SYMBOL, NOT BY POSITION. An object's code carries
placeholder addresses the linker would fix up; patching skips the linker, so
the tool does it.

The tempting shortcut -- read whatever address the ORIGINAL had at the same
offset and copy it across -- works only while our code is byte-identical to
the original. The moment the layout differs by a single byte, every
relocation reads from the wrong place and the patched binary calls into
garbage. Since a function that is byte-identical needs no verifying, that
shortcut is wrong exactly when it matters.

So each relocation is resolved from the object's own relocation table: the
symbol it names is looked up in `--symbols` (a JSON map of name -> address,
which for this binary is just the callees and globals the function touches),
and the value written according to type -- IMAGE_REL_I386_DIR32 gets the
absolute address, REL32 gets `target - (our_va + offset + 4)`. An unknown
symbol is refused rather than guessed at.
"""
from __future__ import annotations

import argparse
import json
import os
import shutil
import struct
import sys
from pathlib import Path

FUN_DOC = Path(os.environ.get("FUNDOC_DIR",
                              r"c:\Users\benam\source\mcp\ghidra-mcp\fun-doc"))
sys.path.insert(0, str(FUN_DOC))
from crt_identify import coff_functions  # noqa: E402

PAD = 0xCC          # int3 -- what the VS2003 linker puts between functions
REL_DIR32 = 6       # IMAGE_REL_I386_DIR32  -- absolute address
REL_REL32 = 20      # IMAGE_REL_I386_REL32  -- displacement from next insn


def coff_relocations(blob: bytes, symbol: str):
    """[(offset_within_function, type, target_symbol_name)] for one function.

    crt_identify.coff_functions gives relocation OFFSETS only, which is all a
    byte-comparison needs (it just masks them). Patching needs to know WHAT
    each site refers to, so the relocation table is re-read here with its
    symbol names attached.
    """
    machine, nsec, _st, symptr, nsym, optsz, _ch = struct.unpack_from("<HHIIIHH", blob, 0)
    secs, off = [], 20 + optsz
    for i in range(nsec):
        s = blob[off + i * 40: off + (i + 1) * 40]
        rawsize, _rawptr, relptr, nrel = (
            struct.unpack_from("<I", s, 16)[0], struct.unpack_from("<I", s, 20)[0],
            struct.unpack_from("<I", s, 24)[0], struct.unpack_from("<H", s, 32)[0])
        secs.append((rawsize, relptr, nrel))
    strtab = blob[symptr + nsym * 18:]

    def symname(rec: bytes) -> str:
        if rec[0:4] == b"\0\0\0\0":
            o = struct.unpack_from("<I", rec, 4)[0]
            return strtab[o:strtab.find(b"\0", o)].decode("ascii", "replace")
        return rec[0:8].rstrip(b"\0").decode("ascii", "replace")

    names, i = {}, 0
    target, tsec = None, None
    while i < nsym:
        rec = blob[symptr + i * 18: symptr + (i + 1) * 18]
        value, secnum, _t, _c, naux = struct.unpack_from("<IhHBB", rec, 8)
        names[i] = symname(rec)
        if names[i] == symbol:
            target, tsec = value, secnum - 1
        i += 1 + naux
    if target is None or tsec is None or tsec >= len(secs):
        return []
    rawsize, relptr, nrel = secs[tsec]
    out = []
    for r in range(nrel):
        ro = relptr + r * 10
        va, symidx, rtype = struct.unpack_from("<IIH", blob, ro)
        if va >= target:
            out.append((va - target, rtype, names.get(symidx, f"<sym{symidx}>")))
    return out


def rva_to_offset(data: bytes, rva: int):
    pe = struct.unpack_from("<I", data, 0x3c)[0]
    nsec = struct.unpack_from("<H", data, pe + 6)[0]
    opt = pe + 24
    sec = opt + struct.unpack_from("<H", data, pe + 20)[0]
    for i in range(nsec):
        b = sec + 40 * i
        va = struct.unpack_from("<I", data, b + 12)[0]
        vsz = struct.unpack_from("<I", data, b + 8)[0]
        raw = struct.unpack_from("<I", data, b + 20)[0]
        rsz = struct.unpack_from("<I", data, b + 16)[0]
        if va <= rva < va + max(vsz, rsz):
            return raw + (rva - va)
    return None


def image_base(data: bytes) -> int:
    pe = struct.unpack_from("<I", data, 0x3c)[0]
    return struct.unpack_from("<I", data, pe + 24 + 28)[0]


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--exe", required=True, help="original Game.exe to copy from")
    ap.add_argument("--obj", required=True, help="COFF object holding our function")
    ap.add_argument("--symbol", required=True, help="decorated symbol, e.g. _F@4")
    ap.add_argument("--address", required=True, help="original VA, e.g. 0x004079d0")
    ap.add_argument("--extent", type=int,
                    help="original function length; defaults to our body's length")
    ap.add_argument("--symbols", help="JSON map of relocation symbol -> address")
    ap.add_argument("--out", required=True)
    ap.add_argument("--json", help="write a patch report here")
    args = ap.parse_args()

    data = bytearray(Path(args.exe).read_bytes())
    base = image_base(data)
    va = int(str(args.address), 16)
    off = rva_to_offset(data, va - base)
    if off is None:
        return print(f"!! {args.address} is not inside any section") or 1

    found = next(((b, tuple(r)) for n, b, r in
                  coff_functions(Path(args.obj).read_bytes())
                  if n == args.symbol), None)
    if not found:
        names = sorted(n for n, _, _ in coff_functions(Path(args.obj).read_bytes()))
        return print(f"!! symbol {args.symbol} not in {args.obj}\n"
                     f"   available: {', '.join(names[:30])}") or 1
    body, relocs = found
    extent = args.extent or len(body)
    if len(body) > extent:
        return print(
            f"!! our function is LARGER than the space available: "
            f"{len(body)} > {extent} bytes.\n"
            f"   In-place patching requires same-or-smaller. Code bigger than "
            f"the original is evidence the reconstruction is wrong -- read the "
            f"emitted code against the original before working around this."
        ) or 1

    original = bytes(data[off:off + extent])
    patched = bytearray(body)

    symbols = {}
    if args.symbols:
        symbols = {k: int(str(v), 16) if isinstance(v, str) else v
                   for k, v in json.loads(Path(args.symbols).read_text()).items()}

    sites = coff_relocations(Path(args.obj).read_bytes(), args.symbol)
    resolved, unknown = [], []
    for r_off, rtype, name in sites:
        if r_off + 4 > len(patched):
            return print(f"!! relocation at +0x{r_off:x} lies past the end of "
                         f"our function -- refusing to guess") or 1
        if name not in symbols:
            unknown.append(f"{name} (+0x{r_off:x}, type {rtype})")
            continue
        tgt = symbols[name]
        if rtype == REL_DIR32:
            value = tgt
        elif rtype == REL_REL32:
            value = (tgt - (va + r_off + 4)) & 0xFFFFFFFF
        else:
            return print(f"!! relocation type {rtype} at +0x{r_off:x} "
                         f"({name}) is not handled") or 1
        struct.pack_into("<I", patched, r_off, value)
        resolved.append({"offset": r_off, "symbol": name,
                         "type": "DIR32" if rtype == REL_DIR32 else "REL32",
                         "value": f"0x{value:08x}"})
    if unknown:
        return print(
            "!! unresolved relocation symbol(s) -- refusing to patch:\n   "
            + "\n   ".join(unknown)
            + f"\n   Add them to --symbols as name -> address. Guessing here "
              f"would produce a binary that calls into garbage."
        ) or 1

    patched.extend(bytes([PAD]) * (extent - len(patched)))
    identical = bytes(patched) == original
    data[off:off + extent] = patched

    out = Path(args.out)
    if out.resolve() == Path(args.exe).resolve():
        return print("!! refusing to overwrite the source exe; use a new --out") or 1
    shutil.copy2(args.exe, out)
    out.write_bytes(bytes(data))

    report = {
        "exe": str(args.exe), "out": str(out), "symbol": args.symbol,
        "address": args.address, "file_offset": f"0x{off:x}",
        "our_size": len(body), "extent": extent,
        "padding": extent - len(body), "relocations": resolved, "symbols": args.symbols,
        "identical_to_original": identical,
    }
    print(f"  patched {args.symbol} @ {args.address} (file +0x{off:x})")
    print(f"  {len(body)} bytes of ours, {extent - len(body)} bytes of 0xCC padding, "
          f"{len(relocs)} relocation(s) resolved")
    print(f"  bytes identical to original: {identical}"
          + ("   <- patch is a no-op; this validates the PATCHER, not the function"
             if identical else ""))
    print(f"  wrote {out}")
    if args.json:
        Path(args.json).write_text(json.dumps(report, indent=2), encoding="utf-8")
    return 0


if __name__ == "__main__":
    sys.exit(main())
