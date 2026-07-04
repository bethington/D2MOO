"""behav_match — the matcher half of the Tier-2 behavioral ordinal resolver.

Given a QUERY fingerprint set (D2MOO functions -> [{in,out}]) and a CANDIDATE
fingerprint set (PD2-S12 export ordinals -> [{in,out}], captured via the live-game
call_function oracle using the SAME ordered inputs), find, for each query function,
the candidate(s) whose outputs are BIT-EXACT across every input. A unique bit-exact
candidate ⇒ PROVEN ordinal match (EMULATION_CONFORMANCE_PLAN §17, Tier 2).

This is what structural Tier-1 could NOT do: behavior distinguishes functions that
are structurally identical (e.g. two coord transforms differing only in a constant).

Usage:
  python behav_match.py <query.json> <candidates.json>
  # candidates.json may be another fingerprint file; keys are candidate labels
  # (PD2 ordinals in real use, function names for the self-match validation).
"""
import sys, json


def outputs(entries):
    # canonical, order-sensitive tuple of outputs (inputs assumed identical+ordered)
    return tuple(tuple(e["out"]) for e in entries)


def main():
    query = json.load(open(sys.argv[1], encoding="utf-8"))
    cands = json.load(open(sys.argv[2], encoding="utf-8"))
    cand_out = {name: outputs(v) for name, v in cands.items()}

    resolved = ambiguous = unmatched = 0
    for qname, qentries in query.items():
        qo = outputs(qentries)
        hits = [c for c, co in cand_out.items() if co == qo]
        if len(hits) == 1:
            resolved += 1
            print(f"  PROVEN     {qname:36} -> {hits[0]}")
        elif len(hits) > 1:
            ambiguous += 1
            print(f"  AMBIGUOUS  {qname:36} -> {hits}  (need more/diverse inputs)")
        else:
            unmatched += 1
            print(f"  UNMATCHED  {qname:36} -> (no bit-exact candidate)")
    total = len(query)
    print(f"\n  {resolved}/{total} PROVEN (unique), {ambiguous} ambiguous, {unmatched} unmatched")
    return 0 if unmatched == 0 and ambiguous == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
