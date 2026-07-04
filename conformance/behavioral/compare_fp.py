"""compare_fp — direct conformance diff of two behavioral fingerprints.

Both files are {fn_name: [{in:[...], out:[...]}, ...]} with the SAME functions
and SAME ordered inputs (e.g. d2moo_fp.json vs pd2_fp.json). For each function
it checks every input's output is bit-identical. Exit 0 iff every function
matches on every input -> PROVEN conformance.

Usage:  python compare_fp.py <a.json> <b.json>   # default: d2moo_fp.json pd2_fp.json
"""
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))


def load(p):
    if not os.path.isabs(p):
        p = os.path.join(HERE, p)
    return json.load(open(p, encoding="utf-8"))


def main():
    a_path = sys.argv[1] if len(sys.argv) > 1 else "d2moo_fp.json"
    b_path = sys.argv[2] if len(sys.argv) > 2 else "pd2_fp.json"
    a, b = load(a_path), load(b_path)
    a_name = os.path.basename(a_path)
    b_name = os.path.basename(b_path)

    ok = True
    for fn in a:
        if fn not in b:
            print(f"  MISSING in {b_name}: {fn}"); ok = False; continue
        av = [(e["in"], e["out"]) for e in a[fn]]
        bv = [(e["in"], e["out"]) for e in b[fn]]
        if av == bv:
            print(f"  PROVEN   {fn:36} {len(av)}/{len(av)} inputs bit-exact")
        else:
            ok = False
            print(f"  MISMATCH {fn}")
            for (ai, ao), (bi, bo) in zip(av, bv):
                if ao != bo:
                    print(f"      in={ai}  {a_name}={ao}  {b_name}={bo}")
    print()
    print(f"  ==> ALL BIT-EXACT: {a_name} == {b_name}" if ok
          else "  ==> DIVERGENCE FOUND")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
