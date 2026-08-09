r"""Build import libraries mapping our C-decorated D2-DLL symbols to ordinals.

D2MOO's own .libs are C++-mangled, so they don't satisfy our C object's
references (@FOG_MPQSetConfig@8 etc.). We emit a .def of
  <our exact decorated name> = <import name> @<ordinal> NONAME
then run lib.exe to produce an import lib, so our fastcall/stdcall call sites
import the game's real DLLs by ordinal.

WHERE THE ORDINALS COME FROM -- and why NOT from D2MOO's .def files.
D2MOO's `*.1.13c.def` name-to-ordinal tables do NOT describe this binary. The
shipped Game.exe reports file version 1.0.13.60, and a direct comparison of the
two import tables showed our D2-DLL imports were DISJOINT from the original's on
every single DLL (D2Win ours 10000/10001/10002/10036/10037/... vs the original's
10005/10032/10052/10073/10086/10139/10142/10158 -- zero overlap). Every D2 DLL
here exports BY ORDINAL ONLY (0 named exports in D2Win/D2gfx), so a name lookup
in someone else's table is unverifiable by construction.

Because static imports run each DLL's DllMain at process load whether or not we
ever call the function, the wrong set corrupted D2Win/D2Lang init before any of
our code ran -- an access violation inside the loader (`D2Win!Ordinal10026` ->
GetProcAddress), while the pristine original faults nowhere under identical
conditions.

So ORDINALS BELOW ARE READ OFF THE ORIGINAL BINARY, not off any table: for each
call site in the original's launcher TU we resolved thunk -> IAT slot -> import
directory -> (dll, ordinal), and matched it to our function by its position and
argument setup in the identical call sequence. Cross-checks that pin the
mapping (see gameexe/README notes): Fog@10101 takes ecx=[cfg+0x200] (bDirect)
exactly as FOG_MPQSetConfig(pCfg->bDirect, FALSE); D2Win@10142 takes the four
D2Win_CreateWindow args; Storm@423/@426 sit under the "Fixed Aspect Ratio" and
"Resolution" string pushes (SRegLoadValue / SRegSaveValue); Fog@10042/@10043
carry the original's OWN source path (r"..\Source\Game\Main.cpp") plus a line
number as their file/line args, which is the game's own alloc/free signature.

Regenerate/verify with `python link/verify_ordinals.py`, which re-derives this
table from the original and fails on any drift.
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

# name -> (dll, ordinal), read off the ORIGINAL Game.exe (see module docstring).
# This is the single source of truth; a D2-DLL symbol missing from it is a hard
# error rather than a silent fallback, because a wrong ordinal here does not
# fail to link -- it calls a different function and corrupts DLL init.
ORDINALS = {
    # --- Fog.dll ---
    "FOG_Alloc":                  ("Fog", 10042),   # ecx=size edx="..\\Source\\Game\\Main.cpp"
    "FOG_Free":                   ("Fog", 10043),   # same file/line arg shape
    "FOG_GetInstallPath":         ("Fog", 10116),   # ecx=buf edx=0x104 (MAX_PATH)
    "FOG_InitErrorMgr":           ("Fog", 10019),
    "FOG_SetLogPrefix":           ("Fog", 10021),
    "FOG_10082_Noop":             ("Fog", 10082),
    "FOG_AsyncDataInitialize":    ("Fog", 10089),   # ecx=1
    "FOG_AsyncDataDestroy":       ("Fog", 10090),
    "FOG_MPQSetConfig":           ("Fog", 10101),   # ecx=cfg->bDirect
    "FOG_DestroyMemoryPoolSystem":("Fog", 10143),   # cdecl, 1 stack arg
    "FOG_10218":                  ("Fog", 10218),
    "FOG_IsExpansion":            ("Fog", 10227),   # result -> cfg->bIsExpansion
    # --- Storm.dll ---
    "SRegLoadString":             ("Storm", 422),   # under "CmdLine"/"SvcCmdLine"
    "SRegLoadValue":              ("Storm", 423),   # under "UseCmdLine"/"Fixed Aspect Ratio"
    "SRegSaveString":             ("Storm", 425),   # under "CmdLine"
    "SRegSaveValue":              ("Storm", 426),   # under "UseCmdLine"/"Resolution"
    "SStrCopy":                   ("Storm", 501),
    "SStrPrintf":                 ("Storm", 578),   # under "v%d.%02d"
    # --- D2Win.dll ---
    "ARCHIVE_LoadExpansionArchives":          ("D2Win", 10005),
    "D2Win_CloseSpriteCache":                 ("D2Win", 10032),
    "D2Win_InitializeSpriteCache":            ("D2Win", 10052),
    "ARCHIVE_ShowInsertExpansionDiscMessage": ("D2Win", 10073),  # edx arg of @10005
    "ARCHIVE_LoadArchives":                   ("D2Win", 10086),
    "ARCHIVE_ShowInsertPlayDiscMessage":      ("D2Win", 10139),  # ecx arg of @10005
    "D2Win_CreateWindow":                     ("D2Win", 10142),  # 4 stack args
    "ARCHIVE_FreeArchives":                   ("D2Win", 10158),
    # --- D2gfx.dll ---
    "D2GFX_EnableVSync":          ("D2Gfx", 10007),
    "D2GFX_SetGamma":             ("D2Gfx", 10034),
    "WINDOW_GetWindow":           ("D2Gfx", 10048),
    "D2GFX_ToggleLowQuality":     ("D2Gfx", 10053),
    "D2GFX_SetFixedAspectRatio":  ("D2Gfx", 10066),
    "D2GFX_SetPerspective":       ("D2Gfx", 10081),
    # The original calls this SAME ordinal for both window-teardown and normal
    # gfx release (0x4076f9, 0x40780a, 0x40784d all target @10084) -- Main.c
    # uses one C name, D2GFX_Release, at every call site rather than aliasing
    # a second name to the same ordinal (lib.exe rejects duplicate NONAME
    # ordinals in one .def).
    "D2GFX_Release":              ("D2Gfx", 10084),
    # --- D2sound.dll / D2MCPClient.dll ---
    "D2SOUND_OpenSoundSystem":    ("D2Sound", 10002),   # ecx=bIsExpansion edx=bBkg
    "D2SOUND_CloseSoundSystem":   ("D2Sound", 10031),
    "D2MCPClientCloseMCP":        ("D2MCPClient", 10018),
}
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
    groups = {}
    missing = []
    for sym in sorted(undefs):
        name = undecorate(sym)
        dll = dll_for(name)
        if dll is None:
            continue                              # CRT / Win32, satisfied elsewhere
        entry = ORDINALS.get(name)
        if entry is None:
            missing.append((sym, name, dll)); continue
        want_dll, ordv = entry
        if want_dll != dll:                       # the name says one DLL, the table another
            missing.append((sym, name, f"{dll} (table says {want_dll})")); continue
        groups.setdefault(dll, []).append((sym, name, ordv))
    if missing:
        # HARD failure. A missing ordinal used to fall through to a .def lookup,
        # which silently produced a *wrong* import -- and a wrong ordinal links
        # fine, then calls a different function and corrupts DLL init at load.
        print("!! no verified ordinal for the following D2-DLL symbols.")
        print("   Add them to ORDINALS after reading the ordinal off the ORIGINAL")
        print("   binary (thunk -> IAT slot -> import directory). Do NOT guess, and")
        print("   do NOT take it from D2MOO's .def -- see this module's docstring.")
        for sym, name, dll in missing:
            print(f"   {sym}  ({name} in {dll})")
        return 1
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
