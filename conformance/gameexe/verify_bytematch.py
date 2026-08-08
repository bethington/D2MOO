"""CONF_BYTEMATCH verifier: is a reimplemented function byte-identical to the
original, modulo relocation sites?

A byte-identical function IS the original function — no runtime observation
required. That is the whole point for Game.exe, whose authored functions run
once at startup (or, on error paths, never), so no amount of live shadow
dispatch can accumulate confidence the way it does for the DLLs.

    original bytes  ← Ghidra /read_memory on the reference program
    reimpl bytes    ← the COFF object our era-toolchain build produced
    both masked at the OBJECT's relocation offsets, then compared

Masking matters and must use the OBJECT's relocations, not the PE's .reloc:
.reloc lists only DIR32 base-relocations, so every `call rel32` displacement
would stay unmasked and every function containing a call would fail. This is
the same lesson (and the same primitives) as fun-doc/crt_identify.py, which
this module deliberately reuses rather than reimplements.

VALIDATED 2026-08-08 on PD2-S12 Game.exe:
  * positive control — 103/103 FID-identified CRT functions byte-identical
    to the vendored VS2003 libcmt.lib (see verify_bytematch_control.py)
  * negative control — the same comparison against VC6's LIBCMT matches
    11/103 (10.7%), all tiny SEH/mbcs helpers shared by both toolchains
  * end-to-end   — hand-written C for DATATBLS_FindConfigOptionIndex
    (0x004078e0) compiled by VS2003 cl /O2 matched exactly: 92 bytes,
    1 reloc, 88/88 informative bytes

Usage:
    python verify_bytematch.py --obj build/main.obj --symbol _GetCmdIndex@4 \\
        --address 0x004078e0
    python verify_bytematch.py --manifest manifest.json      # batch
"""
from __future__ import annotations

import argparse
import json
import os
import sys
import urllib.parse
import urllib.request
from pathlib import Path

# crt_identify owns the COFF parsing + masking primitives; import, never copy.
FUN_DOC = Path(
    os.environ.get("FUNDOC_DIR", r"c:\Users\benam\source\mcp\ghidra-mcp\fun-doc")
)
sys.path.insert(0, str(FUN_DOC))
from crt_identify import coff_functions, mask_bytes  # noqa: E402

GHIDRA = os.environ.get("GHIDRA_MCP_URL", "http://127.0.0.1:8089")
DEFAULT_PROGRAM = "/Mods/PD2-S12/Game.exe"

# A match over too few surviving bytes is not evidence — the import-thunk
# lesson from crt_identify (ff 25 <addr> leaves two informative bytes, and
# every thunk then "matches" every other). CONF_BYTEMATCH is a terminal
# verdict, so it needs the strong floor, not the indexing one.
STRONG_INFORMATIVE_BYTES = 20


def read_original(addr: int, length: int, program: str) -> bytes:
    q = urllib.parse.urlencode(
        {"address": f"0x{addr:08x}", "length": length, "program": program}
    )
    with urllib.request.urlopen(f"{GHIDRA}/read_memory?{q}", timeout=30) as r:
        return bytes.fromhex(json.loads(r.read().decode())["hex"])


def obj_function(obj_path: Path, symbol: str):
    """(body, relocs) for `symbol` in a COFF object, or None."""
    for name, body, relocs in coff_functions(obj_path.read_bytes()):
        if name == symbol:
            return body, tuple(relocs)
    return None


def verify(obj_path: Path, symbol: str, address: int, program: str) -> dict:
    """Compare one function. Returns a verdict dict; never raises on mismatch."""
    found = obj_function(obj_path, symbol)
    if not found:
        return {
            "verdict": "SYMBOL_NOT_FOUND",
            "symbol": symbol,
            "available": sorted(n for n, _, _ in coff_functions(obj_path.read_bytes()))[:40],
        }
    body, relocs = found
    orig = read_original(address, len(body), program)
    informative = len(body) - 4 * len(relocs)
    out = {
        "symbol": symbol,
        "address": f"0x{address:08x}",
        "size": len(body),
        "relocs": len(relocs),
        "informative": informative,
    }
    if len(orig) != len(body):
        return {**out, "verdict": "SHORT_READ", "read": len(orig)}
    masked_ours, masked_orig = mask_bytes(body, relocs), mask_bytes(orig, relocs)
    if masked_ours != masked_orig:
        diffs = [i for i, (a, b) in enumerate(zip(masked_ours, masked_orig)) if a != b]
        return {**out, "verdict": "DIFF", "diff_count": len(diffs),
                "first_diffs": diffs[:12]}
    if informative < STRONG_INFORMATIVE_BYTES:
        # Identical, but over too little surviving evidence to certify.
        return {**out, "verdict": "WEAK",
                "note": f"only {informative} informative bytes "
                        f"(< {STRONG_INFORMATIVE_BYTES}); identity not certified"}
    return {**out, "verdict": "BYTEMATCH"}


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--obj", help="COFF object produced by the era build")
    ap.add_argument("--symbol", help="decorated symbol, e.g. _GetCmdIndex@4")
    ap.add_argument("--address", help="original function address, e.g. 0x004078e0")
    ap.add_argument("--manifest", help="JSON list of {obj, symbol, address} to batch")
    ap.add_argument("--program", default=DEFAULT_PROGRAM)
    ap.add_argument("--json", help="write the full report here")
    args = ap.parse_args()

    if args.manifest:
        entries = json.loads(Path(args.manifest).read_text(encoding="utf-8"))
    elif args.obj and args.symbol and args.address:
        entries = [{"obj": args.obj, "symbol": args.symbol, "address": args.address}]
    else:
        ap.error("need --manifest, or all of --obj/--symbol/--address")

    results = []
    for e in entries:
        r = verify(Path(e["obj"]), e["symbol"], int(str(e["address"]), 16), args.program)
        results.append(r)
        mark = " <<<" if r["verdict"] == "BYTEMATCH" else ""
        print(f"{r.get('address','-'):12} {r['symbol'][:38]:38} {r['verdict']}{mark}")
        if r["verdict"] == "DIFF":
            print(f"      {r['diff_count']} byte(s) differ at {r['first_diffs']}")

    n_ok = sum(1 for r in results if r["verdict"] == "BYTEMATCH")
    print(f"\nCONF_BYTEMATCH: {n_ok}/{len(results)}")
    if args.json:
        Path(args.json).write_text(json.dumps(results, indent=2), encoding="utf-8")
        print(f"wrote {args.json}")
    return 0 if n_ok == len(results) else 1


if __name__ == "__main__":
    sys.exit(main())
