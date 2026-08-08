"""Build import libraries mapping our C-decorated D2-DLL symbols to ordinals.

D2MOO's own .libs are C++-mangled, so they don't satisfy our C object's
references (@FOG_MPQSetConfig@8 etc.). But D2MOO's .def files carry the 1.13c
ordinals. We take each undefined D2-DLL symbol in Main.obj, undecorate it, look
up its ordinal in the matching .def, and emit a .def of
  <our exact decorated name> = <import name> @<ordinal> NONAME
then run lib.exe to produce an import lib. Linking those, our fastcall/stdcall
call sites import PD2's real DLLs by the correct ordinal.
"""
import os, re, subprocess, sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
GAMEEXE = HERE.parent
D2MOO = Path(r"C:\Users\benam\source\cpp\D2MOO")
VCVARS = Path(os.environ.get("FUNDOC_VCVARS", r"C:\VS2003\vcvars32.bat"))
OBJ = GAMEEXE / "build" / "Main.obj"
OUT = HERE / "implibs"
DEFS = {
    "Fog":         D2MOO / "source/Fog/definitions/Fog.1.13c.def",
    "Storm":       D2MOO / "source/Storm/definitions/Storm.1.13c.def",
    "D2Win":       D2MOO / "source/D2Win/definitions/D2Win.1.13c.def",
    "D2Gfx":       D2MOO / "source/D2Gfx/definitions/D2Gfx.1.13c.def",
    "D2Sound":     D2MOO / "source/D2Sound/definitions/D2Sound.1.13c.def",
    "D2MCPClient": D2MOO / "source/D2MCPClient/definitions/D2MCPClient.1.13c.def",
}
DLL_FILE = {"Fog": "Fog.dll", "Storm": "Storm.dll", "D2Win": "D2Win.dll",
            "D2Gfx": "D2gfx.dll", "D2Sound": "D2sound.dll",
            "D2MCPClient": "D2MCPClient.dll"}
_SENT = "___ENV___"


def env():
    out = subprocess.run(["cmd", "/c", "call", str(VCVARS), "&&", "echo", _SENT, "&&", "set"],
                         capture_output=True, text=True)
    e = dict(os.environ); seen = False
    for line in out.stdout.splitlines():
        if line.strip() == _SENT:
            seen = True; continue
        if seen and "=" in line:
            k, v = line.split("=", 1); e[k] = v
    for d in e.get("PATH", "").split(os.pathsep):
        for t in ("cl.exe", "lib.exe", "dumpbin.exe"):
            p = Path(d) / t
            if p.exists():
                e.setdefault("__" + t[:-4].upper() + "__", str(p))
    return e


def undecorate(sym):
    if sym.startswith("@"):                       # fastcall @Name@N
        return sym[1:].split("@", 1)[0]
    if sym.startswith("_"):                        # stdcall _Name@N / cdecl _Name
        return sym[1:].split("@", 1)[0]
    return sym


def dll_for(name):
    if name.startswith("FOG_"): return "Fog"
    if name.startswith(("SReg", "SStr")): return "Storm"
    if name.startswith(("ARCHIVE_", "D2Win_")): return "D2Win"
    if name.startswith(("D2GFX_", "WINDOW_")): return "D2Gfx"
    if name.startswith("D2SOUND_"): return "D2Sound"
    if name.startswith("D2MCPClient"): return "D2MCPClient"
    return None


def parse_def(path):
    m = {}
    for line in Path(path).read_text().splitlines():
        # take "<name> @<ord>" possibly with a leading Alias=Real; use the last name before @<ord>
        mm = re.search(r"([A-Za-z_]\w*)\s+@(\d+)", line)
        if mm:
            m[mm.group(1)] = int(mm.group(2))
    return m


def main():
    e = env()
    OUT.mkdir(exist_ok=True)
    # undefined external symbols in Main.obj
    dump = subprocess.run([e["__DUMPBIN__"], "/symbols", str(OBJ)],
                          capture_output=True, text=True, env=e)
    undefs = set()
    for line in dump.stdout.splitlines():
        if "UNDEF" in line and "External" in line:
            tok = line.split("|")[-1].strip().split()[0]
            undefs.add(tok)
    ordmaps = {k: parse_def(v) for k, v in DEFS.items()}
    groups = {}
    missing = []
    for sym in sorted(undefs):
        name = undecorate(sym)
        dll = dll_for(name)
        if dll is None:
            continue                              # CRT / Win32, satisfied elsewhere
        ordv = ordmaps[dll].get(name)
        if ordv is None:
            missing.append((sym, name, dll)); continue
        groups.setdefault(dll, []).append((sym, name, ordv))
    if missing:
        print("!! no ordinal found for:")
        for sym, name, dll in missing:
            print(f"   {sym}  ({name} in {dll})")
    libs = []
    for dll, entries in groups.items():
        deff = OUT / f"{dll}.def"
        with open(deff, "w") as f:
            f.write(f"LIBRARY {DLL_FILE[dll]}\nEXPORTS\n")
            for sym, name, ordv in entries:
                # lib.exe prepends '_' to a .def name to form the linker symbol,
                # UNLESS it starts with '@' (fastcall). So strip the leading '_'
                # from stdcall/cdecl names; keep '@Name@N' as-is. Import by
                # ordinal, so the InternalName is irrelevant -> no alias.
                dexp = sym[1:] if sym.startswith("_") else sym
                f.write(f"    {dexp} @{ordv} NONAME\n")
        lib = OUT / f"{dll}.lib"
        r = subprocess.run([e["__LIB__"], "/nologo", f"/def:{deff}",
                            f"/out:{lib}", "/machine:x86"],
                           capture_output=True, text=True, env=e)
        if not lib.exists():
            print(f"LIB FAILED ({dll}):\n{r.stdout}{r.stderr}"); return 1
        libs.append(lib)
        print(f"  {dll}.lib  ({len(entries)} imports)")
    print(f"\nOK -> {len(libs)} import libs in {OUT}")
    return 0 if not missing else 1


if __name__ == "__main__":
    sys.exit(main())
