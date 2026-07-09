#!/usr/bin/env python3
"""run_conformance_batch.py -- the ordered conformance ladder, one command.

Thin orchestrator for conformance/CONFORMANCE_WORKFLOW.md. It drives a batch of
functions through the automatable stages IN ORDER and prints a per-function
final-rung report. It NEVER reimplements a stage -- it shells out to the existing
stage drivers:

    stage 0  prioritize   profiler_feed.py            -> pin hot functions in the queue
    stage 1  document  \
    stage 2  vectors    }  fun_doc.py --port          -> DOC_*, CONF_VECTORS, CONF_LIVE,
    stage 3  live prove |                                staged shadow dispatcher
    stage 5  stage      /
    stage 4  V1         adversarial_reproof.py         -> vetted:adversarial
    stage 6  battletest battletest_promoter.py (poll)  -> CONF_BATTLETESTED (needs gameplay)
    stage 7  freeze     freeze_corpus + gen_offline_tests -> CONF_REGRESSION (ABI-gated)

Everything after stage 2 needs the elevated game live on :8790. When it's down,
the batch still runs 1->2 and marks the rest `blocked: oracle unreachable`.

Usage:
    python run_conformance_batch.py --top 10
    python run_conformance_batch.py --subsystem DATATBLS --top 8 --stop-at v1
    python run_conformance_batch.py --functions FOO,BAR --stop-at shipping

Stop points (--stop-at): port | live | v1 (default) | shipping.
"""
from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
import http.client
from pathlib import Path
from urllib.parse import urlparse

HERE = Path(__file__).resolve().parent
D2MOO_REPO = HERE.parent.parent
FUNDOC_DIR = Path(os.environ.get(
    "FUNDOC_DIR", r"C:\Users\benam\source\mcp\ghidra-mcp\fun-doc"))
PROVEN_REGISTRY = D2MOO_REPO / "conformance" / "proven_functions.jsonl"
ORACLE_URL = os.environ.get("D2DBG_MCP_URL", "http://127.0.0.1:8790")

STOP_ORDER = ["port", "live", "v1", "shipping"]


def _fundoc_python() -> str:
    """fun-doc scripts (fun_doc.py, adversarial_reproof.py) import fun-doc's own
    modules + claude_agent_sdk, so they must run under fun-doc's venv."""
    for cand in (FUNDOC_DIR / ".venv" / "Scripts" / "python.exe",
                 FUNDOC_DIR / ".venv" / "bin" / "python"):
        if cand.exists():
            return str(cand)
    return sys.executable  # fallback; model-mode V1 may then be unavailable


def oracle_up() -> bool:
    u = urlparse(ORACLE_URL)
    try:
        conn = http.client.HTTPConnection(u.hostname, u.port or 8790, timeout=4)
        conn.request("GET", "/status")
        ok = json.loads(conn.getresponse().read().decode("utf-8", "replace")).get("ok", False)
        conn.close()
        return bool(ok)
    except OSError:
        return False


def _run(cmd: list[str], *, cwd: Path | None = None, env: dict | None = None,
         label: str = "") -> int:
    """Stream a subprocess; return its exit code. Never raises on non-zero."""
    print(f"\n=== {label or ' '.join(cmd[:2])} ===", flush=True)
    e = dict(os.environ)
    if env:
        e.update(env)
    try:
        return subprocess.call(cmd, cwd=str(cwd) if cwd else None, env=e)
    except OSError as ex:
        print(f"  [error] could not launch: {ex}")
        return 127


def _load_registry() -> list[dict]:
    if not PROVEN_REGISTRY.exists():
        return []
    return [json.loads(l) for l in PROVEN_REGISTRY.read_text(encoding="utf-8").splitlines()
            if l.strip()]


# --------------------------------------------------------------------------- #
# Stages
# --------------------------------------------------------------------------- #
def stage0_prioritize(top: int, subsystem: str | None) -> None:
    """Pin the hottest un-proven functions into fun-doc's queue. Best-effort:
    profiler_feed.py is a NEW component (may be absent) and needs live counters."""
    feed = HERE / "profiler_feed.py"
    if not feed.exists():
        print("[stage0] profiler_feed.py not present yet -- using fun-doc's existing "
              "priority order (xref+leaf). Build the feed to rank by real hits.")
        return
    cmd = [sys.executable, str(feed), "--top", str(top), "--pin"]
    if subsystem:
        cmd += ["--subsystem", subsystem]
    _run(cmd, label="stage0 prioritize (profiler_feed)")


def stage123_port(count: int, provider: str | None, model: str | None,
                  stop_at: str, names: list[str] | None) -> None:
    """Document + port + live-prove + stage, TARGETED at the hot backlog (or an
    explicit name list) via port_targets.py -- which drives process_port_candidate
    directly by address, bypassing fun_doc's auto-selector (that only picks
    already-documented low-address leaves, and swept oracle-crashing string libs).
    FUNDOC_DOC_TAGS stamps the DOC_ axis; live-prove/shadow gated by stop_at."""
    py = _fundoc_python()
    cmd = [py, "port_targets.py"]
    if names:
        cmd += ["--names", ",".join(names)]
    else:
        cmd += ["--backlog", "--count", str(count)]
    if provider:
        cmd += ["--provider", provider]
    if model:
        cmd += ["--model", model]
    # port_targets uses setdefault, so these env values win when set explicitly.
    env = {
        "FUNDOC_DOC_TAGS": "1",
        "FUNDOC_LIVE_PROVE": "1" if STOP_ORDER.index(stop_at) >= STOP_ORDER.index("live") else "0",
        "FUNDOC_SHADOW_PROMOTE": "1" if stop_at == "shipping" else "0",
    }
    _run(cmd, cwd=FUNDOC_DIR, env=env, label="stage1-3 document+port+live-prove (targeted)")


def stage4_v1(use_model: bool) -> None:
    """Independent adversarial re-proof of every CONF_LIVE row -> vetted:adversarial."""
    py = _fundoc_python()
    cmd = [py, "adversarial_reproof.py", "--all"]
    if not use_model:
        cmd.append("--no-model")
    _run(cmd, cwd=FUNDOC_DIR, label="stage4 V1 adversarial re-proof")


def stage6_battletest() -> None:
    """One poll of the shadow counters -> promote any CONF_LIVE past the volume bar.
    (Real promotion needs prior gameplay; this just harvests what's accrued.)"""
    promoter = FUNDOC_DIR / "battletest_promoter.py"
    if not promoter.exists():
        print("[stage6] battletest_promoter.py not found -- skipping")
        return
    _run([sys.executable, str(promoter)], label="stage6 battletest poll")


def stage7_freeze() -> None:
    """Freeze proven functions into the offline CONF_REGRESSION suite where the ABI
    is supported (coord2 today; scalar is a NEW component). Best-effort."""
    print("\n=== stage7 freeze -> CONF_REGRESSION ===")
    print("  [note] scalar-ABI freeze is a NEW component (see CONFORMANCE_WORKFLOW.md). "
          "Only coord2-ABI functions freeze today; run freeze_corpus.py + "
          "gen_offline_tests.py manually for those until the scalar emitter lands.")


# --------------------------------------------------------------------------- #
# Report
# --------------------------------------------------------------------------- #
def report(names: list[str] | None) -> None:
    rows = _load_registry()
    if names:
        want = set(names)
        rows = [r for r in rows if r.get("name") in want]
    print("\n" + "=" * 72)
    print(f"{'function':<38} {'CONF':<17} {'vetted':<12} note")
    print("-" * 72)
    for r in sorted(rows, key=lambda r: (r.get("conf", ""), r.get("name", ""))):
        note = ""
        if r.get("needs_review"):
            note = "NEEDS REVIEW (V1 diverged)"
        elif r.get("orig_regs"):
            note = "register-explicit (Class D needed for shadow/freeze)"
        print(f"{r.get('name', '?'):<38} {r.get('conf', '-'):<17} "
              f"{str(r.get('vetted', '-')):<12} {note}")
    print("=" * 72)
    print(f"{len(rows)} rows.  Full ladder: conformance/CONFORMANCE_WORKFLOW.md")


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    sel = ap.add_mutually_exclusive_group()
    sel.add_argument("--top", type=int, help="take the top-N hot un-proven functions")
    sel.add_argument("--functions", help="comma-separated explicit function names")
    ap.add_argument("--subsystem", help="restrict the hot-list to one subsystem (e.g. DATATBLS)")
    ap.add_argument("--stop-at", choices=STOP_ORDER, default="v1",
                    help="how far to push each function (default: v1)")
    ap.add_argument("--provider", help="fun-doc AI provider override")
    ap.add_argument("--model", help="model override")
    ap.add_argument("--no-model", action="store_true", help="V1 algorithmic floor only")
    ap.add_argument("--report-only", action="store_true", help="just print the rung report")
    args = ap.parse_args()

    if args.report_only:
        report(args.functions.split(",") if args.functions else None)
        return 0

    count = args.top or (len(args.functions.split(",")) if args.functions else 1)
    live = oracle_up()
    print(f"[batch] oracle {'UP' if live else 'DOWN'} @ {ORACLE_URL} | "
          f"stop-at={args.stop_at} | count={count}")
    if not live and STOP_ORDER.index(args.stop_at) >= STOP_ORDER.index("live"):
        print("[batch] game is DOWN -- stages 3+ (live prove, V1, battletest, freeze) will "
              "be blocked. Launch it (./D2MOO-Tool.ps1), then d2dbg_reload_provider + "
              "set_all_modes shadow. Running the offline stages (1-2) anyway.")

    # stage 0 -- prioritize (only when selecting by hotness)
    if args.top:
        stage0_prioritize(args.top, args.subsystem)

    # stages 1-3 (+5 if shipping) -- document, port, live-prove, stage
    names = args.functions.split(",") if args.functions else None
    stage123_port(count, args.provider, args.model, args.stop_at, names)

    # stage 4 -- V1 adversarial (needs live oracle)
    if STOP_ORDER.index(args.stop_at) >= STOP_ORDER.index("v1"):
        if live:
            stage4_v1(use_model=not args.no_model)
        else:
            print("\n[stage4] skipped -- oracle down")

    # stages 6-7 -- battletest poll + freeze (shipping target, needs live + gameplay)
    if args.stop_at == "shipping":
        if live:
            stage6_battletest()
        stage7_freeze()

    report(args.functions.split(",") if args.functions else None)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
