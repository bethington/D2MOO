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

NOTE 2026-07-15: the property-map/program-option endpoints live on the ghidra-mcp branch
feat/program-options-property-map-tools, which is NOT in the deployed plugin -- every
property write since the bookmark->property migration had silently 404'd, leaving Ghidra
with rung tags only. Until that branch ships, push() auto-detects the missing endpoints
and stores the proof detail in CONFORMANCE bookmarks (and --export reads them back).

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
import urllib.error
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


REIMPL_DIR = REG.parent / "reimpl_provider"
_reimpl_sources: list[tuple[str, str]] | None = None   # [(relpath, text)] cache for push


def _resolve_reimpl(name: str) -> str | None:
    """The proof record's reimpl path must point at a REAL file. Most proofs live at
    candidates/<name>.cpp, but the coord family is inline in coord_provider.cpp, early
    batches share multi-function candidates (unit_field_getters.cpp, batch_shakeout.cpp),
    and the name-audit renamed some candidate files after their proofs were recorded --
    so resolve by scanning for the definition instead of assuming the path."""
    global _reimpl_sources
    if not name:
        return None
    if (REIMPL_DIR / "candidates" / f"{name}.cpp").exists():
        return f"candidates/{name}.cpp"
    if _reimpl_sources is None:
        _reimpl_sources = []
        for fp in sorted(REIMPL_DIR.glob("*.cpp")) + sorted((REIMPL_DIR / "candidates").glob("*.cpp")):
            try:
                _reimpl_sources.append((fp.relative_to(REIMPL_DIR).as_posix(),
                                        fp.read_text(encoding="utf-8", errors="replace")))
            except OSError:
                pass
    import re
    pat = re.compile(r"^[A-Za-z_(][^;\n]*\b" + re.escape(name) + r"\s*\(", re.M)
    # renamed candidates carry a "originally proven under the name X" note
    note = re.compile(r"proven under the name\s+" + re.escape(name) + r"\b")
    for rel, text in _reimpl_sources:
        if name in text and (pat.search(text) or note.search(text)):
            return rel
    return None


def resolve_row_reimpl(d: dict) -> str | None:
    """Resolve a registry row's reimpl source, trying the current name first and the
    pre-audit name second (shared by _conf_record and conformance_doctor)."""
    return (_resolve_reimpl(d.get("name") or "")
            or _resolve_reimpl(d.get("renamed_from") or ""))


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
    # a name-audit rename can leave the candidate file under the LEGACY name (the
    # shadow dispatcher still binds by the old export key) -- try both
    reimpl = resolve_row_reimpl(d)
    if reimpl:                      # omit rather than record a path that isn't on disk
        rec["reimpl"] = reimpl
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
        # -- externals/imports aren't defined here) minus everything out of the "real game
        # work" scope: LIB_* (library/runtime) + the trivial dispositions STUB/THUNK/EXTERNAL.
        import re
        _at = re.compile(r"\bat\s+([0-9a-fA-F]+)\s*$")
        local = {m.group(1).lower() for ln in _get_text("/list_functions", program=PROGRAM,
                                                         limit=100000).splitlines()
                 for m in [_at.search(ln.strip())] if m}
        tags = _get("/list_function_tags").get("tags", [])
        def _tagsum(pred):
            return sum(t.get("use_count", 0) for t in tags if pred(str(t.get("name", ""))))
        lib = _tagsum(lambda n: n.startswith("LIB_"))
        disp = _tagsum(lambda n: n in ("STUB", "THUNK", "EXTERNAL"))
        rec["local_defined"] = len(local)
        rec["excluded_lib"] = lib
        rec["excluded_disposition"] = disp
        rec["in_scope"] = len(local) - lib - disp
        rec["total_all"] = _get("/get_function_count", program=PROGRAM).get("function_count")
    except (OSError, AttributeError):
        pass
    return rec


def _property_endpoints_available() -> bool:
    """The Conf PROPERTY MAP endpoints live on the ghidra-mcp branch
    feat/program-options-property-map-tools, which is NOT in the deployed plugin
    (verified 2026-07-15: /set_property 404s on the running 5.16.2 jar, so every
    property write had been silently lost). Until that branch ships, the proof
    detail is stored in CONFORMANCE bookmarks -- the per-address store the
    deployed plugin does support."""
    try:
        _post("/set_property", {})
        return True
    except urllib.error.HTTPError as e:
        return e.code != 404
    except OSError:
        return False


def push(dry: bool = False, tags: bool = True, cleanup_bookmarks: bool = True) -> int:
    rows = _load_registry()
    use_props = _property_endpoints_available()
    if not use_props:
        cleanup_bookmarks = False   # bookmarks ARE the store -- never retire them
        print("property-map endpoints missing in the deployed plugin -> writing proof "
              "detail to CONFORMANCE bookmarks instead")
    if not dry and use_props:
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
        row_ok = False
        try:
            if use_props:
                r = _post("/set_property", {"map": CONF_MAP, "address": addr,
                                            "value": value, "program": PROGRAM})
            else:
                # program is a QUERY param on the bookmark endpoints, not a body field
                r = _post("/set_bookmark?" + urlencode({"program": PROGRAM}),
                          {"address": addr, "category": "CONFORMANCE", "comment": value})
            row_ok = bool(r.get("success"))
            if row_ok:
                prop_ok += 1
            else:
                fails.append((name, r))
        except OSError as e:
            fails.append((name, str(e)))
        # retire the old CONFORMANCE bookmark ONLY after this row's new-store write is
        # verified -- deleting first is how a silent write failure becomes data loss
        if cleanup_bookmarks and row_ok:
            try:
                bm_del += _post("/delete_bookmark?" + urlencode({"program": PROGRAM}),
                                {"address": addr, "category": "CONFORMANCE"}).get("deleted", 0)
            except OSError:
                pass
    orphans = 0
    if tags:
        orphans = _reconcile_orphans({d.get("address") for d in rows if d.get("address")}, dry)
    opt_ok = False
    if not dry:
        try:
            _post("/set_program_option", {"group": OPT_GROUP, "name": OPT_NAME,
                                          "value": json.dumps(_rollup(rows)), "program": PROGRAM})
            opt_ok = True
        except OSError:
            pass
        try:
            _post("/save_program", {"program": PROGRAM})
        except OSError:
            print("  [warn] save_program failed -- writes are in memory, save Ghidra manually")
    tag = " (DRY RUN -- nothing written)" if dry else ""
    store = "Conf properties" if use_props else "CONFORMANCE bookmarks"
    print(f"pushed {prop_ok}/{len(rows)} {store}"
          f"{f', {tag_ok} rung tags (exclusive)' if tags else ''}"
          f"{f', removed {orphans} orphan rung tags' if orphans else ''}"
          f"{f', retired {bm_del} old bookmarks' if bm_del else ''}"
          f"{', wrote Conformance.summary rollup' if opt_ok else ''}{tag}")
    for name, err in fails[:12]:
        print(f"  FAIL {name}: {str(err)[:110]}")
    # ROUND-TRIP GATE: a push isn't done until Ghidra can hand the records back.
    # (A silent one-way write is exactly how the property-endpoint regression hid.)
    gate_ok = True
    if not dry:
        want = len({d.get("address") for d in rows if d.get("address")})
        got = _readback_count()
        if got is None:
            print(f"  [gate FAIL] cannot read proof records back from Ghidra -- push unverified")
            gate_ok = False
        elif got < want:
            print(f"  [gate FAIL] read back {got} records but the registry covers {want} "
                  f"addresses -- proof detail is missing in Ghidra")
            gate_ok = False
        else:
            print(f"  round-trip gate OK: {got}/{want} proof records read back from Ghidra")
    return 0 if not fails and gate_ok else 1


def _readback_count() -> int | None:
    """How many proof records Ghidra can hand back RIGHT NOW -- property map when the
    endpoints exist, CONFORMANCE bookmarks otherwise. None = nothing readable."""
    try:
        n, off = 0, 0
        while True:
            r = _get("/list_properties", map=CONF_MAP, program=PROGRAM, offset=off, limit=500)
            e = r.get("entries", [])
            n += len(e)
            if len(e) < 500 or off + 500 >= r.get("total", 0):
                return n
            off += 500
    except urllib.error.HTTPError:
        pass                            # endpoint missing -> fall through to bookmarks
    except OSError:
        return None
    try:
        r = _get("/list_bookmarks", category="CONFORMANCE", program=PROGRAM)
        return len(r.get("bookmarks") or r.get("entries") or [])
    except OSError:
        return None


def export(write_mirror: bool = False) -> int:
    recs, off = [], 0
    try:
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
        store = "property map"
    except urllib.error.HTTPError:
        # deployed plugin has no property endpoints -- the store is CONFORMANCE bookmarks
        r = _get("/list_bookmarks", category="CONFORMANCE", program=PROGRAM)
        for b in (r.get("bookmarks") or r.get("entries") or []):
            try:
                rec = json.loads(b.get("comment") or "")
                a = str(b.get("address", "")).lower()
                rec["address"] = a if a.startswith("0x") else "0x" + a
                recs.append(rec)
            except (json.JSONDecodeError, TypeError):
                pass
        store = "CONFORMANCE bookmarks"
    print(f"read {len(recs)} Conf records back from Ghidra {store} (round-trip proof)")
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
