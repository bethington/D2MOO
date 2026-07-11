#!/usr/bin/env python3
"""triage.py -- the INTAKE gate (dashboard 'Triage N' action).

A function with NO tag has never been evaluated. Triage is the cheap first touch that
moves it out of the intake pool, WITHOUT any LLM cost:

  1. enumerate the never-evaluated set = defined functions carrying no DOC_/CONF_/LIB_ tag;
  2. SCOPE-classify each (reusing scope_tag_library's conservative name classifier):
       library/runtime -> tag LIB_<which> + Scope property  -> excluded, done forever;
       game logic      -> stays in scope;
  3. ENQUEUE the in-scope remainder into conformance/profiler/triage_backlog.json, which
     the doc + prove workflows drain over time (run_conformance_batch --functions ...,
     the documentation worker). The DOC_/CONF_ rungs then fill via the normal pipeline --
     'triage assigns scope; the workflows assign rungs.'

  python triage.py            # DRY RUN -- classify + report, write nothing
  python triage.py --apply     # tag LIB_, write the backlog, save the program
  python triage.py --count N   # limit how many untriaged to process this pass

Env: GHIDRA_SERVER_URL (default http://127.0.0.1:8089), FUNDOC_GHIDRA_PROGRAM.
"""
from __future__ import annotations
import argparse
import json
import re
from pathlib import Path

import scope_tag_library as scope   # reuse the classifier, HTTP helpers, protect-guard

BACKLOG = Path(__file__).resolve().parent.parent / "profiler" / "triage_backlog.json"
GLOB_BACKLOG = Path(__file__).resolve().parent.parent / "profiler" / "triage_globals_backlog.json"
# list_globals text line: "NAME @ ADDR [Kind] (type)"; image range excludes TIB/PEB/stack labels
_GLOB_LINE = re.compile(r"^(?P<name>\S+)\s+@\s+(?P<addr>[0-9a-fA-F]+)\s+\[[^\]]*\]\s+\((?P<type>[^)]*)\)")
_IMG_LO, _IMG_HI = 0x6F000000, 0x70000000


def _all_globals() -> list[tuple[str, str]]:
    """[(name, 0xaddr), ...] of in-image globals from /list_globals. Skips out-of-image OS
    labels and Ordinal_ export aliases -- the same set the dashboard's Globals Inventory shows."""
    try:
        txt = scope._get("/list_globals", program=scope.PROGRAM, limit=100000)
    except OSError:
        return []
    out = []
    for ln in txt.splitlines():
        m = _GLOB_LINE.match(ln.strip())
        if not m:
            continue
        a = int(m.group("addr"), 16)
        if not (_IMG_LO <= a < _IMG_HI) or m.group("name").startswith("Ordinal_"):
            continue
        out.append((m.group("name"), "0x%08x" % a))
    return out


def _prop_map_addrs(map_name: str) -> set[str]:
    """Data addresses carrying any value in the given property map (`Scope` = excluded,
    `Doc` = already-documented -> protected from exclusion)."""
    try:
        r = json.loads(scope._get("/list_properties", map=map_name, program=scope.PROGRAM, limit=100000))
    except (OSError, json.JSONDecodeError):
        return set()
    return {"0x" + str(p.get("address")).lower().lstrip("0x")
            for p in (r.get("entries") or r.get("properties") or []) if p.get("value")}


def run_globals(apply: bool, count: int | None) -> int:
    """Globals intake gate (mirrors run() for data): name-classify each in-image global; those
    matching a library-data pattern get a `Scope` = LIB_* exclusion (data can't carry function
    tags), the rest are enqueued as documentation targets. Conservative -- only LIB_* excludes a
    global (STUB/THUNK/EXTERNAL are code-only dispositions)."""
    globs = _all_globals()
    excluded = _prop_map_addrs("Scope")          # already scoped out by a prior pass
    protected = _prop_map_addrs("Doc")           # already documented -> never auto-exclude
    todo = [(n, a) for n, a in globs if a not in excluded]
    print(f"\n[globals] {len(globs)} in-image globals - {len(excluded)} already scoped "
          f"-> {len(todo)} to triage ({len(protected)} carry a DOC rung, protected)")
    if count:
        todo = todo[:count]
        print(f"  (this pass: first {len(todo)})")

    # Data analog of the function classifier: LIB_CRT = runtime/CRT data, RTTI/VTABLE/STRPOOL/
    # JUMPTABLE = compiler artifacts. All are excluded from the doc denominator, but the Scope
    # value names WHICH -- accounted-for, not silently dropped. Game data classifies to None.
    lib_hits, in_scope = [], []
    total = len(todo)
    for i, (name, addr) in enumerate(todo, 1):
        cat = None if addr in protected else scope.classify_global(name)
        if cat:
            lib_hits.append((name, addr, cat))
            print(f"  [{i}/{total}] {name} @ {addr}  ->  {cat} (exclude, by name)", flush=True)
        else:
            in_scope.append({"name": name, "address": addr})
            print(f"  [{i}/{total}] {name} @ {addr}  ->  in-scope (doc target)", flush=True)

    by_cat: dict[str, int] = {}
    for _n, _a, c in lib_hits:
        by_cat[c] = by_cat.get(c, 0) + 1
    print(f"\n[globals] {'WOULD ' if not apply else ''}triage {total}: "
          f"{len(lib_hits)} excluded (runtime/compiler), {len(in_scope)} game -> doc targets")
    for c in sorted(by_cat):
        print(f"  {c:12} {by_cat[c]}")
    if not apply:
        print(f"[globals] DRY RUN -- nothing written")
        return 0

    if lib_hits:
        try:
            scope._post("/create_property_map", {"name": "Scope", "type": "string", "program": scope.PROGRAM})
        except OSError:
            pass
    for name, addr, t in lib_hits:
        try:
            scope._post("/set_property", {"map": "Scope", "address": addr, "value": t, "program": scope.PROGRAM})
        except OSError as e:
            print(f"  FAIL {name}: {e}", flush=True)

    # enqueue the in-scope globals as documentation targets (the globals-doc worker drains these)
    prior = []
    if GLOB_BACKLOG.exists():
        try:
            prior = json.loads(GLOB_BACKLOG.read_text(encoding="utf-8")).get("in_scope", [])
        except (OSError, json.JSONDecodeError):
            prior = []
    seen = {f["address"] for f in prior}
    merged = prior + [f for f in in_scope if f["address"] not in seen]
    GLOB_BACKLOG.parent.mkdir(parents=True, exist_ok=True)
    GLOB_BACKLOG.write_text(json.dumps({"source": "triage.py", "in_scope": merged}, indent=2) + "\n",
                            encoding="utf-8")
    try:
        scope._post("/save_program", {"program": scope.PROGRAM})
    except OSError:
        print("  [warn] save_program failed -- Scope marks in memory, save Ghidra manually")
    print(f"[globals] excluded {len(lib_hits)} library data; enqueued {len(in_scope)} "
          f"({len(merged)} total) -> {GLOB_BACKLOG.name}")
    return 0
# tag prefixes that mean "already evaluated" (on either axis) or "already scoped"
EVALUATED_PREFIXES = ("DOC_", "CONF_", "LIB_")


def _tagged_addresses() -> set[str]:
    """Every function that already carries a DOC_/CONF_/LIB_ tag -- fetched as SETS
    (one search per tag) rather than per-function, so this stays cheap."""
    tagged: set[str] = set()
    try:
        tags = json.loads(scope._get("/list_function_tags")).get("tags", [])
    except (OSError, json.JSONDecodeError):
        return tagged
    for t in tags:
        name = t.get("name", "")
        if not name.startswith(EVALUATED_PREFIXES) or not t.get("use_count"):
            continue
        try:
            r = json.loads(scope._get("/search_functions_by_tag", tag=name, program=scope.PROGRAM))
        except (OSError, json.JSONDecodeError):
            continue
        for f in r.get("functions", []):
            a = str(f.get("address", "")).lower()
            if a:
                tagged.add("0x" + a)
    return tagged


def run(apply: bool, count: int | None) -> int:
    all_fns = scope._all_functions()                 # [(name, 0xaddr)] defined-local
    tagged = _tagged_addresses()
    untriaged = [(n, a) for n, a in all_fns if a not in tagged]
    print(f"{len(all_fns)} defined functions - {len(tagged)} already evaluated/scoped "
          f"-> {len(untriaged)} never-evaluated")
    if count:
        untriaged = untriaged[:count]
        print(f"  (this pass: first {len(untriaged)})")

    # Three exclusion signals, in order: (1) name/content -> LIB_* (library; content match
    # catches RENAMED library via the fingerprint DB); (2) structure -> STUB / THUNK / EXTERNAL
    # (trivial functions, no real conformance/doc work). Whatever's left is real game code.
    fingerprints = scope.load_fingerprints()
    hashes = scope._bulk_hashes(scope.PROGRAM)       # instruction counts (+ fingerprint match)
    flags = scope._enhanced_flags(scope.PROGRAM)     # isThunk / isExternal
    print(f"  loaded {len(fingerprints)} library fingerprints; hashed {len(hashes)}, "
          f"flags {len(flags)}")

    lib_hits, in_scope = [], []
    total = len(untriaged)
    by_via = {"name": 0, "fingerprint": 0, "structural": 0}
    for i, (name, addr) in enumerate(untriaged, 1):
        tag = scope._classify(name)                  # by name -> LIB_* / STUB / THUNK
        via = "name"
        if not tag and fingerprints:                 # by content -> renamed library
            tag = scope.classify_by_hash(hashes.get(addr), fingerprints)
            if tag:
                via = "fingerprint"
        if not tag:                                  # by structure -> STUB / THUNK / EXTERNAL
            hinfo = hashes.get(addr) or {}
            isth, isext = flags.get(addr, (False, False))
            tag = scope.classify_structural(hinfo.get("instrs"), isth, isext)
            if tag:
                via = "structural"
        if tag:
            lib_hits.append((name, addr, tag))       # excluded (library or trivial)
            by_via[via] += 1
            print(f"  [{i}/{total}] {name} @ {addr}  ->  {tag} (exclude, by {via})", flush=True)
        else:
            in_scope.append({"name": name, "address": addr})
            print(f"  [{i}/{total}] {name} @ {addr}  ->  in-scope (enqueue)", flush=True)

    by_cat: dict[str, int] = {}
    for _n, _a, t in lib_hits:
        by_cat[t] = by_cat.get(t, 0) + 1
    print(f"\n{'WOULD ' if not apply else ''}triage {len(untriaged)}: "
          f"{len(lib_hits)} excluded (library/stub/thunk/external), {len(in_scope)} game -> enqueue")
    print(f"  by detection: {by_via['name']} name, {by_via['fingerprint']} content-fingerprint, "
          f"{by_via['structural']} structural")
    for t in sorted(by_cat):
        print(f"  {t:14} {by_cat[t]}")

    if not apply:
        print(f"\nDRY RUN -- nothing written. Backlog would get {len(in_scope)} in-scope "
              f"functions -> {BACKLOG.name}")
        return 0

    # exclude the library ones (LIB_ tag + Scope property), reusing scope_tag_library
    if lib_hits:
        try:
            scope._post("/create_property_map", {"name": "Scope", "type": "string", "program": scope.PROGRAM})
        except OSError:
            pass
    tagged_lib = 0
    for name, addr, t in lib_hits:
        try:
            scope._post("/add_function_tag", {"function": addr, "tags": t, "program": scope.PROGRAM})
            scope._post("/set_property", {"map": "Scope", "address": addr, "value": t, "program": scope.PROGRAM})
            tagged_lib += 1
            print(f"  tagged LIB_{t}: {name} @ {addr}", flush=True)
        except OSError as e:
            print(f"  FAIL {name}: {e}", flush=True)

    # enqueue the in-scope remainder for the doc + prove workflows
    prior = []
    if BACKLOG.exists():
        try:
            prior = json.loads(BACKLOG.read_text(encoding="utf-8")).get("in_scope", [])
        except (OSError, json.JSONDecodeError):
            prior = []
    seen = {f["address"] for f in prior}
    merged = prior + [f for f in in_scope if f["address"] not in seen]
    BACKLOG.parent.mkdir(parents=True, exist_ok=True)
    BACKLOG.write_text(json.dumps({"source": "triage.py", "in_scope": merged}, indent=2) + "\n",
                       encoding="utf-8")
    try:
        scope._post("/save_program", {"program": scope.PROGRAM})
    except OSError:
        print("  [warn] save_program failed -- LIB_ tags in memory, save Ghidra manually")
    print(f"\nexcluded {tagged_lib} library functions; enqueued {len(in_scope)} in-scope "
          f"({len(merged)} total in backlog) -> {BACKLOG}")
    print("next: the doc worker + run_conformance_batch drain the backlog (rungs fill over time)")
    return 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--apply", action="store_true", help="tag LIB_, write backlog (default dry run)")
    ap.add_argument("--count", type=int, help="limit untriaged processed this pass")
    ap.add_argument("--functions-only", action="store_true", help="skip the globals phase")
    ap.add_argument("--globals-only", action="store_true", help="skip the functions phase")
    args = ap.parse_args()
    rc = 0
    if not args.globals_only:
        rc = run(apply=args.apply, count=args.count)              # functions intake
    if not args.functions_only:
        rc = run_globals(apply=args.apply, count=args.count) or rc  # globals intake
    return rc


if __name__ == "__main__":
    raise SystemExit(main())
