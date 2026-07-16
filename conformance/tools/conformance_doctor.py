#!/usr/bin/env python3
"""conformance_doctor.py -- consistency checker for the conformance stack.

Born from the 2026-07-15 incident: 11 property-map endpoints silently vanished from the
deployed GhidraMCP jar, every best-effort caller swallowed the 404s, proof-detail writes
were lost for days, stale reimpl paths pointed nowhere, and 17 proofs' candidate files
turned out to have never been committed. Each check below catches one of those classes
EARLY and LOUDLY. Run it in the conformance-loop preflight and before/after any plugin
jar swap.

Checks:
  1. ENDPOINT CONTRACT -- every plugin endpoint the stack depends on answers (404 = the
     deployed jar doesn't match the code that calls it).
  2. REGISTRY <-> DISK  -- every latest CONF_LIVE+ proof resolves a real reimpl source
     under reimpl_provider/ (catches deleted/renamed/never-written candidates).
  3. REGISTRY <-> GHIDRA -- Ghidra hands back a proof record per proven address
     (bookmarks or property map), the CONF_ rung tag matches the registry, and the
     function NAME at each address matches the registry row (rename drift).
  4. GIT HYGIENE -- candidate sources sitting on disk untracked by git (how the 17
     lost-candidate proofs happened: prove-time files that never got committed).

Exit code: 0 = healthy, 1 = at least one FAIL (warnings alone don't fail).

    python conformance_doctor.py            # all checks
    python conformance_doctor.py --quick    # endpoint contract only (fast preflight)
"""
from __future__ import annotations
import argparse
import json
import subprocess
import sys
import urllib.error
import urllib.request
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import sync_conformance_to_ghidra as sync

REPO = Path(__file__).resolve().parents[2]
CANDIDATES = sync.REIMPL_DIR / "candidates"

# (method, path): everything record_proof, the sync tool, and the dashboard call.
CONTRACT_REQUIRED = [
    ("GET", "/check_connection"), ("GET", "/list_functions"),
    ("GET", "/get_function_tags"), ("GET", "/decompile_function"),
    ("GET", "/get_function_signature"), ("GET", "/search_functions_by_tag"),
    ("GET", "/list_bookmarks"),
    ("POST", "/set_bookmark"), ("POST", "/delete_bookmark"),
    ("POST", "/add_function_tag"), ("POST", "/remove_function_tag"),
    ("POST", "/rename_function_by_address"), ("POST", "/save_program"),
]
# expected ABSENT until ghidra-mcp branch feat/program-options-property-map-tools ships
CONTRACT_OPTIONAL = [
    ("GET", "/get_property"), ("GET", "/list_properties"), ("GET", "/get_program_options"),
    ("POST", "/set_property"), ("POST", "/create_property_map"), ("POST", "/set_program_option"),
]

# proofs at these rungs MUST have their reimpl source on disk (regression-freezable)
DISK_BACKED_RUNGS = {"CONF_LIVE", "CONF_BATTLETESTED", "CONF_REGRESSION"}


def _probe(method: str, path: str) -> str:
    req = urllib.request.Request(f"{sync.GHIDRA}{path}", method=method,
                                 data=b"{}" if method == "POST" else None,
                                 headers={"Content-Type": "application/json"})
    try:
        urllib.request.urlopen(req, timeout=10)
        return "present"
    except urllib.error.HTTPError as e:
        return "missing" if e.code == 404 else "present"
    except OSError:
        return "unreachable"


def _latest_rows() -> dict[str, dict]:
    """address -> latest registry row (the jsonl is append-only chronology)."""
    latest = {}
    for d in sync._load_registry():
        if d.get("address"):
            latest[d["address"]] = d
    return latest


def check_contract() -> tuple[int, int]:
    print("== 1. endpoint contract ==")
    fails = warns = 0
    missing = []
    for method, path in CONTRACT_REQUIRED:
        s = _probe(method, path)
        if s == "unreachable":
            print(f"  FAIL Ghidra plugin unreachable at {sync.GHIDRA} -- start Ghidra "
                  "(remaining checks that need it will be skipped)")
            return 1, 0
        if s == "missing":
            missing.append(path)
    if missing:
        fails += 1
        print(f"  FAIL required endpoints MISSING from the deployed plugin: {', '.join(missing)}")
        print("       -> wrong jar deployed; callers will silently degrade")
    else:
        print(f"  ok   all {len(CONTRACT_REQUIRED)} required endpoints answer")
    opt_missing = [p for m, p in CONTRACT_OPTIONAL if _probe(m, p) == "missing"]
    if opt_missing:
        if len(opt_missing) == len(CONTRACT_OPTIONAL):
            print("  info property-map endpoints absent (expected until "
                  "feat/program-options-property-map-tools deploys; store = CONFORMANCE bookmarks)")
        else:
            warns += 1
            print(f"  WARN property-map endpoints PARTIALLY present -- half-deployed branch? "
                  f"missing: {', '.join(opt_missing)}")
    else:
        print("  info property-map endpoints PRESENT -- run sync push to migrate proof "
              "detail off bookmarks")
    return fails, warns


def check_registry_disk() -> tuple[int, int]:
    print("== 2. registry <-> disk (reimpl sources) ==")
    fails = 0
    latest = _latest_rows()
    lost = [(a, d["name"], d.get("conf")) for a, d in sorted(latest.items())
            if d.get("conf") in DISK_BACKED_RUNGS
            and not sync.resolve_row_reimpl(d)]
    # the 17 known casualties are tracked in PROVE_OPPORTUNITIES_BACKLOG.md item 14;
    # this check FAILS only on NEW losses beyond what the backlog already records
    if lost:
        print(f"  {len(lost)} proven function(s) have NO reimpl source on disk:")
        for a, n, c in lost:
            print(f"    {a}  {n}  ({c})")
        backlog = (REPO / "conformance" / "PROVE_OPPORTUNITIES_BACKLOG.md")
        known = backlog.read_text(encoding="utf-8", errors="replace") if backlog.exists() else ""
        new = [n for _, n, _ in lost if n not in known]
        if new:
            fails += 1
            print(f"  FAIL {len(new)} NEW lost candidate(s) (not in the backlog): {', '.join(new)}")
        else:
            print("  ok   all are known casualties already tracked in the backlog")
    else:
        print("  ok   every latest CONF_LIVE+ proof resolves a reimpl source")
    return fails, 0


def check_registry_ghidra() -> tuple[int, int]:
    print("== 3. registry <-> Ghidra (records, rungs, names) ==")
    fails = warns = 0
    latest = _latest_rows()
    got = sync._readback_count()
    if got is None:
        print("  FAIL no proof records readable from Ghidra at all")
        return 1, 0
    if got < len(latest):
        fails += 1
        print(f"  FAIL Ghidra hands back {got} proof records; registry covers {len(latest)} "
              "addresses -- run sync_conformance_to_ghidra.py")
    else:
        print(f"  ok   {got}/{len(latest)} proof records readable from Ghidra")
    name_drift = rung_drift = 0
    for addr, d in sorted(latest.items()):
        try:
            tg = sync._get("/get_function_tags", function=addr, program=sync.PROGRAM)
        except OSError:
            continue
        gname = tg.get("function")
        tags = {t.get("name") for t in tg.get("tags", [])}
        want = {d.get("name"), d.get("renamed_from")}
        if gname and gname not in want:
            name_drift += 1
            print(f"  WARN name drift @ {addr}: Ghidra='{gname}' registry='{d.get('name')}'")
        conf = d.get("conf")
        if conf and conf not in tags:
            rung_drift += 1
            print(f"  WARN rung drift @ {addr} ({d.get('name')}): registry={conf} "
                  f"ghidra_tags={sorted(t for t in tags if t.startswith('CONF_')) or 'none'}")
    if not name_drift and not rung_drift:
        print(f"  ok   names + CONF_ rungs match the registry at all {len(latest)} addresses")
    warns += name_drift + rung_drift
    return fails, warns


def check_git_hygiene() -> tuple[int, int]:
    print("== 4. git hygiene (uncommitted proof artifacts) ==")
    warns = 0
    try:
        r = subprocess.run(["git", "ls-files", "--others", "--exclude-standard",
                            str(CANDIDATES.relative_to(REPO)).replace("\\", "/")],
                           capture_output=True, text=True, cwd=REPO, timeout=30)
        untracked = [l for l in r.stdout.splitlines() if l.strip().endswith(".cpp")]
    except (OSError, subprocess.SubprocessError) as e:
        print(f"  WARN git check skipped: {e}")
        return 0, 1
    if untracked:
        warns += 1
        print(f"  WARN {len(untracked)} candidate source(s) exist ONLY on disk (untracked "
              "-- this is how the 17 lost-candidate proofs happened). Commit them:")
        for u in untracked[:15]:
            print(f"    {u}")
        if len(untracked) > 15:
            print(f"    ... and {len(untracked) - 15} more")
    else:
        print("  ok   every candidate source is git-tracked")
    return 0, warns


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--quick", action="store_true", help="endpoint contract only")
    args = ap.parse_args()
    fails = warns = 0
    for chk in ([check_contract] if args.quick
                else [check_contract, check_registry_disk, check_registry_ghidra,
                      check_git_hygiene]):
        f, w = chk()
        fails += f
        warns += w
    print(f"\ndoctor: {'FAIL' if fails else 'healthy'} ({fails} fail, {warns} warn)")
    return 1 if fails else 0


if __name__ == "__main__":
    raise SystemExit(main())
