"""pd2_frida_capture — the PD2 (in-process) half of the behavioral oracle.

Attaches Frida to the running Game.exe, loads pd2_frida_agent.js, and calls the
coord family with the SAME chosen inputs as the D2MOO fingerprint
(d2moo_fingerprint.cpp / d2moo_fp.json). Emits pd2_fp.json in the identical
shape, so behav_match.py (or a direct diff) can prove PD2 == D2MOO bit-for-bit.

Why in-process (Frida) instead of the external ghidra-mcp debugger: external
dbgeng on 32-bit WOW64 PD2 fought the anti-debug INT3 flood, comtypes/COM
instability, and the can't-hijack-a-parked-thread wall. In-process removes all
of it -- the coord functions are pure math, so we just call them. See
reference-frida-inprocess-oracle.

Usage:
  python pd2_frida_capture.py [Game.exe|<pid>]
  # needs a Frida venv (pip install frida) and, since PD2 runs elevated,
  # likely an elevated shell to inject.
"""
import frida
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))

# Must match d2moo_fingerprint.cpp kInputs exactly (order-sensitive).
INPUTS = [(0, 0), (1, 0), (0, 1), (5, 3), (-7, 2), (10, -4),
          (100, 20), (-1, -2), (3, 7), (-4, -8), (255, 1), (-100, 100)]

FNS = [
    "DUNGEON_GameTileToClientCoords",
    "DUNGEON_GameTileToSubtileCoords",
    "DUNGEON_ClientToGameCoords",
    "DUNGEON_GameToClientCoords",
    "DUNGEON_GameSubtileToClientCoords",
]


def main():
    target = sys.argv[1] if len(sys.argv) > 1 else "Game.exe"
    try:
        target = int(target)
    except ValueError:
        pass

    session = frida.attach(target)
    with open(os.path.join(HERE, "pd2_frida_agent.js"), encoding="utf-8") as f:
        script = session.create_script(f.read())
    script.load()
    api = script.exports_sync
    print("D2Common base:", api.base())

    result = {}
    for fn in FNS:
        entries = []
        for (x, y) in INPUTS:
            out = api.call(fn, x, y)
            entries.append({"in": [x, y], "out": list(out)})
        result[fn] = entries
        # quick eyeball of the first non-trivial vector
        print(f"  {fn:36} (5,3) -> {result[fn][3]['out']}")

    outpath = os.path.join(HERE, "pd2_fp.json")
    with open(outpath, "w", encoding="utf-8") as f:
        json.dump(result, f, indent=0)
    session.detach()
    print("wrote", outpath)


if __name__ == "__main__":
    main()
