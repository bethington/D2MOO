#!/usr/bin/env python3
"""Apply conformance/naming_simplification_map.json to the source tree.

Word-boundary, byte-safe (latin-1 roundtrip; all names are ASCII) rename of
every map entry where old != new. Scope: source/ and D2.Detours.patches/,
excluding external/ submodules and build output.

Usage: apply_naming_simplification.py [--dry-run]
"""
import json
import re
import sys
from collections import Counter
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
SCOPES = [REPO / "source", REPO / "D2.Detours.patches", REPO / "conformance"]
EXTS = {".h", ".hpp", ".c", ".cc", ".cpp", ".inl"}
EXCLUDE_PARTS = {"external", ".git", "out"}


def main():
    dry = "--dry-run" in sys.argv
    m = json.loads((REPO / "conformance" / "naming_simplification_map.json").read_text())
    repl = {e["old"]: e["new"] for e in m["entries"] if e["old"] != e["new"]}
    pat = re.compile(
        r"\b(" + "|".join(sorted(map(re.escape, repl), key=len, reverse=True)) + r")\b"
    )

    per_name = Counter()
    files_changed = 0
    total = 0
    for scope in SCOPES:
        if not scope.exists():
            continue
        for f in sorted(scope.rglob("*")):
            if f.suffix.lower() not in EXTS or not f.is_file():
                continue
            if EXCLUDE_PARTS & set(p.lower() for p in f.parts):
                continue
            raw = f.read_bytes()
            if raw[:2] in (b"\xff\xfe", b"\xfe\xff"):
                print(f"SKIP utf-16: {f}")
                continue
            text = raw.decode("latin-1")
            hits = pat.findall(text)
            if not hits:
                continue
            new_text = pat.sub(lambda mo: repl[mo.group(1)], text)
            per_name.update(hits)
            files_changed += 1
            total += len(hits)
            if not dry:
                f.write_bytes(new_text.encode("latin-1"))

    print(f"{'DRY RUN: ' if dry else ''}{total} replacements in {files_changed} files")
    print("top 20:")
    for name, n in per_name.most_common(20):
        print(f"  {name} -> {repl[name]}: {n}")
    unused = set(repl) - set(per_name)
    if unused:
        print(f"map entries with zero occurrences: {len(unused)}")
        for u in sorted(unused)[:15]:
            print("  ", u)


if __name__ == "__main__":
    main()
