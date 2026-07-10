#!/usr/bin/env python3
"""sync_conformance_to_ghidra.py -- make Ghidra the single source of truth for the
conformance FACTS.

The DOC_/CONF_ RUNG lives in Ghidra function tags already (the port pipeline writes
them on every proof). The one semantic fact that lived ONLY in proven_functions.jsonl
was the PROOF DETAIL -- method, vector counts, shadow hits/divergence, vetted status,
date, and which reimpl candidate proves it. This tool moves that into Ghidra too, into
the `Conf` PROPERTY MAP (Ghidra's typed per-address store) keyed by the function's entry
address -- queryable as a set via list_properties, no bookmark/plate pollution. It also
writes a program-level `Conformance.summary` OPTION (the rollup the dashboard reads in a
single get_program_options call, no per-function scan) and retires the older CONFORMANCE
bookmarks so there is ONE authoritative store.

After a push, Ghidra holds the rung (tag), the proof detail (Conf map), and the summary
(option) -- it is the authoritative record AND the dashboard's read-model.
proven_functions.jsonl becomes a generated MIRROR: `--export` reads the property map
straight back out of Ghidra, proving nothing was lost.

    python sync_conformance_to_ghidra.py            # registry -> Ghidra (push), save
    python sync_conformance_to_ghidra.py --dry-run  # show what would be written
    python sync_conformance_to_ghidra.py --export    # Ghidra -> stdout (round-trip proof)
    python sync_conformance_to_ghidra.py --export --write-mirror  # + regenerate a mirror jsonl

Env: GHIDRA_SERVER_URL (default http://127.0.0.1:8089), FUNDOC_GHIDRA_PROGRAM
(default /Mods/PD2-S12/D2Common.dll). Ghidra must be open with the ghidra-mcp bridge up.
"""
from __future__ import annotations
import argparse
import datetime
import json
import os
import urllib.request
from collections import Counter
from urllib.parse import urlencode
from pathlib import Path

GHIDRA = os.environ.get("GHIDRA_SERVER_URL", "http://127.0.0.1:8089").rstrip("/")
PROGRAM = os.environ.get("FUNDOC_GHIDRA_PROGRAM", "/Mods/PD2-S12/D2Common.dll")
REG = Path(__file__).resolve().parent.parent / "proven_functions.jsonl"


def _get(path: str, **params) -> dict:
    url = f"{GHIDRA}{path}" + ("?" + urlencode(params) if params else "")
    with urllib.request.urlopen(url, timeout=60) as r:
        return json.loads(r.read().decode("utf-8", "replace"))


def _get_text(path: str, **params) -> str:
    """Raw text response (for /list_functions, which returns 'NAME at ADDR' lines)."""
    url = f"{GHIDRA}{path}" + ("?" + urlencode(params) if params else "")
    with urllib.request.urlopen(url, timeout=90) as r:
        return r.read().decode("utf-8", "replace")


def _post(path: str, data: dict) -> dict:
    req = urllib.request.Request(
        f"{GHIDRA}{path}", data=json.dumps(data).encode(),
        headers={"Content-Type": "application/json"}, method="POST")
    with urllib.request.urlopen(req, timeout=60) as r:
        return json.loads(r.read().decode("utf-8", "replace"))


def _load_registry() -> list[dict]:
    rows = []
    for line in REG.read_text(encoding="utf-8").splitlines():
        s = line.strip()
        if not s:
            continue
        try:
            rows.append(json.loads(s))
        except json.JSONDecodeError:
            pass
    return rows


def _conf_record(d: dict) -> dict:
    """The compact proof record stored in the CONFORMANCE bookmark comment.
    Only the SEMANTIC proof facts -- never queue/token/telemetry state."""
    rec: dict = {"conf": d.get("conf")}
    method = d.get("proof_kind") or d.get("method")
    if method:
        rec["method"] = method
    for src, dst in (("vectors", "vectors"), ("passed", "passed"), ("total", "total"),
                     ("shadow_hits", "shadow_hits"), ("shadow_divergences", "shadow_div"),
                     ("vetted", "vetted"), ("ret", "ret"), ("callconv", "callconv"),
                     ("orig_regs", "orig_regs"), ("date", "date")):
        v = d.get(src)
        if v not in (None, "", 0):
            rec[dst] = v
    rec["reimpl"] = f"candidates/{d.get('name')}.cpp"
    if d.get("needs_review"):
        rec["needs_review"] = True
    return rec


CONF_MAP = "Conf"
OPT_GROUP = "Program Information"   # user-writable options group; namespaced key below
OPT_NAME = "Conformance.summary"
# The CONF_ rung ladder is MUTUALLY EXCLUSIVE: setting one must remove the others, or a
# promoted function ends up double-tagged (CONF_LIVE + CONF_BATTLETESTED) and the matrix
# double-counts it. record_proof does this; the sync tool must too.
CONF_RUNGS = ["CONF_DRAFT", "CONF_VECTORS", "CONF_LIVE", "CONF_BATTLETESTED", "CONF_REGRESSION"]


def _ensure_conf_map() -> None:
    """Create the `Conf` string property map if missing (idempotent -- create on an
    existing map just errors harmlessly)."""
    try:
        _post("/create_property_map", {"name": CONF_MAP, "type": "string", "program": PROGRAM})
    except OSError:
        pass


def _set_rung_exclusive(addr: str, rung: str, dry: bool) -> None:
    """Set ONE CONF_ rung, removing the others first (mutual exclusivity)."""
    others = ",".join(t for t in CONF_RUNGS if t != rung)
    _post("/remove_function_tag", {"function": addr, "tags": others, "program": PROGRAM, "dry_run": dry})
    _post("/add_function_tag", {"function": addr, "tags": rung, "program": PROGRAM, "dry_run": dry})


def _reconcile_orphans(reg_addrs: set[str], dry: bool) -> int:
    """Remove CONF_ rung tags from functions NOT in the registry -- orphans left behind by
    registry dedup / false-clean removal (e.g. mislabeled fast_path proofs whose Ghidra
    tag lingered). The registry is authoritative for WHICH functions are proven."""
    removed = 0
    for rung in CONF_RUNGS:
        try:
            r = _get("/search_functions_by_tag", tag=rung, program=PROGRAM)
        except OSError:
            continue
        for f in r.get("functions", []):
            a = "0x" + str(f.get("address", "")).lower()
            if a and a not in reg_addrs:
                if not dry:
                    _post("/remove_function_tag", {"function": a, "tags": rung, "program": PROGRAM})
                removed += 1
                print(f"  orphan: removed {rung} from {a} ({f.get('name')})")
    return removed


def _rollup(rows: list[dict]) -> dict:
    """The program-level conformance summary the dashboard reads in ONE get_program_options
    call -- no per-function scan. Raw counts; the dashboard derives in-scope."""
    c = Counter(d.get("conf") for d in rows)
    rec = {
        "proven": len(rows),
        "CONF_LIVE": c.get("CONF_LIVE", 0),
        "CONF_BATTLETESTED": c.get("CONF_BATTLETESTED", 0),
        "CONF_REGRESSION": c.get("CONF_REGRESSION", 0),
        "vetted": sum(1 for d in rows if d.get("vetted")),
        "last_sync": datetime.date.today().isoformat(),
    }
    try:
        # in_scope = functions DEFINED in this program (list_functions, deduped by address
        # -- externals/imports aren't defined here) minus the LIB_ library/runtime tags.
        import re
        _at = re.compile(r"\bat\s+([0-9a-fA-F]+)\s*$")
        local = {m.group(1).lower() for ln in _get_text("/list_functions", program=PROGRAM,
                                                         limit=6000).splitlines()
                 for m in [_at.search(ln.strip())] if m}
        tags = _get("/list_function_tags").get("tags", [])
        lib = sum(t.get("use_count", 0) for t in tags if str(t.get("name", "")).startswith("LIB_"))
        rec["local_defined"] = len(local)
        rec["excluded_lib"] = lib
        rec["in_scope"] = len(local) - lib
        rec["total_all"] = _get("/get_function_count", program=PROGRAM).get("function_count")
    except (OSError, AttributeError):
        pass
    return rec


def push(dry: bool = False, tags: bool = True, cleanup_bookmarks: bool = True) -> int:
    rows = _load_registry()
    if not dry:
        _ensure_conf_map()
    prop_ok = tag_ok = bm_del = 0
    fails = []
    for d in rows:
        addr, name, conf = d.get("address"), d.get("name"), d.get("conf")
        if not addr:
            continue
        value = json.dumps(_conf_record(d), separators=(",", ":"))
        if tags and conf:
            try:
                _set_rung_exclusive(addr, conf, dry)   # mutual exclusivity, no double-tags
                tag_ok += 1
            except OSError:
                pass
        if dry:
            prop_ok += 1
            continue
        try:
            r = _post("/set_property", {"map": CONF_MAP, "address": addr,
                                        "value": value, "program": PROGRAM})
            if r.get("success"):
                prop_ok += 1
            else:
                fails.append((name, r))
        except OSError as e:
            fails.append((name, str(e)))
        if cleanup_bookmarks:   # retire the old CONFORMANCE bookmark -- one store now
            try:
                bm_del += _post("/delete_bookmark",
                                {"address": addr, "category": "CONFORMANCE",
                                 "program": PROGRAM}).get("deleted", 0)
            except OSError:
                pass
    orphans = 0
    if tags:
        orphans = _reconcile_orphans({d.get("address") for d in rows if d.get("address")}, dry)
    if not dry:
        try:
            _post("/set_program_option", {"group": OPT_GROUP, "name": OPT_NAME,
                                          "value": json.dumps(_rollup(rows)), "program": PROGRAM})
        except OSError:
            pass
        try:
            _post("/save_program", {"program": PROGRAM})
        except OSError:
            print("  [warn] save_program failed -- writes are in memory, save Ghidra manually")
    tag = " (DRY RUN -- nothing written)" if dry else ""
    print(f"pushed {prop_ok}/{len(rows)} Conf properties"
          f"{f', {tag_ok} rung tags (exclusive)' if tags else ''}"
          f"{f', removed {orphans} orphan rung tags' if orphans else ''}"
          f"{f', retired {bm_del} old bookmarks' if bm_del else ''}"
          f"{'' if dry else ', wrote Conformance.summary rollup'}{tag}")
    for name, err in fails[:12]:
        print(f"  FAIL {name}: {str(err)[:110]}")
    return 0 if not fails else 1


def export(write_mirror: bool = False) -> int:
    recs, off = [], 0
    while True:
        r = _get("/list_properties", map=CONF_MAP, program=PROGRAM, offset=off, limit=500)
        entries = r.get("entries", [])
        for e in entries:
            try:
                rec = json.loads(e["value"])
                rec["address"] = "0x" + e["address"]
                recs.append(rec)
            except (KeyError, json.JSONDecodeError):
                pass
        if len(entries) < 500 or off + 500 >= r.get("total", 0):
            break
        off += 500
    print(f"read {len(recs)} Conf records back from the Ghidra property map (round-trip proof)")
    for rec in recs[:3]:
        print("  " + json.dumps(rec, separators=(",", ":")))
    try:
        opts = _get("/get_program_options", group=OPT_GROUP, program=PROGRAM).get("options", [])
        summ = next((o["value"] for o in opts if o.get("name") == OPT_NAME), None)
        if summ:
            print("Conformance.summary (one-call dashboard rollup): " + summ)
    except OSError:
        pass
    if write_mirror:
        out = REG.parent / "proven_functions.from_ghidra.jsonl"
        out.write_text("\n".join(json.dumps(r) for r in recs) + "\n", encoding="utf-8")
        print(f"wrote Ghidra-sourced mirror -> {out}")
    return 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--dry-run", action="store_true", help="show writes, change nothing")
    ap.add_argument("--export", action="store_true", help="read Ghidra bookmarks back out")
    ap.add_argument("--write-mirror", action="store_true",
                    help="with --export: regenerate a jsonl from Ghidra")
    ap.add_argument("--no-tags", action="store_true", help="only write bookmarks, skip rung tags")
    args = ap.parse_args()
    if args.export:
        return export(write_mirror=args.write_mirror)
    return push(dry=args.dry_run, tags=not args.no_tags)


if __name__ == "__main__":
    raise SystemExit(main())
