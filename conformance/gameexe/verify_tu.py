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

# D2 was built with VS2003 SP1 (build 6030), the toolchain at C:\VS2003.
# vcvars32.bat sets PATH/INCLUDE/LIB including SP1's OWN Platform SDK -- the
# era-accurate environment, not a VC6 header borrow (per C:\VS2003\_provenance
# \AGENT_PRIMER.md). FUNDOC_VCVARS overrides the toolchain.
VCVARS = Path(os.environ.get("FUNDOC_VCVARS", r"C:\VS2003\vcvars32.bat"))
_SENTINEL = "___VCVARS_ENV___"


def _toolchain_env() -> dict:
    """The compiler's own PATH/INCLUDE/LIB via its vcvars, cached per run."""
    out = subprocess.run(
        ["cmd", "/c", "call", str(VCVARS), "&&", "echo", _SENTINEL, "&&", "set"],
        capture_output=True, text=True)
    env = dict(os.environ)
    seen = False
    for line in out.stdout.splitlines():
        if line.strip() == _SENTINEL:
            seen = True
            continue
        if seen and "=" in line:
            k, v = line.split("=", 1)
            env[k] = v
    if "INCLUDE" not in env:
        raise SystemExit(f"vcvars did not set INCLUDE -- is {VCVARS} correct?\n"
                         + out.stdout[-500:] + out.stderr[-500:])
    # Resolve cl.exe on the vcvars PATH: Windows subprocess locates the exe via
    # the PARENT's PATH, not the child env's, so an unqualified "cl" is not
    # found even with the right env. Pin the absolute path here.
    for d in env.get("PATH", "").split(os.pathsep):
        cand = Path(d) / "cl.exe"
        if cand.exists():
            env["__CL__"] = str(cand)
            break
    else:
        raise SystemExit("cl.exe not on the vcvars PATH")
    return env
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


_ENV = None


def compile_tu(src: Path, obj: Path, extra_flags=None) -> str:
    global _ENV
    if _ENV is None:
        _ENV = _toolchain_env()
    if obj.exists():
        obj.unlink()
    cmd = [_ENV["__CL__"], "/nologo", "/c", "/O2",
           *(extra_flags or []), f"/Fo{obj}", str(src)]
    p = subprocess.run(cmd, capture_output=True, text=True, env=_ENV, cwd=HERE)
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
