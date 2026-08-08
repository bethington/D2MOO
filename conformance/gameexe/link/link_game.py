"""Link the reconstructed launcher into a running Game.exe.

    python link_game.py

Compiles src/Main.c + link/winmain_shim.c with the VS2003 SP1 toolchain (D2's
exact compiler), links against D2MOO's already-built import libraries for the
six D2 DLLs (Fog/Storm/D2Win/D2Gfx/D2Sound/D2MCPClient -- these map our symbols
to each DLL's 1.13c ordinals) plus the Win32 libs and the static CRT, at image
base 0x400000. Output: build/Game.recon.exe.

Run it in the PD2 folder and it imports PD2's real DLLs by ordinal.
"""
import os, subprocess, sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
GAMEEXE = HERE.parent
VCVARS = Path(os.environ.get("FUNDOC_VCVARS", r"C:\VS2003\vcvars32.bat"))
IMPLIBS = HERE / "implibs"    # our own C-decorated import libs (build_import_libs.py)
IMPORT_LIB_NAMES = ["Fog", "Storm", "D2Win", "D2Gfx", "D2Sound", "D2MCPClient"]


def find_lib(name):
    p = IMPLIBS / f"{name}.lib"
    return p if p.exists() else None
_SENT = "___ENV___"


def env():
    out = subprocess.run(["cmd", "/c", "call", str(VCVARS), "&&", "echo", _SENT, "&&", "set"],
                         capture_output=True, text=True)
    e = dict(os.environ)
    seen = False
    for line in out.stdout.splitlines():
        if line.strip() == _SENT:
            seen = True; continue
        if seen and "=" in line:
            k, v = line.split("=", 1); e[k] = v
    for d in e.get("PATH", "").split(os.pathsep):
        for tool in ("cl.exe", "link.exe"):
            p = Path(d) / tool
            if p.exists():
                e["__" + tool[:-4].upper() + "__"] = str(p)
    return e


def main():
    e = env()
    build = GAMEEXE / "build"
    build.mkdir(exist_ok=True)
    srcs = [GAMEEXE / "src" / "Main.c", HERE / "winmain_shim.c"]
    objs = []
    for s in srcs:
        obj = build / (s.stem + ".obj")
        if obj.exists():
            obj.unlink()
        cmd = [e["__CL__"], "/nologo", "/c", "/O2", "/MT", f"/Fo{obj}", str(s)]
        p = subprocess.run(cmd, capture_output=True, text=True, env=e, cwd=GAMEEXE)
        if not obj.exists():
            print(f"COMPILE FAILED ({s.name}):\n" + p.stdout + p.stderr)
            return 1
        objs.append(str(obj))

    libs = []
    for name in IMPORT_LIB_NAMES:
        p = find_lib(name)
        if p is None:
            print(f"!! no built import lib for {name} under {D2MOO_BUILD}")
            return 1
        libs.append(str(p))

    out = build / "Game.recon.exe"
    if out.exists():
        out.unlink()
    # Drive the linker through cl.exe so it locates its own link.exe (a bare
    # "link" resolves to Git Bash's /usr/bin/link otherwise).
    cmd = [e["__CL__"], "/nologo", "/MT", *objs, *libs,
           "kernel32.lib", "user32.lib", "advapi32.lib",
           f"/Fe{out}", "/link", "/subsystem:windows", "/machine:x86",
           "/base:0x400000"]
    p = subprocess.run(cmd, capture_output=True, text=True, env=e, cwd=GAMEEXE)
    print(p.stdout[-4000:])
    if p.stderr:
        print("STDERR:\n" + p.stderr[-4000:])
    if out.exists():
        print(f"\nOK -> {out} ({out.stat().st_size} bytes)")
        return 0
    print("\nLINK FAILED")
    return 1


if __name__ == "__main__":
    sys.exit(main())
