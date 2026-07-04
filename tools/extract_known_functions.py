#!/usr/bin/env python3
"""Extract address-annotated Diablo II symbols from the D2MOO source tree.

D2MOO often tags functions/objects with comments like:
  //1.10f: Game.0x401040
  // 1.10f: 0x6FF5A860 (#10176)
  //1.13c: 0x6FF69CB0

This script scans `source/` for those annotations and attempts to resolve the
associated function name by looking at the following signature.

Outputs:
  - artifacts/d2moo_known_functions.csv
  - artifacts/d2moo_known_functions.json

Notes:
  - These are the functions/symbols D2MOO has *annotated*, not an exhaustive
    list of every function in Diablo II binaries.
  - Addresses are specific to the tagged version (1.10f, 1.13c, etc.).
"""

from __future__ import annotations

import csv
import json
import os
import re
import sys
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Iterable, List, Optional, Tuple


ADDRESS_TAG_RE = re.compile(
    r"^\s*//\s*(?P<ver>1\.\d+[a-z]?)\s*:\s*(?:(?P<module>[A-Za-z0-9_]+)\.)?0x(?P<addr>[0-9A-Fa-f]+)(?P<rest>.*)$"
)

CONTROL_KEYWORDS = {
    "if",
    "for",
    "while",
    "switch",
    "return",
    "sizeof",
    "catch",
}


@dataclass(frozen=True)
class KnownSymbol:
    version: str
    module_hint: str
    address_hex: str
    address_int: int
    name: str
    kind: str
    rel_file: str
    line: int
    signature: str
    tag_rest: str


def iter_source_files(root: Path) -> Iterable[Path]:
    src = root / "source"
    for path in src.rglob("*"):
        if not path.is_file():
            continue
        # Skip generated/irrelevant content if it exists inside source/
        if any(part.lower().startswith("build") for part in path.parts):
            continue
        if path.suffix.lower() not in {".c", ".cc", ".cpp", ".cxx", ".h", ".hpp", ".inl"}:
            continue
        yield path


def _is_trivial_line(line: str) -> bool:
    s = line.strip()
    return (not s) or s.startswith("//") or s.startswith("/*") or s.startswith("*")


def _looks_like_function_signature(text: str) -> bool:
    s = text.strip()
    if not s:
        return False
    if s.startswith("#"):
        return False
    # Filter typedefs and obvious non-signatures
    if s.startswith("typedef "):
        return False
    if "(" not in s:
        return False
    # Avoid function pointer typedef/declarations like: (*fn)(...)
    if "(*" in s:
        return False
    # Avoid macro invocations that look like calls
    head = s.split("(", 1)[0].strip().split()
    if head and head[-1] in CONTROL_KEYWORDS:
        return False
    return True


def _extract_name_from_signature(signature: str) -> Optional[str]:
    # Take the last token before the first '(' across a potentially multi-line signature.
    before_paren = signature.split("(", 1)[0]
    # Remove common qualifiers that can appear at the end (rare, but safe)
    before_paren = before_paren.strip()

    # Pick the last C/C++ identifier-ish token (allow namespaces and destructors).
    matches = re.findall(r"(~?[A-Za-z_][A-Za-z0-9_]*?(?:::[A-Za-z_][A-Za-z0-9_]*)*)\s*$", before_paren)
    if not matches:
        return None
    name = matches[-1]
    if name in CONTROL_KEYWORDS:
        return None
    return name


def _collect_signature(lines: List[str], start_index: int, max_lines: int = 8) -> Tuple[str, int]:
    """Collect a signature-like chunk starting at start_index.

    Returns (signature_text, lines_consumed).
    """
    parts: List[str] = []
    consumed = 0
    for i in range(start_index, min(len(lines), start_index + max_lines)):
        line = lines[i].rstrip("\n")
        if line.strip().startswith("//"):
            # stop if we hit another tag/comment block before a signature
            if parts:
                break
            continue
        if line.strip().startswith("#") and not parts:
            continue

        parts.append(line.strip())
        consumed += 1

        joined = " ".join(parts)
        if "{" in joined or ";" in joined:
            break

    signature = " ".join(p for p in parts if p)
    signature = re.sub(r"\s+", " ", signature).strip()
    return signature, consumed


def extract_known_symbols(root: Path) -> List[KnownSymbol]:
    results: List[KnownSymbol] = []

    for path in iter_source_files(root):
        try:
            text = path.read_text(encoding="utf-8", errors="replace")
        except Exception:
            continue

        lines = text.splitlines(True)
        rel = path.relative_to(root).as_posix()

        i = 0
        while i < len(lines):
            m = ADDRESS_TAG_RE.match(lines[i])
            if not m:
                i += 1
                continue

            version = m.group("ver")
            module_hint = (m.group("module") or "").strip()
            addr_hex_raw = m.group("addr")
            addr_hex = "0x" + addr_hex_raw.upper()
            addr_int = int(addr_hex_raw, 16)
            tag_rest = (m.group("rest") or "").strip()

            # Look ahead to find the associated symbol signature.
            lookahead_start = i + 1
            while lookahead_start < len(lines) and _is_trivial_line(lines[lookahead_start]):
                lookahead_start += 1

            signature, consumed = _collect_signature(lines, lookahead_start)
            name = None
            kind = "unknown"

            if _looks_like_function_signature(signature):
                name = _extract_name_from_signature(signature)
                if name:
                    kind = "function"

            if not name:
                # fallback: keep a deterministic placeholder so it can be searched
                name = "<unresolved>"

            results.append(
                KnownSymbol(
                    version=version,
                    module_hint=module_hint,
                    address_hex=addr_hex,
                    address_int=addr_int,
                    name=name,
                    kind=kind,
                    rel_file=rel,
                    line=i + 1,
                    signature=signature,
                    tag_rest=tag_rest,
                )
            )

            i += 1

    # Sort deterministically.
    results.sort(key=lambda r: (r.version, r.module_hint, r.address_int, r.name, r.rel_file, r.line))
    return results


def main(argv: List[str]) -> int:
    root = Path(__file__).resolve().parents[1]
    out_dir = root / "artifacts"
    out_dir.mkdir(parents=True, exist_ok=True)

    results = extract_known_symbols(root)

    # Filter to functions for the main outputs.
    functions = [r for r in results if r.kind == "function"]

    csv_path = out_dir / "d2moo_known_functions.csv"
    json_path = out_dir / "d2moo_known_functions.json"

    fieldnames = [
        "version",
        "module_hint",
        "address_hex",
        "address_int",
        "name",
        "rel_file",
        "line",
        "signature",
        "tag_rest",
    ]

    with csv_path.open("w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=fieldnames)
        w.writeheader()
        for r in functions:
            row = {k: getattr(r, k) for k in fieldnames}
            w.writerow(row)

    with json_path.open("w", encoding="utf-8") as f:
        json.dump([asdict(r) for r in functions], f, indent=2)

    # Also write a quick summary file including unresolved tags.
    summary_path = out_dir / "d2moo_known_functions_summary.json"
    summary = {
        "total_tags_found": len(results),
        "functions_extracted": len(functions),
        "unresolved_tags": [asdict(r) for r in results if r.kind != "function"],
        "versions": sorted({r.version for r in results}),
    }
    with summary_path.open("w", encoding="utf-8") as f:
        json.dump(summary, f, indent=2)

    print(f"Tags found: {len(results)}")
    print(f"Functions extracted: {len(functions)}")
    print(f"Wrote: {csv_path}")
    print(f"Wrote: {json_path}")
    print(f"Wrote: {summary_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
