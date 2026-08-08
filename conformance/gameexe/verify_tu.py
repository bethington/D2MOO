"""Compile the launcher translation unit and byte-check every function.

    python verify_tu.py                 # compile src/Main.c, score against
                                        # launcher_manifest.json

The launcher is one object file (0x407550..0x408540, 22 functions, ~4 KB),
so it is reconstructed as one src/Main.c and compiled as one TU -- that is
what lets MSVC reproduce the private register conventions on its own (see
README). This tool is the primary verification loop: it is entirely static,
needs no linking, no tracing and no game.

For every function in the manifest it finds the matching compiled symbol
(by undecorated name), masks relocation sites on both sides, and reports
MATCH / DIFF(n) / LARGER / MISSING. A function is DONE when it MATCHes.
"""
from __future__ import annotations

import json
import os
import re
import subprocess
import sys
import urllib.parse
import urllib.request
from pathlib import Path

HERE = Path(__file__).resolve().parent
FUN_DOC = Path(os.environ.get(
    "FUNDOC_DIR", r"c:\Users\benam\source\mcp\ghidra-mcp\fun-doc"))
sys.path.insert(0, str(FUN_DOC))
from crt_identify import coff_functions, mask_bytes  # noqa: E402

CL = FUN_DOC / "benchmark/tools/vc6/VS7/Bin/cl.exe"
VC6_INC = FUN_DOC / "benchmark/tools/vc6/VC98/Include"      # Win32 headers
VS7_INC = FUN_DOC / "benchmark/tools/vc6/VS7/Include"
PROG = "/Mods/PD2-S12/Game.exe"
GHIDRA = "http://127.0.0.1:8089"


def original(addr: int, length: int) -> bytes:
    q = urllib.parse.urlencode(
        {"address": f"0x{addr:08x}", "length": length, "program": PROG})
    with urllib.request.urlopen(f"{GHIDRA}/read_memory?{q}", timeout=30) as r:
        return bytes.fromhex(json.loads(r.read().decode())["hex"])


def undecorate(sym: str) -> str:
    """_Foo@16 -> Foo, @Foo@8 -> Foo, _Foo -> Foo."""
    s = sym
    if s and s[0] in "_@":
        s = s[1:]
    return re.sub(r"@\d+$", "", s)


def compile_tu(src: Path, obj: Path, extra_flags=None) -> str:
    if obj.exists():
        obj.unlink()
    env = dict(os.environ)
    env["PATH"] = str(CL.parent) + os.pathsep + env.get("PATH", "")
    cmd = [str(CL), "/nologo", "/c", "/O2",
           f"/I{VS7_INC}", f"/I{VC6_INC}",
           *(extra_flags or []), f"/Fo{obj}", str(src)]
    p = subprocess.run(cmd, capture_output=True, text=True, env=env, cwd=HERE)
    if not obj.exists():
        return p.stdout + p.stderr
    return ""


def main() -> int:
    manifest = json.loads((HERE / "launcher_manifest.json").read_text())
    src = HERE / "src" / "Main.c"
    if not src.exists():
        return print(f"!! {src} does not exist yet") or 1
    obj = HERE / "build" / "Main.obj"
    obj.parent.mkdir(exist_ok=True)

    err = compile_tu(src, obj)
    if err:
        print("COMPILE FAILED:\n" + err[-2000:])
        return 1

    compiled = {}
    for name, body, relocs in coff_functions(obj.read_bytes()):
        compiled.setdefault(undecorate(name), (body, tuple(relocs)))

    done = 0
    print(f"{'address':11} {'orig':>5} {'ours':>5}  status   name")
    for m in manifest:
        addr = int(m["address"], 16)
        want = m["extent"]
        got = compiled.get(m["name"])
        if not got:
            print(f"{m['address']:11} {want:5} {'':>5}  --       {m['name']}")
            continue
        body, relocs = got
        orig = original(addr, want)
        if len(body) > want:
            status = f"LARGER +{len(body)-want}"
        elif len(body) < want:
            status = f"short -{want-len(body)}"
        else:
            mb, mo = mask_bytes(body, relocs), mask_bytes(orig, relocs)
            nd = sum(1 for a, b in zip(mb, mo) if a != b)
            status = "MATCH   " if nd == 0 else f"DIFF {nd}"
            if nd == 0:
                done += 1
        print(f"{m['address']:11} {want:5} {len(body):5}  {status:8} {m['name']}")

    print(f"\n  byte-matched: {done}/{len(manifest)} functions")
    return 0 if done == len(manifest) else 1


if __name__ == "__main__":
    sys.exit(main())
