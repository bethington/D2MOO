"""POSITIVE CONTROL for the byte-match verification chain.

Question: does the whole chain — read the original function's bytes out of
Game.exe, parse a COFF object, mask relocation sites on both sides, compare —
actually prove identity when identity is known to hold?

Method: Game.exe statically links the VS2003 CRT (Rich header: 89 Utc1310_C
objects, build 6030). So for every FID-identified CRT function in Game.exe
there is a corresponding object in the vendored VS2003 libcmt.lib whose body
MUST be byte-identical modulo relocations. Any mismatch is a harness defect,
not a code difference.

Negative control in the same run: compare against VC6's LIBCMT (the WRONG
toolchain). A harness that "matches" both proves nothing.
"""
import json
import sys
import urllib.parse
import urllib.request
from collections import defaultdict
from pathlib import Path

FUN_DOC = Path(r"c:\Users\benam\source\mcp\ghidra-mcp\fun-doc")
sys.path.insert(0, str(FUN_DOC))
from crt_identify import ar_members, coff_functions, mask_bytes  # noqa: E402

PROG = "/Mods/PD2-S12/Game.exe"
VS2003_LIB = FUN_DOC / "benchmark/tools/vc6/VS7/Lib/libcmt.lib"
VC6_LIB = FUN_DOC / "benchmark/tools/vc6/VC98/LIB/LIBCMT.LIB"


def ghidra(path, **params):
    params["program"] = PROG
    url = f"http://127.0.0.1:8089/{path}?" + urllib.parse.urlencode(params)
    with urllib.request.urlopen(url, timeout=60) as r:
        return json.loads(r.read().decode())


def read_bytes(addr: int, length: int) -> bytes:
    return bytes.fromhex(
        ghidra("read_memory", address=f"0x{addr:08x}", length=length)["hex"]
    )


def index_lib(lib_path: Path) -> dict:
    """symbol -> list of (body, relocs) across all objects in the archive."""
    idx = defaultdict(list)
    data = lib_path.read_bytes()
    for _member, blob in ar_members(data):
        try:
            for name, body, relocs in coff_functions(blob):
                idx[name].append((body, tuple(relocs)))
        except Exception:  # noqa: BLE001 - malformed member, skip
            continue
    return idx


def compare(read_at, addr: int, cands) -> tuple:
    """Best verdict across candidate (body, relocs) pairs.

    The LIBRARY body's length defines the comparison window — exactly how
    crt_identify does it. Guessing the original's extent from a disassembly
    listing would make instruction-length errors look like code differences.
    """
    best = ("NO_CANDIDATE", None)
    for body, relocs in cands:
        orig = read_at(addr, len(body))
        if len(orig) != len(body):
            continue
        mb, mo = mask_bytes(body, relocs), mask_bytes(orig, relocs)
        informative = len(body) - 4 * len(relocs)
        if mb == mo:
            return ("MATCH", f"{len(body)}B, {informative} informative, {len(relocs)} relocs")
        ndiff = sum(1 for a, b in zip(mb, mo) if a != b)
        if best[0] in ("NO_CANDIDATE", "DIFF") and (
            best[1] is None or ndiff < int(best[1].split("/")[0])
        ):
            best = ("DIFF", f"{ndiff}/{len(body)} differ")
    return best


def main():
    # FID-identified CRT functions in Game.exe (from the Function ID bookmarks)
    bookmarks = ghidra("list_bookmarks", limit=10000)["bookmarks"]
    fid = []
    for b in bookmarks:
        if b.get("category") == "Function ID Analyzer" and "Single Match" in b.get("comment", ""):
            sym = b["comment"].split("Single Match,")[-1].strip()
            fid.append((b["address"], sym))
    print(f"{len(fid)} FID-identified CRT functions in Game.exe\n")

    print("indexing libraries ...")
    vs = index_lib(VS2003_LIB)
    vc6 = index_lib(VC6_LIB)
    print(f"  VS2003 libcmt.lib: {len(vs)} symbols")
    print(f"  VC6    LIBCMT.LIB: {len(vc6)} symbols\n")

    stats = {"vs": defaultdict(int), "vc6": defaultdict(int)}
    rows = []
    cache = {}

    def read_at(addr, n):
        key = (addr, n)
        if key not in cache:
            try:
                cache[key] = read_bytes(addr, n)
            except Exception:  # noqa: BLE001
                cache[key] = b""
        return cache[key]

    for addr_hex, sym in fid:
        addr = int(addr_hex, 16)
        v_verdict, v_detail = compare(read_at, addr, vs.get(sym, []))
        c_verdict, c_detail = compare(read_at, addr, vc6.get(sym, []))
        stats["vs"][v_verdict] += 1
        stats["vc6"][c_verdict] += 1
        rows.append((addr_hex, sym, v_verdict, v_detail, c_verdict, c_detail))

    print(f"{'addr':10} {'symbol':32} {'VS2003':12} {'VC6':12} detail")
    for addr, sym, vv, vd, cv, cd in rows:
        mark = " <<<" if vv == "MATCH" and cv != "MATCH" else ""
        print(f"{addr:10} {sym[:32]:32} {vv:12} {cv:12} {vd or cd or ''}{mark}")

    print("\n=== TALLY ===")
    print("  VS2003 (correct toolchain):", dict(stats["vs"]))
    print("  VC6    (negative control): ", dict(stats["vc6"]))
    vsm, vcm = stats["vs"]["MATCH"], stats["vc6"]["MATCH"]
    print(f"\n  byte-identical: VS2003 {vsm}  |  VC6 {vcm}  of {len(rows)} compared")
    return 0 if vsm else 1


if __name__ == "__main__":
    sys.exit(main())
