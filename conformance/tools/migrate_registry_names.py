#!/usr/bin/env python3
"""migrate_registry_names.py -- heal name drift between proven_functions.jsonl and Ghidra.

The 2026-07-14 function-name audit renamed Ghidra functions by CODE (the naming
authority is the D2MOO source, resolved via fun-doc); proof rows recorded before the
audit still carry the draft names. conformance_doctor.py check 3 WARNs on each such
address; this tool applies the fix in the right DIRECTION per row:

  REGISTRY_STALE (the common case): the Ghidra name is a semantic, audited name and the
      registry name is a pre-audit draft (usually offset-derived, e.g.
      ITEMS_GetItemRecordByte137 -> ITEMS_GetItemHasInv). Update the LATEST registry row:
      name = Ghidra name, renamed_from = old name (kept so reimpl resolution can still
      find candidate files written under the legacy name, and so history is traceable).
      Older rows at the same address are left untouched -- the jsonl is chronology.

  GHIDRA_STALE: the registry name is a COMPONENT_-prefixed canonical proven name but
      Ghidra still has a junk/pre-audit name with no component prefix (e.g.
      MultiplyValuesBy5 vs DUNGEON_GameTileToSubtileCoords, runtime-verified in
      registry.json). Rename the Ghidra function (strict_mode=warn -- these names are
      proof-backed, the collision guard's token heuristics don't apply).

Ambiguity guard: a drift row is only auto-migrated when the direction rule is clear;
anything else is listed for manual review and left alone.

    python migrate_registry_names.py --dry-run   # show classification + planned writes
    python migrate_registry_names.py             # apply, then save the Ghidra program

Run `sync_conformance_to_ghidra.py` afterwards so the proof records pick up the new
names/paths, then `conformance_doctor.py` to confirm the drift is gone.
"""
from __future__ import annotations
import argparse
import json
import re
import sys
from pathlib import Path
from urllib.parse import urlencode

sys.path.insert(0, str(Path(__file__).resolve().parent))
import sync_conformance_to_ghidra as sync

COMPONENT = re.compile(r"^[A-Z][A-Z0-9]*_")


def _classify(ghidra_name: str, reg_name: str) -> str:
    if not COMPONENT.match(ghidra_name) and COMPONENT.match(reg_name):
        return "GHIDRA_STALE"
    if COMPONENT.match(ghidra_name):
        return "REGISTRY_STALE"
    return "AMBIGUOUS"


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--dry-run", action="store_true", help="classify + plan, change nothing")
    args = ap.parse_args()
    dry = args.dry_run

    # latest row per address, WITH its line index so only that jsonl line is rewritten
    lines = sync.REG.read_text(encoding="utf-8").splitlines()
    latest: dict[str, tuple[int, dict]] = {}
    for i, line in enumerate(lines):
        s = line.strip()
        if not s:
            continue
        try:
            d = json.loads(s)
        except json.JSONDecodeError:
            continue
        if d.get("address"):
            latest[d["address"]] = (i, d)

    reg_fixed = ghidra_fixed = 0
    manual = []
    fails = []
    for addr, (i, d) in sorted(latest.items()):
        try:
            tg = sync._get("/get_function_tags", function=addr, program=sync.PROGRAM)
        except OSError:
            continue
        gname = tg.get("function")
        if not gname or gname in (d.get("name"), d.get("renamed_from")):
            continue
        side = _classify(gname, d.get("name") or "")
        if side == "REGISTRY_STALE":
            print(f"registry: {addr}  {d.get('name')} -> {gname}")
            if not dry:
                d["renamed_from"] = d.get("name")
                d["name"] = gname
                lines[i] = json.dumps(d)
            reg_fixed += 1
        elif side == "GHIDRA_STALE":
            print(f"ghidra:   {addr}  {gname} -> {d.get('name')}")
            if not dry:
                try:
                    r = sync._post("/rename_function_by_address?" + urlencode({"program": sync.PROGRAM}),
                                   {"function_address": addr, "new_name": d.get("name"),
                                    "strict_mode": "warn"})
                    if r.get("status") == "success":
                        ghidra_fixed += 1
                    else:
                        fails.append((addr, r))
                except OSError as e:
                    fails.append((addr, str(e)))
            else:
                ghidra_fixed += 1
        else:
            manual.append((addr, gname, d.get("name")))

    if not dry and reg_fixed:
        sync.REG.write_text("\n".join(lines) + "\n", encoding="utf-8")
    if not dry and ghidra_fixed:
        try:
            sync._post("/save_program", {"program": sync.PROGRAM})
        except OSError:
            print("  [warn] save_program failed -- Ghidra renames are in memory only")

    tag = " (DRY RUN)" if dry else ""
    print(f"\nmigrated {reg_fixed} registry row(s), renamed {ghidra_fixed} Ghidra function(s){tag}")
    for addr, g, r in manual:
        print(f"  MANUAL REVIEW {addr}: ghidra='{g}' registry='{r}' -- no clear direction")
    for addr, err in fails:
        print(f"  FAIL {addr}: {str(err)[:140]}")
    if not dry:
        print("now run: sync_conformance_to_ghidra.py  then  conformance_doctor.py")
    return 1 if fails else 0


if __name__ == "__main__":
    raise SystemExit(main())
