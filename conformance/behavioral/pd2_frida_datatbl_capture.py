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

    exp = api.dump(levels)
    diff = api.dump_diff()
    if not exp or "error" in exp:
        print("  !! experience:", (exp or {}).get("error", "dump failed")); return
    if not diff or "error" in diff:
        print("  !! difficultylevels:", (diff or {}).get("error", "dump failed")); return
    print("g_pExperienceTxtRecords ->", exp["ptr"], " g_pDifficultyLevelsTxt ->", diff["ptr"])

    # keep the standalone experience golden (back-compat)
    with open(os.path.join(HERE, "pd2_experience.json"), "w", encoding="utf-8") as f:
        json.dump(exp["rows"], f, indent=0)
    # combined golden matching the harness output shape
    combined = {"experience": exp["rows"][:12], "difficultylevels": diff["rows"]}
    with open(os.path.join(HERE, "pd2_datatables.json"), "w", encoding="utf-8") as f:
        json.dump(combined, f, indent=0)
    session.detach()
    print("  difficultylevels golden:", diff["rows"])
    print("  wrote pd2_experience.json + pd2_datatables.json")


if __name__ == "__main__":
    main()
