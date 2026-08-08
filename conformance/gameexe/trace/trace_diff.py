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


# Returns that are volatile by definition. Comparing them is meaningless and
# — worse — symbolising one run's value but not the other's desynchronises
# every symbol that follows. Found by the self-control: GetCurrentThreadId
# returned 0xedb8, below the pointer threshold, so it was compared literally
# and shifted all later ids by one.
VOLATILE_RETURNS = {
    "GetCurrentThreadId", "GetCurrentProcessId", "GetTickCount",
    "GetTickCount64", "timeGetTime", "QueryPerformanceCounter",
    "GetSystemTimeAsFileTime", "GetLastError",
}


def _as_int(raw):
    """Parse a trace value as an int, or None if it is not numeric."""
    if not isinstance(raw, str):
        return None
    try:
        return int(raw, 16) if raw.startswith("0x") else int(raw)
    except (ValueError, TypeError):
        return None


class Normaliser:
    """Canonicalise volatile values, counting every substitution.

    Two-pass, deliberately. Symbolic ids are assigned ONLY to values that
    the run actually REUSES; a value seen once becomes an anonymous <ptr>.

    Sequential first-seen numbering over every large value looked right and
    was fragile: a single extra allocation in one run shifted every later id,
    so the self-control (the SAME binary traced twice) reported 181
    divergences. Numbering only reused values removes that coupling while
    keeping the property worth having -- that a handle returned by one call
    and passed to a later one is visibly the same handle, which is what makes
    "closed the key it opened" checkable.
    """

    def __init__(self, events=None):
        self.fired = Counter()
        self._handles = {}
        self._reused = set()
        self._flowed = set()
        if events:
            seen, twice = set(), set()
            returned, passed = set(), set()
            for e in events:
                for raw in self._raw_values(e):
                    if raw in seen:
                        twice.add(raw)
                    seen.add(raw)
                if e.get("type") == "call":
                    if isinstance(e.get("ret"), str):
                        returned.add(e["ret"])
                    for a in e.get("args", []) or []:
                        if isinstance(a, dict) and isinstance(a.get("raw"), str):
                            passed.add(a["raw"])
            self._reused = twice
            # A value RETURNED by one call and later PASSED to another is an
            # identity, not data -- whatever its magnitude. OS handles are
            # routinely small: GetStdHandle returned 0xb0 in one run and 0xb4
            # in another, both far below the pointer threshold, so they were
            # compared literally and produced 12 spurious divergences between
            # two runs of the SAME binary. Size is not the discriminator;
            # flow is. The `> 0x20` floor keeps genuine small constants
            # (FILE_TYPE_CHAR = 2, ACL_REVISION = 2) out of it.
            self._flowed = {v for v in (returned & passed)
                            if _as_int(v) is not None and _as_int(v) > 0x20}

    @staticmethod
    def _raw_values(e):
        if e.get("type") != "call":
            return
        for a in e.get("args", []) or []:
            if isinstance(a, dict) and "str" not in a and "konst" not in a:
                raw = a.get("raw")
                if isinstance(raw, str):
                    yield raw
        ret = e.get("ret")
        if isinstance(ret, str):
            yield ret

    def _symbol(self, raw: str) -> str:
        if raw not in self._handles:
            self._handles[raw] = f"<h{len(self._handles)}>"
        return self._handles[raw]

    def value(self, raw: str, volatile: bool = False) -> str:
        if not isinstance(raw, str):
            return raw
        if volatile:
            self.fired["volatile return -> dropped"] += 1
            return "<volatile>"
        v = _as_int(raw)
        if v is None:
            return raw
        # A handle that flows between calls is an identity at any size.
        if raw in self._flowed:
            self.fired["flowing handle -> symbol"] += 1
            return self._symbol(raw)
        # A value reused across calls is an identity even when small: Win32
        # hands most handles back through an OUT PARAMETER, never as a return
        # value, so the flow rule above cannot see them. RegOpenKeyExA writes
        # its HKEY into *phkResult, and that handle (0x40c in one run, 0x3ec
        # in another) then appears only ever as an ARGUMENT.
        if raw in self._reused and v > 0xff:
            self.fired["reused handle -> symbol"] += 1
            return self._symbol(raw)
        # Small integers are real data (flags, counts, indices): keep them.
        if -0x10000 <= v <= 0x10000:
            return raw
        if raw in self._reused:
            self.fired["reused pointer -> symbol"] += 1
            return self._symbol(raw)
        self.fired["one-off pointer -> anonymous"] += 1
        return "<ptr>"

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
        fn = e.get("fn") or ""
        bare = fn.split("!")[-1]
        return {
            "fn": fn,
            "args": [self.arg(a) for a in e.get("args", [])],
            "ret": self.value(e.get("ret"), volatile=bare in VOLATILE_RETURNS),
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

    na, nb = Normaliser(ev_a), Normaliser(ev_b)
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
