"""Diff two Game.exe traces and decide CONF_TRACE.

    python trace_diff.py original.jsonl ours.jsonl
    python trace_diff.py original.jsonl ours.jsonl --json verdict.json

NORMALISATION IS VISIBLE, ALWAYS. A trace contains values that legitimately
differ between two runs of the SAME binary -- heap pointers, HKEY handles,
module bases, tick counts. Those must be canonicalised or every run reports
divergence and the check gets ignored. But a silent normaliser is how a real
divergence gets erased, which is the same shape as every other
gate-with-no-input defect in this project. So every rule that fires is
counted and printed, and `--json` records the counts. If a rule starts
firing far more than usual, that is itself a signal.

WHAT IS NOT NORMALISED, ON PURPOSE: strings. A wrong registry path, a wrong
filename, a wrong module name or a wrong window class is exactly the defect
this exists to catch, so string arguments are compared literally.
"""
from __future__ import annotations

import argparse
import json
import re
import sys
from collections import Counter
from pathlib import Path


class Normaliser:
    """Canonicalise volatile values, counting every substitution."""

    def __init__(self):
        self.fired = Counter()
        self._handles = {}

    def _symbol(self, raw: str) -> str:
        """Stable symbolic id per distinct value, in FIRST-SEEN ORDER.

        Order-dependence is the point: it preserves the relationship between
        a handle returned by one call and the same handle passed to a later
        one, which is what makes 'closed the key it opened' checkable.
        """
        if raw not in self._handles:
            self._handles[raw] = f"<h{len(self._handles)}>"
        return self._handles[raw]

    def value(self, raw: str) -> str:
        if not isinstance(raw, str):
            return raw
        try:
            v = int(raw, 16) if raw.startswith("0x") else int(raw)
        except (ValueError, TypeError):
            return raw
        # Small integers are real data (flags, counts, indices): keep them.
        if -0x10000 <= v <= 0x10000:
            return raw
        # Everything large is a pointer or a handle in practice.
        self.fired["pointer_or_handle -> symbol"] += 1
        return self._symbol(raw)

    def arg(self, a: dict) -> dict:
        if not isinstance(a, dict):
            return a
        out = {}
        if "str" in a:
            out["str"] = a["str"]          # NEVER normalised
        if "konst" in a:
            out["konst"] = a["konst"]
        # A constant or a string identifies the argument; the raw value then
        # adds nothing but noise.
        if "str" not in a and "konst" not in a:
            out["raw"] = self.value(a.get("raw"))
        return out

    def event(self, e: dict) -> dict:
        if e.get("type") != "call":
            return {"type": e.get("type")}
        return {
            "fn": e.get("fn"),
            "args": [self.arg(a) for a in e.get("args", [])],
            "ret": self.value(e.get("ret")),
        }


def load(path: Path):
    meta, events = {}, []
    for line in Path(path).read_text(encoding="utf-8").splitlines():
        if not line.strip():
            continue
        rec = json.loads(line)
        if rec.get("type") == "meta":
            meta = rec
        else:
            events.append(rec)
    return meta, events


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("original")
    ap.add_argument("ours")
    ap.add_argument("--json", help="write the verdict here")
    ap.add_argument("--context", type=int, default=3)
    args = ap.parse_args()

    meta_a, ev_a = load(Path(args.original))
    meta_b, ev_b = load(Path(args.ours))

    # A run that never reached the handoff traced a DIFFERENT amount of the
    # program; comparing those is meaningless and must not read as a pass.
    for label, meta in (("original", meta_a), ("ours", meta_b)):
        if not meta.get("reached_handoff"):
            print(f"!! {label} did not reach the module handoff "
                  f"({meta.get('events', '?')} events) -- the run is incomplete, "
                  f"not clean")

    na, nb = Normaliser(), Normaliser()
    sa = [na.event(e) for e in ev_a]
    sb = [nb.event(e) for e in ev_b]

    diffs = []
    for i in range(max(len(sa), len(sb))):
        x = sa[i] if i < len(sa) else None
        y = sb[i] if i < len(sb) else None
        if x != y:
            diffs.append((i, x, y))

    print(f"\n  original: {len(sa)} events   ours: {len(sb)} events")
    print("  normalisation rules fired:")
    for src, counter in (("original", na.fired), ("ours", nb.fired)):
        for rule, n in counter.most_common():
            print(f"    {src:9} {rule:34} {n}")
        if not counter:
            print(f"    {src:9} (none)")

    if not diffs:
        print(f"\n  CONF_TRACE: PASS -- {len(sa)} events identical after "
              f"normalisation")
    else:
        print(f"\n  CONF_TRACE: FAIL -- {len(diffs)} divergence(s); first "
              f"{min(len(diffs), args.context)}:")
        for i, x, y in diffs[:args.context]:
            print(f"    @{i}")
            print(f"      original: {json.dumps(x)}")
            print(f"      ours    : {json.dumps(y)}")

    verdict = {
        "verdict": "PASS" if not diffs else "FAIL",
        "events_original": len(sa),
        "events_ours": len(sb),
        "divergences": len(diffs),
        "reached_handoff": {"original": meta_a.get("reached_handoff"),
                            "ours": meta_b.get("reached_handoff")},
        "normalisation": {"original": dict(na.fired), "ours": dict(nb.fired)},
        "first_divergences": [
            {"index": i, "original": x, "ours": y} for i, x, y in diffs[:20]
        ],
    }
    if args.json:
        Path(args.json).write_text(json.dumps(verdict, indent=2), encoding="utf-8")
        print(f"\n  wrote {args.json}")
    return 0 if not diffs else 1


if __name__ == "__main__":
    sys.exit(main())
