"""prove_candidate.py -- headless proving driver for the graduated-conformance
pipeline (WS-5/WS-6 of ../GRADUATED_CONFORMANCE_PIPELINE_PLAN.md).

Runs the full autonomous proving loop against a LIVE Project Diablo 2 process
(via the D2Debugger MCP HTTP control surface on 127.0.0.1:8790) in ONE command:

    (optional) build the reimpl-provider  ->  stage it  ->  hot-reload it live
    ->  direct-call oracle on chosen input vectors  ->  match/mismatch verdict

Exit code 0 iff every vector matched (allMatch); non-zero otherwise. This is the
reusable seam a fun-doc worker (or CI, or a human) calls after drafting a C++
equivalent, to PROVE it matches the game with no restart and no ImGui clicks.

Examples:
    # prove dispatcher by name using an auto vector spread, rebuilding first
    python prove_candidate.py --dispatcher DUNGEON_GameToClientCoords --build --vectors auto

    # prove by index using a vector file, against an already-loaded provider
    python prove_candidate.py --dispatcher 3 --vectors my_vectors.json

Vector file format (coord-family ABI): {"vectors": [{"x": 0, "y": 0}, ...]}
or a bare list [{"x":..,"y":..}, ...].
"""
from __future__ import annotations

import argparse
import http.client
import json
import os
import subprocess
import sys
from urllib.parse import urlparse

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))  # repo root
DEFAULT_BUILD = os.path.join(ROOT, "build-1.13c")
DEFAULT_URL = os.environ.get("D2DBG_MCP_URL", "http://127.0.0.1:8790")


def http_json(url: str, method: str, path: str, body: dict | None = None) -> dict:
    u = urlparse(url)
    conn = http.client.HTTPConnection(u.hostname, u.port or 8790, timeout=30)
    payload = json.dumps(body).encode() if body is not None else None
    headers = {"Content-Type": "application/json"} if payload is not None else {}
    try:
        conn.request(method, path, body=payload, headers=headers)
        r = conn.getresponse()
        raw = r.read().decode("utf-8", "replace")
    finally:
        conn.close()
    try:
        return json.loads(raw)
    except json.JSONDecodeError:
        return {"ok": False, "error": f"non-JSON: {raw[:200]}"}


def auto_vectors() -> list[dict]:
    """A deterministic spread that stresses the coord family: origin, small
    positives, negatives (arith-shift sign behavior), and INT16 extremes."""
    xs = [0, 1, 5, 100, -7, -100, 32767, -32768, 999, -999, 12345, -54321]
    ys = [0, 1, 3, 40, 2, -100, -32768, 32767, -999, 999, -6789, 6789]
    return [{"x": x, "y": y} for x, y in zip(xs, ys)]


def load_vectors(spec: str) -> list[dict]:
    if spec == "auto":
        return auto_vectors()
    with open(spec, encoding="utf-8") as f:
        data = json.load(f)
    vecs = data["vectors"] if isinstance(data, dict) else data
    return [{"x": int(v["x"]), "y": int(v["y"])} for v in vecs]


def resolve_index(url: str, dispatcher: str) -> tuple[int, str]:
    """Accept a numeric index or a dispatcher name; return (index, name)."""
    listing = http_json(url, "GET", "/dispatchers")
    if not listing.get("ok"):
        raise SystemExit(f"cannot list dispatchers: {listing.get('error')}")
    ds = listing["dispatchers"]
    if dispatcher.lstrip("-").isdigit():
        i = int(dispatcher)
        for d in ds:
            if d["index"] == i:
                return i, d["name"]
        raise SystemExit(f"index {i} out of range (0..{len(ds) - 1})")
    for d in ds:
        if d["name"] == dispatcher:
            return d["index"], d["name"]
    names = ", ".join(d["name"] for d in ds)
    raise SystemExit(f"no dispatcher named {dispatcher!r}. Have: {names}")


def build_and_stage(build_dir: str, config: str) -> None:
    # Reconfigure FIRST so a brand-new candidates/*.cpp (or a new
    # D2MOO_REIMPL_EXPORT marker added to an existing one) is actually picked up:
    # CMake's CONFIGURE_DEPENDS re-globs + regenerates the .def only on a
    # configure, and a plain `cmake --build` after such an edit can compile the
    # object but link with a STALE .def -> "reimpl not found" at prove time (the
    # glob caveat documented in source/D2Debugger/CMakeLists.txt). A reconfigure
    # is cheap and makes the automated live path robust; ignore its failure and
    # let the build surface any real error.
    print(f"[configure] cmake -S {ROOT} -B {build_dir}")
    subprocess.run(["cmake", "-S", ROOT, "-B", build_dir], check=False)
    print(f"[build] cmake --build {build_dir} --target D2MOO_ReimplProvider")
    subprocess.run(
        ["cmake", "--build", build_dir, "--config", config,
         "--target", "D2MOO_ReimplProvider"],
        check=True,
    )
    built = os.path.join(build_dir, "source", "D2Debugger", config,
                         "D2MOO_ReimplProvider.dll")
    if not os.path.exists(built):
        # fall back to a search (output dir can vary by generator)
        for dirpath, _, files in os.walk(build_dir):
            if "D2MOO_ReimplProvider.dll" in files and os.sep + "patch" + os.sep not in dirpath:
                built = os.path.join(dirpath, "D2MOO_ReimplProvider.dll")
                break
    dst = os.path.join(build_dir, "patch", "D2MOO_ReimplProvider.dll")
    import shutil
    shutil.copyfile(built, dst)
    print(f"[stage] {built} -> {dst}")


def main() -> int:
    ap = argparse.ArgumentParser(description="Prove a reimpl candidate against live PD2.")
    ap.add_argument("--dispatcher", help="coord-mode: dispatcher index or name (uses /call)")
    ap.add_argument("--spec", help="general-mode: path to an ABI spec JSON (uses /oracle) "
                    "-- proves ANY function, see conformance/vectors/*.spec.json")
    ap.add_argument("--vectors", default="auto", help="coord-mode: 'auto', or a vectors JSON")
    ap.add_argument("--build", action="store_true", help="rebuild + stage the provider first")
    ap.add_argument("--reload", dest="reload", action="store_true", default=True,
                    help="hot-reload the provider before proving (default on)")
    ap.add_argument("--no-reload", dest="reload", action="store_false")
    ap.add_argument("--url", default=DEFAULT_URL)
    ap.add_argument("--build-dir", default=DEFAULT_BUILD)
    ap.add_argument("--config", default="Release")
    ap.add_argument("--json", action="store_true", help="emit the raw oracle JSON")
    args = ap.parse_args()

    st = http_json(args.url, "GET", "/status")
    if not st.get("ok"):
        print(f"[fatal] D2Debugger not reachable at {args.url}: {st.get('error')}",
              file=sys.stderr)
        print("        Is PD2 running with D2_DEBUGGER=1?", file=sys.stderr)
        return 2
    print(f"[status] bridge={st['bridge']} dispatchers={st['dispatchers']} "
          f"provider={st['provider']!r}")

    if not args.dispatcher and not args.spec:
        print("[fatal] pass --dispatcher (coord /call mode) or --spec (general /oracle mode)",
              file=sys.stderr)
        return 2

    if args.build:
        build_and_stage(args.build_dir, args.config)

    if args.reload:
        rl = http_json(args.url, "POST", "/reimpl/reload")
        print(f"[reload] {rl.get('provider', rl.get('error'))}")

    # General mode: a full ABI spec -> POST /oracle. Proves ANY function.
    if args.spec:
        with open(args.spec, encoding="utf-8") as f:
            spec = json.load(f)
        name = spec.get("name", "?")
        nvec = len(spec.get("vectors", []))
        print(f"[oracle] {name} (general ABI, {spec.get('callconv', 'stdcall')}) x {nvec} vectors")
        res = http_json(args.url, "POST", "/oracle", spec)
        if not res.get("ok"):
            print(f"[fatal] oracle error: {res.get('error')}", file=sys.stderr)
            return 2
        if args.json:
            print(json.dumps(res, indent=2))
        else:
            for i, r in enumerate(res["results"]):
                if not r["match"]:
                    print(f"  MISMATCH vector[{i}] ret={r['ret']} bufs={r['bufs']}")
        verdict = "PROVEN" if res["allMatch"] else "DIVERGED"
        print(f"[{verdict}] {name}: {res['matches']}/{res['count']} match, "
              f"{res['mismatches']} mismatch")
        return 0 if res["allMatch"] else 1

    index, name = resolve_index(args.url, args.dispatcher)
    vecs = load_vectors(args.vectors)
    print(f"[oracle] {name} (idx {index}) x {len(vecs)} vectors")

    res = http_json(args.url, "POST", f"/call/{index}", {"vectors": vecs})
    if not res.get("ok"):
        print(f"[fatal] oracle error: {res.get('error')}", file=sys.stderr)
        return 2

    if args.json:
        print(json.dumps(res, indent=2))
    else:
        for r in res["results"]:
            if not r["match"]:
                i, o, re_ = r["in"], r["original"], r["reimpl"]
                print(f"  MISMATCH in=({i['x']},{i['y']}) "
                      f"orig=({o['x']},{o['y']}) reimpl=({re_['x']},{re_['y']})")

    verdict = "PROVEN" if res["allMatch"] else "DIVERGED"
    print(f"[{verdict}] {name}: {res['matches']}/{res['count']} match, "
          f"{res['mismatches']} mismatch")
    return 0 if res["allMatch"] else 1


if __name__ == "__main__":
    sys.exit(main())
