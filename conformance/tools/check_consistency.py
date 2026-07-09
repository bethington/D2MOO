#!/usr/bin/env python3
"""check_consistency.py -- cross-artifact NAME consistency for proven functions.

A proof (and now the post-proof auto-rename, CONFORMANCE_TAXONOMY.md) references the
same function across several artifacts keyed by NAME, which DRIFT from Ghidra (the
source of truth) after a rename. This reconciles them by the stable ADDRESS -- the
load-bearing companion to the auto-rename, so a rename can't silently desync the pipeline.

DIRECTION IS AN EVIDENCE CALL, so it is REPORT-ONLY by default -- it never guesses
which side is right. Ghidra is the NOMINAL source of truth, BUT the registry can hold
a validated D2MOO CANONICAL name while Ghidra still has the decompiler guess (the coord
family: registry DUNGEON_GameTileToClientCoords vs Ghidra TileToScreenCoordsInPlace) --
there, blindly adopting Ghidra would OVERWRITE the good name. So each disagreement gets
a direction HINT, and reconciliation needs an explicit flag:
  --adopt-ghidra    : registry + corrected_maps follow Ghidra (use AFTER an auto-rename)
  --adopt-registry  : rename the GHIDRA function to the registry's canonical name

Reports downstream staleness either way: candidates/<name>.cpp + vectors/<name>.spec.json
(named by the old name; functionally OK -- specs resolve by ADDRESS -- but rename to align),
and the resolve-table / profiler snapshot regen + provider rebuild a name change needs.

Usage:
    python check_consistency.py                  # report + direction hints, change nothing
    python check_consistency.py --adopt-ghidra   # registry <- Ghidra (post auto-rename)
    python check_consistency.py --adopt-registry # Ghidra <- registry canonical name
"""
from __future__ import annotations

import argparse
import http.client
import json
import os
import re
from pathlib import Path
from urllib.parse import urlencode

HERE = Path(__file__).resolve().parent
REPO = HERE.parent.parent
REGISTRY = REPO / "conformance" / "proven_functions.jsonl"
CANDIDATES = REPO / "conformance" / "reimpl_provider" / "candidates"
VECTORS = REPO / "conformance" / "vectors"
CORRECTED = REPO / "conformance" / "corrected_maps"
GHIDRA = os.environ.get("GHIDRA_MCP_URL", "http://127.0.0.1:8089")


def _ghidra_name(address: str, program: str) -> str | None:
    """Current Ghidra function name at an address (the source of truth). None on any
    failure (unreachable / not a function)."""
    try:
        conn = http.client.HTTPConnection("127.0.0.1", 8089, timeout=15)
        conn.request("GET", "/get_function_by_address?" +
                     urlencode({"address": address, "program": program}))
        raw = conn.getresponse().read().decode("utf-8", "replace")
        conn.close()
    except OSError:
        return None
    m = re.search(r"Function:\s+(\S+)\s+at", raw)
    return m.group(1) if m else None


def _load_registry() -> list[dict]:
    return [json.loads(l) for l in REGISTRY.read_text(encoding="utf-8").splitlines() if l.strip()]


def _save_registry(rows: list[dict]) -> None:
    with open(REGISTRY, "w", encoding="utf-8") as f:
        for r in rows:
            f.write(json.dumps(r) + "\n")


def _corrected_tsv(program: str) -> Path | None:
    p = CORRECTED / f"{program.replace('.dll', '')}.tsv"
    return p if p.exists() else None


def _reconcile_corrected_map(program: str, address: str, new_name: str, dry: bool) -> str | None:
    """Update the ghidra_name column for `address` in corrected_maps. Returns a note
    if it was stale (or None). Address compared case-insensitively, 0x-normalized."""
    tsv = _corrected_tsv(program)
    if not tsv:
        return None
    want = address.lower().lstrip("0x")
    lines = tsv.read_text(encoding="utf-8").splitlines()
    changed = None
    for i, line in enumerate(lines):
        cols = line.split("\t")
        if len(cols) < 3:
            continue
        addr = cols[1].lower().lstrip("0x")
        if addr == want and cols[2] != new_name:
            changed = f"corrected_maps {tsv.name}: '{cols[2]}' -> '{new_name}'"
            if not dry:
                cols[2] = new_name
                if len(cols) >= 5:  # names_agree column: recompute vs def_name
                    cols[4] = "yes" if cols[3] == new_name else "no"
                lines[i] = "\t".join(cols)
            break
    if changed and not dry:
        tsv.write_text("\n".join(lines) + "\n", encoding="utf-8")
    return changed


# A canonical D2MOO name is SUBSYSTEM_Action (uppercase prefix + '_' + word); a
# decompiler-era name is not. The DIRECTION of a fix depends on which side is
# canonical -- so the tool never guesses it, it HINTS and requires an explicit flag.
_CANON_RE = re.compile(r"^[A-Z][A-Z0-9]{1,}_[A-Za-z]")
_AUTO_RE = re.compile(r"^(FUN_|Ordinal_|SUB_|LAB_|UNK_|DAT_)", re.IGNORECASE)


def _canonicalish(name: str) -> bool:
    return bool(_CANON_RE.match(name or "")) and not _AUTO_RE.match(name or "")


def _direction_hint(regname: str, gname: str) -> str:
    rc, gc = _canonicalish(regname), _canonicalish(gname)
    if rc and not gc:
        return f"registry '{regname}' looks canonical, Ghidra '{gname}' looks decompiler-era -> " \
               f"likely rename GHIDRA to the registry name (--adopt-registry, aligns w/ D2MOO)"
    if gc and not rc:
        return f"Ghidra '{gname}' looks canonical -> likely update the registry (--adopt-ghidra)"
    if _AUTO_RE.match(gname):
        return f"Ghidra name is a raw auto-name -> registry likely right (--adopt-registry)"
    return "AMBIGUOUS (both look intentional) -- resolve by evidence (string/xref/D2MOO); no flag will guess"


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--adopt-ghidra", action="store_true",
                    help="reconcile registry+corrected_maps TO the Ghidra name (use after an auto-rename, "
                         "where Ghidra is the fresh source of truth)")
    ap.add_argument("--adopt-registry", action="store_true",
                    help="rename the GHIDRA function TO the registry name (use when the registry holds the "
                         "validated D2MOO canonical name and Ghidra has a decompiler guess, e.g. the coord family)")
    args = ap.parse_args()
    write = args.adopt_ghidra or args.adopt_registry

    rows = _load_registry()
    fixed_registry = fixed_maps = fixed_ghidra = 0
    reports: list[str] = []
    orphans: list[str] = []
    regen = False

    for r in rows:
        addr, prog, regname = r.get("address", ""), r.get("program", "D2Common.dll"), r.get("name", "?")
        gname = _ghidra_name(addr, prog)
        if gname is None:
            orphans.append(f"  {regname} @{addr}: NOT a Ghidra function (deleted/moved?)")
            continue
        if gname == regname:
            continue
        regen = True
        reports.append(f"  DISAGREE @{addr}: registry '{regname}'  vs  Ghidra '{gname}'")
        reports.append(f"      hint: {_direction_hint(regname, gname)}")

        if args.adopt_ghidra:  # registry + maps follow Ghidra
            r["ghidra_prev_name"] = r.get("ghidra_prev_name", regname)
            r["name"] = gname
            fixed_registry += 1
            if _reconcile_corrected_map(prog, addr, gname, dry=False):
                fixed_maps += 1
            for base, ext, d in ((regname, ".cpp", CANDIDATES), (regname, ".spec.json", VECTORS)):
                if (d / f"{base}{ext}").exists():
                    reports.append(f"      + stale file {(d / f'{base}{ext}').relative_to(REPO)} "
                                   f"-> rename to {gname}{ext} + update the export marker (build step)")
        elif args.adopt_registry:  # Ghidra follows the registry (canonical name)
            try:
                conn = http.client.HTTPConnection("127.0.0.1", 8089, timeout=20)
                conn.request("POST", "/rename_function_by_address?" + urlencode({"program": prog}),
                             body=json.dumps({"function_address": addr, "new_name": regname}),
                             headers={"Content-Type": "application/json"})
                conn.getresponse().read()
                conn.close()
                fixed_ghidra += 1
                reports.append(f"      + renamed Ghidra '{gname}' -> '{regname}' (save_program + snapshot regen needed)")
            except OSError as e:
                reports.append(f"      + Ghidra rename FAILED: {e}")

    print("=" * 78)
    print("CROSS-ARTIFACT NAME CONSISTENCY  (keyed by ADDRESS; direction is an evidence call)")
    print("=" * 78)
    print("\n".join(reports) if reports else "  all proven functions' names agree with Ghidra.")
    if orphans:
        print("-- registry rows with no Ghidra function --\n" + "\n".join(orphans))
    print("-" * 78)
    if not write:
        print("REPORT ONLY (default). Direction is not auto-chosen -- Ghidra is the nominal source of")
        print("truth, but the registry may hold a validated D2MOO CANONICAL name (see the coord family).")
        print("Re-run with --adopt-ghidra (post-rename) or --adopt-registry (registry is canonical).")
    else:
        print(f"adopted: {fixed_registry} registry, {fixed_maps} corrected_map, {fixed_ghidra} Ghidra rename(s)")
        if args.adopt_registry and fixed_ghidra:
            print("  -> run save_program in Ghidra to persist the renames.")
        if fixed_registry or fixed_maps:
            _save_registry(rows)
    if regen:
        print("NOTE: name-keyed snapshots go stale on any adopted change -> regenerate + rebuild:")
        print("  python conformance/tools/build_profiler_tables.py D2Common")
        print("  python conformance/tools/gen_resolve_table.py   (then rebuild the D2Common patch)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
