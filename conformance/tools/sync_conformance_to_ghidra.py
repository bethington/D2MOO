#!/usr/bin/env python3
"""sync_conformance_to_ghidra.py -- make Ghidra the single source of truth for the
conformance FACTS.

The DOC_/CONF_ RUNG lives in Ghidra function tags already (the port pipeline writes
them on every proof). The one semantic fact that lived ONLY in proven_functions.jsonl
was the PROOF DETAIL -- method, vector counts, shadow hits/divergence, vetted status,
date, and which reimpl candidate proves it. This tool moves that into Ghidra too, as a
`CONFORMANCE`-category BOOKMARK on the function's entry address whose comment is a
compact JSON record. Bookmarks are separate from the plate comment, so the human docs
stay clean; they're queryable (list_bookmarks category=CONFORMANCE) and diff-free.

After a push, Ghidra holds BOTH the rung (tag) and the proof detail (bookmark) -- it is
the authoritative record. proven_functions.jsonl becomes a generated MIRROR: `--export`
reads the bookmarks straight back out of Ghidra, proving nothing was lost.

    python sync_conformance_to_ghidra.py            # registry -> Ghidra (push), save
    python sync_conformance_to_ghidra.py --dry-run  # show what would be written
    python sync_conformance_to_ghidra.py --export    # Ghidra -> stdout (round-trip proof)
    python sync_conformance_to_ghidra.py --export --write-mirror  # + regenerate a mirror jsonl

Env: GHIDRA_SERVER_URL (default http://127.0.0.1:8089), FUNDOC_GHIDRA_PROGRAM
(default /Mods/PD2-S12/D2Common.dll). Ghidra must be open with the ghidra-mcp bridge up.
"""
from __future__ import annotations
import argparse
import json
import os
import urllib.request
from urllib.parse import urlencode
from pathlib import Path

GHIDRA = os.environ.get("GHIDRA_SERVER_URL", "http://127.0.0.1:8089").rstrip("/")
PROGRAM = os.environ.get("FUNDOC_GHIDRA_PROGRAM", "/Mods/PD2-S12/D2Common.dll")
REG = Path(__file__).resolve().parent.parent / "proven_functions.jsonl"


def _get(path: str, **params) -> dict:
    url = f"{GHIDRA}{path}" + ("?" + urlencode(params) if params else "")
    with urllib.request.urlopen(url, timeout=60) as r:
        return json.loads(r.read().decode("utf-8", "replace"))


def _post(path: str, data: dict) -> dict:
    req = urllib.request.Request(
        f"{GHIDRA}{path}", data=json.dumps(data).encode(),
        headers={"Content-Type": "application/json"}, method="POST")
    with urllib.request.urlopen(req, timeout=60) as r:
        return json.loads(r.read().decode("utf-8", "replace"))


def _load_registry() -> list[dict]:
    rows = []
    for line in REG.read_text(encoding="utf-8").splitlines():
        s = line.strip()
        if not s:
            continue
        try:
            rows.append(json.loads(s))
        except json.JSONDecodeError:
            pass
    return rows


def _conf_record(d: dict) -> dict:
    """The compact proof record stored in the CONFORMANCE bookmark comment.
    Only the SEMANTIC proof facts -- never queue/token/telemetry state."""
    rec: dict = {"conf": d.get("conf")}
    method = d.get("proof_kind") or d.get("method")
    if method:
        rec["method"] = method
    for src, dst in (("vectors", "vectors"), ("passed", "passed"), ("total", "total"),
                     ("shadow_hits", "shadow_hits"), ("shadow_divergences", "shadow_div"),
                     ("vetted", "vetted"), ("ret", "ret"), ("callconv", "callconv"),
                     ("orig_regs", "orig_regs"), ("date", "date")):
        v = d.get(src)
        if v not in (None, "", 0):
            rec[dst] = v
    rec["reimpl"] = f"candidates/{d.get('name')}.cpp"
    if d.get("needs_review"):
        rec["needs_review"] = True
    return rec


def push(dry: bool = False, tags: bool = True) -> int:
    rows = _load_registry()
    bm_ok = tag_ok = 0
    fails = []
    for d in rows:
        addr, name, conf = d.get("address"), d.get("name"), d.get("conf")
        if not addr:
            continue
        comment = json.dumps(_conf_record(d), separators=(",", ":"))
        if tags and conf:
            try:
                _post("/add_function_tag",
                      {"function": addr, "tags": conf, "program": PROGRAM, "dry_run": dry})
                tag_ok += 1
            except OSError:
                pass
        try:
            r = _post("/set_bookmark", {"address": addr, "category": "CONFORMANCE",
                                        "comment": comment, "program": PROGRAM, "dry_run": dry})
            if r.get("success"):
                bm_ok += 1
            else:
                fails.append((name, r))
        except OSError as e:
            fails.append((name, str(e)))
    if not dry:
        try:
            _post("/save_program", {"program": PROGRAM})
        except OSError:
            print("  [warn] save_program failed -- writes are in memory, save Ghidra manually")
    tag = " (DRY RUN -- nothing written)" if dry else ""
    print(f"pushed {bm_ok}/{len(rows)} CONFORMANCE bookmarks"
          f"{f', topped up {tag_ok} rung tags' if tags else ''}{tag}")
    for name, err in fails[:12]:
        print(f"  FAIL {name}: {str(err)[:110]}")
    return 0 if not fails else 1


def export(write_mirror: bool = False) -> int:
    r = _get("/list_bookmarks", category="CONFORMANCE", program=PROGRAM)
    recs = []
    for b in r.get("bookmarks", []):
        try:
            rec = json.loads(b["comment"])
            rec["address"] = "0x" + b["address"]
            recs.append(rec)
        except (KeyError, json.JSONDecodeError):
            pass
    print(f"read {len(recs)} CONFORMANCE records back from Ghidra (round-trip proof)")
    for rec in recs[:3]:
        print("  " + json.dumps(rec, separators=(",", ":")))
    if write_mirror:
        out = REG.parent / "proven_functions.from_ghidra.jsonl"
        out.write_text("\n".join(json.dumps(r) for r in recs) + "\n", encoding="utf-8")
        print(f"wrote Ghidra-sourced mirror -> {out}")
    return 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--dry-run", action="store_true", help="show writes, change nothing")
    ap.add_argument("--export", action="store_true", help="read Ghidra bookmarks back out")
    ap.add_argument("--write-mirror", action="store_true",
                    help="with --export: regenerate a jsonl from Ghidra")
    ap.add_argument("--no-tags", action="store_true", help="only write bookmarks, skip rung tags")
    args = ap.parse_args()
    if args.export:
        return export(write_mirror=args.write_mirror)
    return push(dry=args.dry_run, tags=not args.no_tags)


if __name__ == "__main__":
    raise SystemExit(main())
