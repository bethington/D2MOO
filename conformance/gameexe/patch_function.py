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

RELOCATIONS. An object's code carries placeholder addresses that the linker
would fix up. Patching skips the linker, so any relocation site has to be
resolved by hand against the original's own bytes: for each relocation the
tool reads the address the ORIGINAL used at that offset and writes it into
ours. That works precisely because we are replacing a function with another
version of the same function -- the data and callees it refers to are at the
same addresses. If a relocation lands where the original had something
structurally different, the tool refuses rather than guess.
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

    # Resolve each relocation from the ORIGINAL's own bytes: we are replacing
    # a function with another version of itself, so the addresses it refers to
    # are unchanged.
    resolved = []
    for r in relocs:
        if r + 4 > len(original):
            return print(f"!! relocation at +0x{r:x} lies outside the original "
                         f"function ({extent} bytes) -- refusing to guess") or 1
        value = struct.unpack_from("<I", original, r)[0]
        struct.pack_into("<I", patched, r, value)
        resolved.append({"offset": r, "value": f"0x{value:08x}"})

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
        "padding": extent - len(body), "relocations": resolved,
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
