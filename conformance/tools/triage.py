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
from pathlib import Path

import scope_tag_library as scope   # reuse the classifier, HTTP helpers, protect-guard

BACKLOG = Path(__file__).resolve().parent.parent / "profiler" / "triage_backlog.json"
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
    args = ap.parse_args()
    return run(apply=args.apply, count=args.count)


if __name__ == "__main__":
    raise SystemExit(main())
