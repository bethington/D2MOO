#!/usr/bin/env python3
"""freeze_corpus.py -- capture REAL game ground truth into a CONF_REGRESSION corpus.

The bridge from LIVE proving to the OFFLINE regression suite. Given a coord-family
dispatcher (by name or index) and a set of input vectors, it calls the live
D2Debugger oracle (:8790 /call), reads the REAL original DLL's outputs, and writes
(or merges into) conformance/regression/<name>.cases.json with those outputs
frozen. gen_offline_tests.py then turns that corpus into an offline doctest.

So the ground truth in the offline suite is never hand-typed -- it is captured
from the running game and frozen. Re-freeze any time to refresh against a new
game build.

Examples:
    # freeze the whole coord family with a real+synthetic vector spread
    python freeze_corpus.py --dispatcher all --out coord_family.cases.json

    # freeze one function with vectors from an existing spec.json's inputs
    python freeze_corpus.py --dispatcher DUNGEON_GameToClientCoords \
        --vectors-from ../vectors/coord_gametoclient.spec.json

Only the coord2 ABI (void __stdcall(int*,int*)) is wired here today, matching the
live /call oracle; the general /oracle ABI freeze is the next extension.
"""
from __future__ import annotations

import argparse
import http.client
import json
import os
from urllib.parse import urlparse

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
CORPUS_DIR = os.path.join(ROOT, "conformance", "regression")
DEFAULT_URL = os.environ.get("D2DBG_MCP_URL", "http://127.0.0.1:8790")

# A real+synthetic spread: gameplay-plausible coords, then boundary hardening
# (INT16 extremes, negative-delta sign stress). src tags flow into the corpus.
DEFAULT_VECTORS = [
    ({"x": 0, "y": 0}, "real"), ({"x": 1, "y": 0}, "real"),
    ({"x": 0, "y": 1}, "real"), ({"x": 5, "y": 3}, "real"),
    ({"x": 10, "y": -4}, "real"), ({"x": -7, "y": 2}, "real"),
    ({"x": 3, "y": 7}, "real"), ({"x": -2, "y": 4}, "real"),
    ({"x": 100, "y": 20}, "real"),
    ({"x": 32767, "y": -32768}, "synthetic"), ({"x": -32768, "y": 32767}, "synthetic"),
    ({"x": 2000, "y": -2000}, "synthetic"), ({"x": -1, "y": -2}, "synthetic"),
]


def http_json(url, method, path, body=None):
    u = urlparse(url)
    conn = http.client.HTTPConnection(u.hostname, u.port or 8790, timeout=30)
    payload = json.dumps(body).encode() if body is not None else None
    headers = {"Content-Type": "application/json"} if payload else {}
    try:
        conn.request(method, path, body=payload, headers=headers)
        return json.loads(conn.getresponse().read().decode("utf-8", "replace"))
    finally:
        conn.close()


def dispatchers(url):
    d = http_json(url, "GET", "/dispatchers")
    if not d.get("ok"):
        raise SystemExit(f"cannot list dispatchers: {d.get('error')}")
    return d["dispatchers"]


def freeze_one(url, index, name, vectors):
    """Call the live oracle and return a function corpus entry with frozen outputs."""
    res = http_json(url, "POST", f"/call/{index}", {"vectors": [v for v, _ in vectors]})
    if not res.get("ok"):
        raise SystemExit(f"oracle error for {name}: {res.get('error')}")
    if not res["allMatch"]:
        # A divergence here means the current reimpl != game; don't freeze a corpus
        # off an unproven function -- fix it first (the whole point of shadow proving).
        print(f"  [WARN] {name}: {res['mismatches']} live mismatch(es) -- "
              f"freezing the ORIGINAL's outputs anyway, but the reimpl is not proven.")
    cases = []
    for (vin, src), r in zip(vectors, res["results"]):
        cases.append({"in": r["in"], "out": r["original"], "src": src})
    return {"fn": name, "cases": cases}


def main():
    ap = argparse.ArgumentParser(description="Freeze live oracle ground truth into a CONF_REGRESSION corpus.")
    ap.add_argument("--dispatcher", default="all",
                    help="'all' for the coord family, or a dispatcher name/index")
    ap.add_argument("--out", default="coord_family.cases.json", help="corpus filename under conformance/regression/")
    ap.add_argument("--vectors-from", help="a spec.json to take input vectors from (else the default spread)")
    ap.add_argument("--url", default=DEFAULT_URL)
    args = ap.parse_args()

    st = http_json(args.url, "GET", "/status")
    if not st.get("ok"):
        raise SystemExit(f"D2Debugger not reachable at {args.url}. Is PD2 running with D2_DEBUGGER=1?")

    vectors = DEFAULT_VECTORS
    if args.vectors_from:
        with open(args.vectors_from, encoding="utf-8") as f:
            spec = json.load(f)
        raw = spec["vectors"] if isinstance(spec, dict) else spec
        vectors = [({"x": int(v["x"]), "y": int(v["y"])}, "real") for v in raw]

    ds = dispatchers(args.url)
    coord_family = {"DUNGEON_GameTileToClientCoords", "DUNGEON_GameTileToSubtileCoords",
                    "DUNGEON_ClientToGameCoords", "DUNGEON_GameToClientCoords",
                    "DUNGEON_GameSubtileToClientCoords"}
    if args.dispatcher == "all":
        targets = [(d["index"], d["name"]) for d in ds if d["name"] in coord_family]
    elif args.dispatcher.lstrip("-").isdigit():
        i = int(args.dispatcher)
        targets = [(d["index"], d["name"]) for d in ds if d["index"] == i]
    else:
        targets = [(d["index"], d["name"]) for d in ds if d["name"] == args.dispatcher]
    if not targets:
        raise SystemExit(f"no dispatcher matched {args.dispatcher!r}")

    functions = []
    for index, name in targets:
        print(f"[freeze] {name} (idx {index}) x {len(vectors)} vectors")
        functions.append(freeze_one(args.url, index, name, vectors))

    corpus = {
        "abi": "coord2",
        "abi_note": "void __stdcall(int* pX, int* pY) -- in/out coordinate pair, the D2MOO coord family",
        "header": "D2Dungeon.h",
        "frozen_from": f"PD2-S12 live oracle via {args.url} /call -- original-DLL ground truth",
        "frozen_date": st.get("date", "captured-live"),
        "note": "CONF_REGRESSION frozen corpus (generated by freeze_corpus.py). Outputs are the "
                "REAL game original's results; replayed OFFLINE against D2MOO's compiled D2Common.",
        "functions": functions,
    }
    out_path = os.path.join(CORPUS_DIR, args.out)
    with open(out_path, "w", encoding="utf-8", newline="\n") as f:
        json.dump(corpus, f, indent=2)
        f.write("\n")
    total = sum(len(fn["cases"]) for fn in functions)
    print(f"[freeze] {len(functions)} fn, {total} cases -> {os.path.relpath(out_path, ROOT)}")
    print("         now run: python conformance/tools/gen_offline_tests.py")


if __name__ == "__main__":
    main()
