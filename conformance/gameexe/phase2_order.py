"""What do we actually have to WRITE for Game.exe, and what can be PROVEN?

Two questions the raw per-function classification gets wrong on its own.

1. WHERE THE AUTHORED CODE IS.
   `measure_regargs.py` calls everything the byte lane did not identify
   "authored". On Game.exe that is 55 functions — but 37 of them sit below
   0x407000 and are plainly CRT that the byte lane merely failed to match
   (weak or ambiguous evidence): CRT_strtok, __security_check_cookie,
   __ismbblead, fourteen copies of _unlock_fhandle, __doserrno, malloc,
   realloc, the locale and codepage machinery. We LINK those; we do not
   write them. The launcher lives at 0x407000 and above: 18 functions,
   1,284 instructions. Several wear game-flavoured names a documentation
   pass gave them (`ALLOC_AllocateMemory`, `D2LANG_LocaleMapString`), which
   doc_lint cannot currently flag because they carry neither a LIB_* tag
   nor a FID match.

2. LTCG CONTAMINATES CALLERS, NOT JUST CALLEES.
   A function that takes only stack arguments still cannot be byte-matched
   if it CALLS one that takes arguments in registers the ABI does not name,
   because the call-site setup is part of its bytes and C has no syntax for
   it. `GameEntryPoint` (WinMain) is the clean example: standard stack
   parameters, but it ends

       LEA ECX,[ESP]        ; ECX = &argv
       MOV EAX,0x2          ; EAX = 2
       CALL GAME_InitializeAndStartGame

   MSVC cannot be asked to emit that from a declaration — `__fastcall` gives
   ECX/EDX and there is no spelling for "argument in EAX". So the caller is
   capped too, and the cap propagates along call edges.

Run this after `measure_regargs.py`; it consumes `regargs_report.json`.
"""
from __future__ import annotations

import json
import re
import sys
import urllib.parse
import urllib.request

GHIDRA = "http://127.0.0.1:8089"
PROGRAM = "/Mods/PD2-S12/Game.exe"
LAUNCHER_BASE = 0x407000       # .text below this is CRT on this binary
FASTCALL = {"ECX", "EDX"}      # a real convention; reproducible from C


def get(path, **params):
    params["program"] = PROGRAM
    url = f"{GHIDRA}/{path}?" + urllib.parse.urlencode(params)
    with urllib.request.urlopen(url, timeout=90) as r:
        return json.loads(r.read().decode())


def call_targets(instructions) -> set:
    out = set()
    for i in instructions:
        m = re.match(r"CALL\s+0x([0-9a-fA-F]+)$", (i.get("instruction") or "").strip())
        if m:
            out.add(m.group(1).lower().zfill(8))
    return out


def main() -> int:
    rows = {r["address"]: r for r in json.load(open("regargs_report.json"))}
    authored = [r for r in rows.values()
                if r["verdict"] in ("STANDARD", "FASTCALL", "LTCG_CUSTOM")]
    launcher = [r for r in authored if int(r["address"], 16) >= LAUNCHER_BASE]
    crt_leftovers = len(authored) - len(launcher)

    # Exotic = receives arguments in registers the ABI does not name.
    exotic = {a for a, r in rows.items() if set(r["regs"]) - FASTCALL}

    entries = []
    for r in launcher:
        ins = get("disassemble_function", address=r["address"]).get("instructions") or []
        calls_exotic = sorted(call_targets(ins) & exotic)
        own_exotic = r["address"] in exotic
        if own_exotic:
            tier, why = "TRACE/VEC", "takes register args"
        elif calls_exotic:
            tier, why = "TRACE/VEC", "calls " + ", ".join(
                rows[a]["name"][:24] for a in calls_exotic)
        else:
            tier, why = "BYTEMATCH", ""
        entries.append({"instrs": len(ins), "address": r["address"],
                        "name": r["name"], "tier": tier, "reason": why})
    entries.sort(key=lambda e: (e["tier"] != "BYTEMATCH", e["instrs"]))

    print(f"{'instr':>6} {'addr':10} {'name':40} {'tier':10} why")
    for e in entries:
        print(f"{e['instrs']:6} {e['address']:10} {e['name'][:40]:40} "
              f"{e['tier']:10} {e['reason']}")

    bm = [e for e in entries if e["tier"] == "BYTEMATCH"]
    print(f"\n  CRT below 0x{LAUNCHER_BASE:x} we LINK, never write: {crt_leftovers}")
    print(f"  launcher functions to write:  {len(entries)} "
          f"({sum(e['instrs'] for e in entries)} instructions)")
    print(f"  provable by identity:         {len(bm)}")
    print(f"  behavioural verification:     {len(entries) - len(bm)}")
    json.dump(entries, open("phase2_order.json", "w", encoding="utf-8"), indent=2)
    print("\nwrote phase2_order.json")
    return 0


if __name__ == "__main__":
    sys.exit(main())
