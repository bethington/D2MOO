#!/usr/bin/env python3
"""Review-policy v1 for the proven_*_pending_review pile (backlog #4).

Applies the SHIPPING_PROMOTION_PLAN.md ladder to every function sitting in
`proven_live_pending_review` / `proven_pending_review` in the fun-doc state DB:

AUTO-PROMOTE (port_status -> proven_live, ledger row re-appended with review
fields) when the latest registry row carries evidence BEYOND the drafting
model's own word:
  - battletested:        conf == CONF_BATTLETESTED with 0 shadow divergences
                         (V2: the game's own inputs at volume)
  - vetted_adversarial:  vetted == "adversarial" (V1: independent adversarial
                         vector set passed)
  - strong_synth:        proof_kind in (synth, synth2, prove_spec_discriminating)
                         with passed==total, >=10 vectors, not weak_proof
                         (discriminating-by-construction synthetic object)

HOLD everything else and emit conformance/v1_reproof_queue.json — the work
list for the V1 adversarial re-proof loop (fun-doc/adversarial_reproof.py)
next time the oracle is up. Hold reasons:
  - plain_live_no_vetting: model-chosen vectors only (self-consistency risk,
                           SHIPPING_PROMOTION_PLAN.md risk #1) -> V1 queue
  - low_vectors:           < 10 vectors -> V1 queue
  - weak_proof:            degenerate capture flagged at prove time -> V1 queue
  - delegate_call_through: proof validity rides on the callee; V1 with the
                           delegate envelope -> V1 queue
  - re_prove_queued:       already spot-checked 2026-07-15 and marked for
                           re-proof (weak 0-vec); not double-queued
  - ledger_backfill_needed: static-lane proof never appended a registry row;
                           needs a ledger backfill before any promotion

Promotion here clears the REVIEW queue; it is NOT the V3 shipping gate —
moving code into the shipping reimpl set stays a human decision.

Dry-run by default; --apply writes state.db + ledger + queue file.
"""
import argparse
import datetime as _dt
import json
import sqlite3
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
LEDGER = REPO / "conformance" / "proven_functions.jsonl"
QUEUE_OUT = REPO / "conformance" / "v1_reproof_queue.json"
DEFAULT_STATE_DB = Path(
    "C:/Users/benam/source/mcp/ghidra-mcp/fun-doc/state.db")

PENDING_STATUSES = ("proven_live_pending_review", "proven_pending_review")
STRONG_SYNTH_KINDS = {"synth", "synth2", "prove_spec_discriminating"}
STRONG_SYNTH_MIN_VECTORS = 10
POLICY_VERSION = "review-policy-v1"


def load_latest_ledger_rows():
    latest = {}
    with open(LEDGER, encoding="utf-8") as fh:
        for line in fh:
            line = line.strip()
            if not line:
                continue
            row = json.loads(line)
            latest[row["address"].lower()] = row
    return latest


def decide(row):
    """Return (verdict, rule_or_reason) for a latest ledger row (or None)."""
    if row is None:
        return "hold", "ledger_backfill_needed"
    if row.get("conf") == "CONF_BATTLETESTED" and not row.get("shadow_divergences"):
        return "promote", "battletested"
    if row.get("vetted") == "adversarial":
        return "promote", "vetted_adversarial"
    if (row.get("proof_kind") in STRONG_SYNTH_KINDS
            and row.get("passed") == row.get("total")
            and (row.get("vectors") or 0) >= STRONG_SYNTH_MIN_VECTORS
            and not row.get("weak_proof")):
        return "promote", "strong_synth"
    if row.get("weak_proof"):
        return "hold", "weak_proof"
    if row.get("proof_kind") == "delegate_call_through":
        return "hold", "delegate_call_through"
    if (row.get("vectors") or 0) < STRONG_SYNTH_MIN_VECTORS:
        return "hold", "low_vectors"
    return "hold", "plain_live_no_vetting"


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--state-db", type=Path, default=DEFAULT_STATE_DB)
    ap.add_argument("--apply", action="store_true",
                    help="write state.db + ledger + V1 queue (default: dry-run report)")
    args = ap.parse_args()

    today = _dt.date.today().isoformat()
    latest = load_latest_ledger_rows()

    con = sqlite3.connect(str(args.state_db), timeout=15)
    cur = con.cursor()
    q = ("SELECT program_path, address, name, port_status, port_last_result "
         "FROM functions_workflow WHERE port_status IN (%s)"
         % ",".join("?" * len(PENDING_STATUSES)))
    pending = list(cur.execute(q, PENDING_STATUSES))

    promotions, holds, queue = [], [], []
    for prog, addr, name, status, last_result in pending:
        row = latest.get("0x" + addr.lower())
        verdict, rule = decide(row)
        # Already spot-checked + marked for re-proof on 2026-07-15: keep out
        # of both the promote set and the V1 queue (the re-proof loop owns it).
        if rule == "ledger_backfill_needed" and "reset for re-proof" in (last_result or ""):
            verdict, rule = "hold", "re_prove_queued"
        if verdict == "promote":
            promotions.append((prog, addr, name, rule, row))
        else:
            holds.append((prog, addr, name, rule))
            if rule in ("plain_live_no_vetting", "low_vectors", "weak_proof",
                        "delegate_call_through"):
                queue.append({
                    "address": "0x" + addr.lower(),
                    "name": name,
                    "reason": rule,
                    "vectors": (row or {}).get("vectors"),
                    "proof_kind": (row or {}).get("proof_kind"),
                    "weak_proof": bool((row or {}).get("weak_proof")),
                })

    print(f"pending review: {len(pending)}")
    print(f"AUTO-PROMOTE:   {len(promotions)}")
    for prog, addr, name, rule, _ in promotions:
        print(f"  {addr}  {name:<40s} {rule}")
    hold_counts = {}
    for _, _, _, rule in holds:
        hold_counts[rule] = hold_counts.get(rule, 0) + 1
    print(f"HOLD:           {len(holds)}  {hold_counts}")
    print(f"V1 queue:       {len(queue)} entries -> {QUEUE_OUT.name}")

    if not args.apply:
        print("\ndry-run only; pass --apply to write.")
        return 0

    for prog, addr, name, rule, row in promotions:
        cur.execute(
            "UPDATE functions_workflow SET port_status='proven_live', "
            "port_last_result=? WHERE program_path=? AND address=? "
            "AND port_status IN (?,?)",
            (f"auto-promoted {today}: {rule} ({POLICY_VERSION})",
             prog, addr, *PENDING_STATUSES))
    con.commit()

    with open(LEDGER, "a", encoding="utf-8") as fh:
        for prog, addr, name, rule, row in promotions:
            out = dict(row)
            out["reviewed"] = "auto"
            out["review_rule"] = rule
            out["review_date"] = today
            out["review_policy"] = POLICY_VERSION
            fh.write(json.dumps(out) + "\n")

    QUEUE_OUT.write_text(json.dumps({
        "generated": today,
        "policy": POLICY_VERSION,
        "consumer": "fun-doc/adversarial_reproof.py (--function <name> per entry, "
                    "or --all when the oracle is up)",
        "entries": sorted(queue, key=lambda e: e["address"]),
    }, indent=1), encoding="utf-8")

    print(f"\napplied: {len(promotions)} promoted in state.db + ledger; "
          f"V1 queue written ({len(queue)}).")
    return 0


if __name__ == "__main__":
    sys.exit(main())
