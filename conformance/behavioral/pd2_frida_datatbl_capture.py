"""pd2_frida_datatbl_capture -- PD2 golden capture of the live Experience.txt
table (Milestone 2 of ../DATATABLE_CONFORMANCE_PLAN.md). Read-only: dereferences
a Ghidra-verified global pointer, no calls into game code.

Usage: python pd2_frida_datatbl_capture.py [Game.exe|<pid>] [levels]
"""
import frida
import json
import os
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))


def main():
    target = sys.argv[1] if len(sys.argv) > 1 else "Game.exe"
    try:
        target = int(target)
    except ValueError:
        pass
    levels = int(sys.argv[2]) if len(sys.argv) > 2 else 100

    session = frida.attach(target)
    with open(os.path.join(HERE, "pd2_frida_datatbl.js"), encoding="utf-8") as f:
        script = session.create_script(f.read())
    script.load()
    api = script.exports_sync

    base = None
    for _ in range(20):
        base = api.ready()
        if base:
            break
        print("  D2Common not loaded yet..."); time.sleep(3)
    if not base:
        print("  !! D2Common never loaded"); return
    print("D2Common base", base)

    result = api.dump(levels)
    if not result or "error" in result:
        print("  !!", (result or {}).get("error", "dump failed")); return
    print("g_pExperienceTxtRecords ->", result["ptr"])
    for r in result["rows"][:5]:
        print(" ", r)
    print(f"  ... {len(result['rows'])} rows total")

    with open(os.path.join(HERE, "pd2_experience.json"), "w", encoding="utf-8") as f:
        json.dump(result["rows"], f, indent=0)
    session.detach()
    print("  wrote pd2_experience.json")


if __name__ == "__main__":
    main()
