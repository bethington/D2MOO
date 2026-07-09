#!/usr/bin/env python3
"""profiler_feed.py -- stage 0 of conformance/CONFORMANCE_WORKFLOW.md.

Turn the live dynamic profiler's real hit counts into an OBJECTIVE, data-driven
conformance backlog (DYNAMIC_PROFILER_PLAN.md Phase 4). Reads the D2Debugger
profiler over :8790, joins each function with its proof status, ranks the
UN-PORTED functions by how often PD2 actually executed them, and:

  * writes conformance/profiler/hot_backlog.json (the durable ranked list), and
  * optionally --pin's the top-N into fun-doc's priority_queue.json so the next
    `fun_doc.py --port` / run_conformance_batch.py run documents+ports the
    hottest functions first.

Precondition: the game is running AND its subsystems are instrumented
(d2dbg_instrument_all, or the "Instrument All" button) AND a play session has
populated counters. Un-instrumented / unplayed => all-zero hits => empty backlog.

Usage:
    python profiler_feed.py --top 20              # print the hot backlog
    python profiler_feed.py --top 20 --pin        # + pin into fun-doc's queue
    python profiler_feed.py --subsystem STAT --top 10
    python profiler_feed.py --include-ported       # don't filter out hasEquiv fns
"""
from __future__ import annotations

import argparse
import http.client
import json
import os
from pathlib import Path
from urllib.parse import urlparse

HERE = Path(__file__).resolve().parent
D2MOO_REPO = HERE.parent.parent
PROVEN_REGISTRY = D2MOO_REPO / "conformance" / "proven_functions.jsonl"
BACKLOG_OUT = D2MOO_REPO / "conformance" / "profiler" / "hot_backlog.json"
ORACLE_URL = os.environ.get("D2DBG_MCP_URL", "http://127.0.0.1:8790")
D2COMMON_BASE = 0x6FD50000

# fun-doc keys functions as "<ghidra_program_path>::<addr_hex_no_0x>". Override
# the program path via env if fun-doc's project folder differs.
FUNDOC_PROGRAM_PATH = os.environ.get("FUNDOC_GHIDRA_PROGRAM", "/Mods/PD2-S12/D2Common.dll")
FUNDOC_DIR = Path(os.environ.get("FUNDOC_DIR", r"C:\Users\benam\source\mcp\ghidra-mcp\fun-doc"))
PRIORITY_QUEUE = FUNDOC_DIR / "priority_queue.json"


def _get(path: str, timeout: int = 15) -> dict:
    u = urlparse(ORACLE_URL)
    conn = http.client.HTTPConnection(u.hostname, u.port or 8790, timeout=timeout)
    try:
        conn.request("GET", path)
        return json.loads(conn.getresponse().read().decode("utf-8", "replace"))
    finally:
        conn.close()


def _load_proven() -> set[str]:
    """Names already carrying a CONF_ rung (any) -- so the backlog excludes them."""
    if not PROVEN_REGISTRY.exists():
        return set()
    names = set()
    for line in PROVEN_REGISTRY.read_text(encoding="utf-8").splitlines():
        if line.strip():
            try:
                names.add(json.loads(line)["name"])
            except (json.JSONDecodeError, KeyError):
                pass
    return names


def collect(subsystem: str | None) -> list[dict]:
    """Pull every profiled function (or one subsystem) with live hits + proof
    status. Returns rows sorted by hits desc."""
    subs = _get("/profiler/subsystems")
    if not subs.get("ok"):
        raise SystemExit(f"[fatal] profiler unreachable/te: {subs.get('error', subs)}")
    names = [s["name"] for s in subs.get("subsystems", [])
             if subsystem is None or s["name"] == subsystem]
    proven = _load_proven()
    rows: list[dict] = []
    for sub in names:
        fns = _get(f"/profiler/functions/{sub}")
        if not fns.get("ok"):
            continue
        for f in fns.get("functions", []):
            addr = D2COMMON_BASE + int(f["offset"])
            rows.append({
                "name": f["name"],
                "subsystem": sub,
                "ordinal": f.get("ordinal"),
                "offset": int(f["offset"]),
                "address": f"0x{addr:x}",
                "hits": int(f.get("hits", 0)),
                "has_dispatcher": bool(f.get("hasEquiv")),
                "proven": f["name"] in proven,
                "funckey": f"{FUNDOC_PROGRAM_PATH}::{addr:x}",
            })
    rows.sort(key=lambda r: r["hits"], reverse=True)
    return rows


def pin_into_queue(funckeys: list[str]) -> dict:
    """Prepend funckeys to fun-doc's priority_queue.json `pinned` (dedup, keep
    order). Best-effort; the queue file must exist."""
    if not PRIORITY_QUEUE.exists():
        return {"ok": False, "error": f"queue not found: {PRIORITY_QUEUE}"}
    q = json.loads(PRIORITY_QUEUE.read_text(encoding="utf-8"))
    existing = list(q.get("pinned", []))
    seen = set(existing)
    new = [k for k in funckeys if k not in seen]
    q["pinned"] = new + existing            # hot ones first
    PRIORITY_QUEUE.write_text(json.dumps(q, indent=2), encoding="utf-8")
    return {"ok": True, "added": len(new), "already": len(funckeys) - len(new)}


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--top", type=int, default=20, help="how many to show/pin")
    ap.add_argument("--subsystem", help="restrict to one subsystem")
    ap.add_argument("--include-ported", action="store_true",
                    help="include functions that already have a dispatcher/proof")
    ap.add_argument("--pin", action="store_true", help="pin the top-N into fun-doc's queue")
    ap.add_argument("--dump-all", action="store_true",
                    help="write EVERY function's hits (ported+un-ported) to a durable "
                         "snapshot, so the live counters aren't lost on the next restart")
    args = ap.parse_args()

    rows = collect(args.subsystem)
    total_hits = sum(r["hits"] for r in rows)

    if args.dump_all:
        import datetime
        stamp = datetime.date.today().isoformat()
        dump = D2MOO_REPO / "conformance" / "profiler" / f"hit_profile_{stamp}.json"
        dump.write_text(json.dumps(
            {"captured": stamp, "total_profiled_hits": total_hits,
             "count": len(rows), "functions": rows}, indent=2), encoding="utf-8")
        print(f"[dump-all] {len(rows)} functions ({total_hits:,} total hits) -> {dump}")
    if not args.include_ported:
        rows = [r for r in rows if not r["has_dispatcher"] and not r["proven"]]
    hot = [r for r in rows if r["hits"] > 0][:args.top]

    # durable artifact -- the orchestrator / a human can read this without the game
    BACKLOG_OUT.write_text(json.dumps(
        {"total_profiled_hits": total_hits, "count": len(hot), "backlog": hot},
        indent=2), encoding="utf-8")

    print(f"{'function':<44}{'subsystem':<12}{'hits':>16}  {'ord':>6}")
    print("-" * 84)
    for r in hot:
        print(f"{r['name'][:43]:<44}{r['subsystem']:<12}{r['hits']:>16,}  {str(r['ordinal']):>6}")
    print("-" * 84)
    print(f"top {len(hot)} un-ported by real hits  |  backlog -> {BACKLOG_OUT}")

    if args.pin:
        res = pin_into_queue([r["funckey"] for r in hot])
        if res.get("ok"):
            print(f"[pin] added {res['added']} to fun-doc queue ({res['already']} already pinned)")
        else:
            print(f"[pin] FAILED: {res.get('error')}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
