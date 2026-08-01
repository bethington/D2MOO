"""Turn a crash minidump into a symbolised backtrace.

Usage:
    python conformance/tools/analyze_crash_dump.py            # newest dump
    python conformance/tools/analyze_crash_dump.py <file.dmp>
    python conformance/tools/analyze_crash_dump.py --all      # every dump

WHY A DUMP AND NOT THE JSON RECORD. The in-process record carries registers, an
EBP chain walk and a heuristic stack scan. The EBP walk is empty for the crash
that motivated all of this -- ebp was 0x00000001, and once the frame pointer is
smashed there is no chain to walk. cdb reading a minidump does not depend on
the frame pointer: it has every thread's stack and the module list, so it can
produce a real backtrace with symbols.

.ecxr is the important command: it switches context to the RECORDED EXCEPTION
rather than wherever the dumping thread happened to be, which is otherwise the
classic way to get a confident-looking backtrace of the crash reporter instead
of the crash.
"""
from __future__ import annotations

import argparse
import glob
import os
import subprocess
import sys

CRASH_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                         "..", "behavioral", "crashes")

CDB_CANDIDATES = [
    r"C:\Program Files (x86)\Windows Kits\10\Debuggers\x86\cdb.exe",
    r"C:\Program Files\Windows Kits\10\Debuggers\x86\cdb.exe",
    r"C:\Program Files (x86)\Windows Kits\10\Debuggers\x64\cdb.exe",
]

# BITNESS MATTERS. A cdb of the wrong architecture fails with 0xC000007B
# (invalid image format) and writes nothing -- measured while validating this
# script. The game is 32-bit so x86 is the right one for its dumps, but the
# fallback keeps the tool usable against any dump rather than failing opaquely.
def cdb_paths() -> list:
    return [p for p in CDB_CANDIDATES if os.path.isfile(p)]


def find_cdb() -> str | None:
    paths = cdb_paths()
    return paths[0] if paths else None


def newest_dump() -> str | None:
    dumps = glob.glob(os.path.join(CRASH_DIR, "**", "*.dmp"), recursive=True)
    return max(dumps, key=os.path.getmtime) if dumps else None


def analyse(dump: str, cdb: str) -> int:
    # .ecxr  -> switch to the exception context
    # k      -> backtrace of the faulting thread
    # ~*k    -> every thread, since the fault may have been caused elsewhere
    # lm     -> module list, to resolve raw addresses by hand if symbols are thin
    cmds = ".ecxr; r; k 40; .echo ---ALL THREADS---; ~*k 12; .echo ---MODULES---; lm; q"
    # Local symbols only. A public symbol server would be nicer, but this runs
    # against a modded 2000-era game whose DLLs are not on one -- and pointing at
    # the network makes the whole thing hang when it is offline.
    env = dict(os.environ, _NT_SYMBOL_PATH=os.environ.get("_NT_SYMBOL_PATH", ""))
    print("=" * 72)
    print("dump:", dump)
    print("=" * 72, flush=True)
    out = ""
    r = None
    # Try each available cdb: the wrong architecture fails immediately, so this
    # costs nothing and removes a confusing failure mode.
    for exe in cdb_paths():
        r = subprocess.run([exe, "-z", dump, "-c", cmds],
                           capture_output=True, text=True, env=env, timeout=180)
        out = r.stdout or ""
        if "ExceptionAddress" in out or "ChildEBP" in out or "Child-SP" in out:
            break
    # cdb is noisy before it settles; the interesting part starts at .ecxr.
    marker = out.find("ExceptionAddress")
    print(out[marker:] if marker != -1 else out)
    if r.returncode != 0 and not out.strip():
        print("cdb failed:", r.stderr[:400], file=sys.stderr)
        return 1
    return 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("dump", nargs="?", help="dump file (default: newest)")
    ap.add_argument("--all", action="store_true", help="analyse every dump found")
    a = ap.parse_args()

    cdb = find_cdb()
    if not cdb:
        print("cdb.exe not found. Install the Windows SDK 'Debugging Tools for "
              "Windows' feature, or point CDB_CANDIDATES at it.", file=sys.stderr)
        return 2

    if a.all:
        dumps = sorted(glob.glob(os.path.join(CRASH_DIR, "**", "*.dmp"), recursive=True),
                       key=os.path.getmtime)
        if not dumps:
            print("no dumps found in", CRASH_DIR)
            return 1
        for d in dumps:
            analyse(d, cdb)
        return 0

    dump = a.dump or newest_dump()
    if not dump:
        print("no dumps found in", CRASH_DIR)
        print("A dump is written when the game faults AND the crash observer is")
        print("enabled -- set D2DBG_CRASH=1 in the launcher.")
        return 1
    return analyse(dump, cdb)


if __name__ == "__main__":
    raise SystemExit(main())
