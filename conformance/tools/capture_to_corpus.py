#!/usr/bin/env python3
"""capture_to_corpus.py -- fold live-captured shadow vectors into offline corpora.

The back half of Demo B. When PD2 runs with D2MOO_CAPTURE_VECTORS=1, matching
shadow calls append distinct {real input -> real game output} samples to
conformance/behavioral/captured_vectors.jsonl (see gen_shadow_dispatch.py's
LogMatch). This tool converts those into CONF_REGRESSION corpora
(conformance/regression/*.captured.cases.json) that gen_offline_tests.py compiles
into the offline suite. So real gameplay auto-populates the regression tests.

    python conformance/tools/capture_to_corpus.py            # convert all captured fns
    python conformance/tools/capture_to_corpus.py --fn GetItemRandSeed

OFFLINE-SAFE BOUNDARY (important): a function is trivially replayable offline only
if its inputs are self-contained VALUES. Functions whose args are POINTERS (kind
"ptr") read state behind the pointer; the capture logs the pointer VALUE, which is
meaningless in the offline process. Those are SKIPPED here with a note -- they need
input-STATE capture (snapshot the pointed-to bytes) or an offline fixture (cf.
d2moo_loader_harness for data tables). Pure scalar-in/scalar-ret functions convert
cleanly.
"""
from __future__ import annotations

import argparse
import json
import os
from collections import OrderedDict

HERE = os.path.dirname(os.path.abspath(__file__))
CONF = os.path.dirname(HERE)                       # conformance/
CAPTURED = os.path.join(CONF, "behavioral", "captured_vectors.jsonl")
MANIFEST = os.path.join(CONF, "shadow_manifest.json")
CORPUS_DIR = os.path.join(CONF, "regression")

SCALAR_KINDS = {"i32", "u32"}
# Best-effort header per function for the generated #include. Extend as functions
# are added; unknown -> "<SET-HEADER>" with a warning so it is obvious in review.
HEADERS = {
    "DUNGEON_GetTownLevelIdFromActNo": "D2Dungeon.h",
    "GetDataTableRowEntryCount": "D2DataTbls.h",
}


def load_manifest():
    with open(MANIFEST, encoding="utf-8") as f:
        m = json.load(f)
    return {e["name"]: e for e in m["entries"]}


def main():
    ap = argparse.ArgumentParser(description="Convert captured shadow vectors to offline corpora.")
    ap.add_argument("--in", dest="inp", default=CAPTURED)
    ap.add_argument("--fn", help="only convert this function")
    args = ap.parse_args()

    if not os.path.exists(args.inp):
        raise SystemExit(f"no capture file at {args.inp}. Run PD2 with D2MOO_CAPTURE_VECTORS=1 "
                         f"in shadow mode first.")
    manifest = load_manifest()

    # Group captured records by fn, dedup on the input tuple.
    by_fn: dict[str, "OrderedDict[tuple, dict]"] = {}
    with open(args.inp, encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            rec = json.loads(line)
            fn = rec.get("fn")
            if not fn or "ret" not in rec:      # only scalar-ret records here (LogMatch)
                continue
            if args.fn and fn != args.fn:
                continue
            key = tuple(rec.get("args", []))
            by_fn.setdefault(fn, OrderedDict())[key] = rec

    if not by_fn:
        raise SystemExit("no convertible (scalar-ret) records found in the capture file.")

    written = skipped = 0
    for fn, recs in by_fn.items():
        e = manifest.get(fn)
        if not e:
            print(f"[skip] {fn}: not in shadow_manifest.json"); skipped += 1; continue
        kinds = e.get("args", [])
        if not all(k in SCALAR_KINDS for k in kinds):
            print(f"[skip] {fn}: arg kinds {kinds} not all scalar -- needs input-STATE capture "
                  f"or an offline fixture, not a value replay"); skipped += 1; continue

        argnames = [f"a{i}" for i in range(len(kinds))]
        cases = []
        for rec in recs.values():
            vin = {name: int(v) for name, v in zip(argnames, rec.get("args", []))}
            cases.append({"in": vin, "ret": int(rec["ret"]), "src": rec.get("src", "real")})
        header = HEADERS.get(fn, "<SET-HEADER>")
        if header == "<SET-HEADER>":
            print(f"[warn] {fn}: no header mapping -- edit HEADERS in capture_to_corpus.py "
                  f"or fix the .cases.json 'header' field before building")
        corpus = {
            "abi": "scalar_ret",
            "header": header,
            "frozen_from": f"live SHADOW capture (D2MOO_CAPTURE_VECTORS=1) -> {os.path.basename(args.inp)}",
            "frozen_date": "captured-live",
            "note": f"CONF_REGRESSION corpus auto-built from real gameplay capture of {fn}.",
            "functions": [{"fn": fn, "args": argnames, "cases": cases}],
        }
        out = os.path.join(CORPUS_DIR, f"{fn}.captured.cases.json")
        with open(out, "w", encoding="utf-8", newline="\n") as f:
            json.dump(corpus, f, indent=2)
            f.write("\n")
        print(f"[ok] {fn}: {len(cases)} case(s) -> {os.path.relpath(out, CONF)}")
        written += 1

    print(f"\n{written} corpus file(s) written, {skipped} fn skipped. "
          f"Now run: python conformance/tools/gen_offline_tests.py")


if __name__ == "__main__":
    main()
