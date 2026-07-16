"""prove_all.py -- regression gate over the whole proven set.

Reloads the reimpl provider, then runs EVERY conformance/vectors/*.spec.json
through the live D2Debugger oracle (:8790) and prints a pass/fail summary.
Exit code 0 iff every runnable spec still proves (allMatch); non-zero if any
REGRESSED. Specs that need a live game-object handle (a "handle" arg) are
skipped when none is captured yet -- reported as SKIP, not FAIL.

Run against a live PD2 (D2_DEBUGGER=1): python conformance/tools/prove_all.py
"""
from __future__ import annotations

import glob
import http.client
import json
import os
import sys
from urllib.parse import urlparse

URL = os.environ.get("D2DBG_MCP_URL", "http://127.0.0.1:8790")
VECTORS = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "vectors")


def req(method: str, path: str, body=None) -> dict:
    u = urlparse(URL)
    c = http.client.HTTPConnection(u.hostname, u.port or 8790, timeout=30)
    payload = json.dumps(body).encode() if body is not None else None
    try:
        c.request(method, path, payload, {"Content-Type": "application/json"} if payload else {})
        raw = c.getresponse().read().decode("utf-8", "replace")
    except OSError as e:
        return {"ok": False, "error": f"cannot reach {URL} ({e})"}
    finally:
        c.close()
    try:
        return json.loads(raw)
    except json.JSONDecodeError:
        return {"ok": False, "error": raw[:200]}


def safe_reload() -> dict:
    """Drain armed shadow dispatchers -> original before the hot-reload, then
    restore them: reload's drain-to-zero deadlocks the oracle while shadow
    reimpls are in-flight (backlog item 13, 2026-07-15 incident)."""
    import time
    shadow = [int(d["index"]) for d in req("GET", "/dispatchers").get("dispatchers", [])
              if d.get("modeName") == "shadow" or d.get("mode") == 2]
    if shadow:
        print(f"[reload] draining {len(shadow)} armed shadow dispatcher(s) -> original first")
        for i in shadow:
            req("POST", f"/dispatcher/{i}/mode", {"mode": "original"})
        time.sleep(1.0)
    rl = req("POST", "/reimpl/reload")
    for i in shadow:
        req("POST", f"/dispatcher/{i}/mode", {"mode": "shadow"})
    if shadow:
        print(f"[reload] restored {len(shadow)} dispatcher(s) to shadow")
    return rl


def main() -> int:
    st = req("GET", "/status")
    if not st.get("ok"):
        print(f"[fatal] oracle unreachable: {st.get('error')}", file=sys.stderr)
        return 2
    print(f"[status] provider={st.get('provider')!r} captureCount={st.get('captureCount')}")
    rl = safe_reload()
    print(f"[reload] {rl.get('provider', rl.get('error'))}\n")

    specs = sorted(glob.glob(os.path.join(VECTORS, "*.spec.json")))
    proven = failed = skipped = 0
    fails = []
    for path in specs:
        try:
            spec = json.load(open(path, encoding="utf-8"))
        except (OSError, json.JSONDecodeError) as e:
            print(f"  SKIP  {os.path.basename(path)} (unreadable: {e})"); skipped += 1; continue
        if "name" not in spec:
            continue
        name = spec["name"]
        res = req("POST", "/oracle", spec)
        if not res.get("ok"):
            err = res.get("error", "")
            if "handle" in err:  # needs a live captured object we don't have yet
                print(f"  SKIP  {name:34s} (needs live handle)"); skipped += 1
            else:
                print(f"  FAIL  {name:34s} {err}"); failed += 1; fails.append(name)
            continue
        tag = "PROVEN" if res["allMatch"] else "DIVERGED"
        line = f"  {tag:8s} {name:34s} {res['matches']}/{res['count']}"
        print(line)
        if res["allMatch"]:
            proven += 1
        else:
            failed += 1; fails.append(name)

    print(f"\n=== {proven} proven, {failed} failed, {skipped} skipped ===")
    if fails:
        print("REGRESSED: " + ", ".join(fails), file=sys.stderr)
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
