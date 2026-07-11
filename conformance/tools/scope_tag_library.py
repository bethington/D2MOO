#!/usr/bin/env python3
"""scope_tag_library.py -- the SCOPE gate.

Library / compiler-runtime functions (CRT, exception handling, stack-cookie, FP
helpers) are not game logic and there's no useful reason to document or prove them.
This tags each one `LIB_<which>` in Ghidra so the pipeline skips it and the dashboard
denominators count only in-scope GAME functions. Same source-of-truth mechanism as the
rungs (function tags) -- composes directly with sync_conformance_to_ghidra.py.

Design guardrails (false exclusion is the dangerous failure -- a wrongly-tagged game
function silently never gets processed):
  * CONSERVATIVE, name-based, high-confidence patterns only.
  * NEVER tags a function that already carries a CONF_ or DOC_ rung -- that's game logic
    we've deliberately worked (e.g. the seed RNG can look CRT-ish but IS game-critical).
  * Records the disposition in the `Scope` property map (value = the LIB_ category = the
    reason), queryable as a set via list_properties; the LIB_ tag is reversible.

  python scope_tag_library.py            # DRY RUN -- classify + report, write nothing
  python scope_tag_library.py --apply     # write LIB_ tags + SCOPE bookmarks, save
  python scope_tag_library.py --export     # list what is already LIB_-tagged

Env: GHIDRA_SERVER_URL (default http://127.0.0.1:8089), FUNDOC_GHIDRA_PROGRAM.
"""
from __future__ import annotations
import argparse
import json
import os
import re
import urllib.request
from urllib.parse import urlencode

GHIDRA = os.environ.get("GHIDRA_SERVER_URL", "http://127.0.0.1:8089").rstrip("/")
PROGRAM = os.environ.get("FUNDOC_GHIDRA_PROGRAM", "/Mods/PD2-S12/D2Common.dll")

# High-confidence library/runtime name signatures. Ordered; first match wins.
CLASSIFIERS = [
    ("LIB_CRT", re.compile(
        r'^_?(mem(cpy|set|move|cmp|chr)|str(len|cpy|cmp|cat|nchr|str|ncmp|tok)'
        r'|malloc|free|calloc|realloc|qsort|bsearch|printf|sprintf|sscanf|atoi|atol'
        r'|_ftol|_alloca)$', re.I)),
    ("LIB_MSVC_EH", re.compile(
        r'(CxxFrameHandler|_EH[0-9]?_prolog|except_handler|__unDName|_RTC_|CxxThrow'
        r'|type_info|__crt|_purecall|_terminate|unexpected)', re.I)),
    ("LIB_SECURITY", re.compile(
        r'(security_(check|init)_cookie|report_gsfailure|report_rangecheck|_chkstk)', re.I)),
    ("LIB_MATH", re.compile(
        r'^_?(dtol|ltod|ftol|__CI(pow|exp|log|sqrt|sin|cos|tan)|_fltused|adjust_fdiv)', re.I)),
    ("LIB_MSVC", re.compile(
        r'^(__imp_|DllMainCRTStartup|_initterm|onexit|_amsg_exit|_XcptFilter|doexit'
        r'|_cinit|_controlfp|_ismbblead)', re.I)),
    # MSVC/CRT compiler-runtime convention: any name that begins with two or more
    # underscores. This is the reserved-identifier convention the compiler/CRT uses
    # for its internal helpers -- float & long-double math, FP control word,
    # number<->string formatting, 64/96-bit integer routines, x87 instruction thunks
    # (__cfltcvt, __alldiv, __control87, __fptostr, __RoundMan, ___dtold,
    # __adj_fdiv_m32i, __ms_p5_test_fdiv, __fdivp_sti_st, ...). Game code never uses a
    # leading double underscore, and the PROTECT_TAGS guard still refuses to exclude
    # anything that already carries a CONF_/DOC_ rung.
    ("LIB_CRT", re.compile(r'^__')),
]
# rungs that mean "this is game logic we care about" -- NEVER auto-exclude these.
PROTECT_TAGS = ("CONF_DRAFT", "CONF_VECTORS", "CONF_LIVE", "CONF_BATTLETESTED",
                "CONF_REGRESSION", "DOC_DRAFT", "DOC_REVIEWED", "DOC_VERIFIED")
LIB_TAGS = [c[0] for c in CLASSIFIERS] + ["LIB_UNKNOWN"]


def _get(path: str, **params) -> str:
    url = f"{GHIDRA}{path}" + ("?" + urlencode(params) if params else "")
    with urllib.request.urlopen(url, timeout=90) as r:
        return r.read().decode("utf-8", "replace")


def _post(path: str, data: dict) -> dict:
    req = urllib.request.Request(
        f"{GHIDRA}{path}", data=json.dumps(data).encode(),
        headers={"Content-Type": "application/json"}, method="POST")
    with urllib.request.urlopen(req, timeout=60) as r:
        return json.loads(r.read().decode("utf-8", "replace"))


_LINE = re.compile(r"^(?P<name>\S.*?)\s+at\s+(?P<addr>[0-9a-fA-F]+)\s*$")


def _all_functions() -> list[tuple[str, str]]:
    """[(name, 0xaddr), ...] from /list_functions ('NAME at ADDR' text). NOTE: this
    endpoint ignores offset and returns the full list every call, so we dedupe by
    address and stop once a page adds nothing new (works whether or not it paginates)."""
    seen: dict[str, str] = {}   # addr -> name
    off = 0
    while off < 40000:
        txt = _get("/list_functions", program=PROGRAM, offset=off, limit=2000)
        lines = [ln for ln in txt.splitlines() if ln.strip()]
        if not lines:
            break
        before = len(seen)
        for ln in lines:
            m = _LINE.match(ln.strip())
            if m:
                seen.setdefault("0x" + m.group("addr").lower(), m.group("name"))
        if len(seen) == before or len(lines) < 2000:
            break
        off += 2000
    return [(name, addr) for addr, name in seen.items()]


def _classify(name: str) -> str | None:
    for tag, rx in CLASSIFIERS:
        if rx.search(name):
            return tag
    return None


def _tags_of(addr: str) -> set[str]:
    try:
        r = json.loads(_get("/get_function_tags", function=addr, program=PROGRAM))
        return {t.get("name") for t in r.get("tags", [])}
    except (OSError, json.JSONDecodeError):
        return set()


# ---- content fingerprints: detect library functions by normalized opcode hash ----
# Name-based classification breaks once a library function has been renamed. But the same
# statically-linked CRT function is byte-for-byte identical across binaries, and Ghidra's
# normalized opcode hash is stable across relocation (proven: __alldiv has an identical hash
# in D2Common and D2Client). So we fingerprint every function we CAN still name-classify as
# LIB_ and match its renamed copies anywhere by EXACT hash -- zero false positives.
FINGERPRINT_DB = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                              "lib_fingerprints.json")
FP_MIN_INSTRS = 8   # don't fingerprint tiny stubs -- short opcode hashes can collide


def _bulk_hashes(program: str | None = None) -> dict:
    """{'0x<addr>': {'hash','name','instrs'}} for every function via /get_bulk_function_hashes."""
    program = program or PROGRAM
    out: dict[str, dict] = {}
    off = 0
    while True:
        try:
            r = json.loads(_get("/get_bulk_function_hashes", filter="all",
                                program=program, limit=1000, offset=off))
        except (OSError, json.JSONDecodeError):
            break
        fns = r.get("functions") or []
        for f in fns:
            a = "0x" + str(f.get("address", "")).lower().lstrip("0x")
            out[a] = {"hash": f.get("hash"), "name": f.get("name"),
                      "instrs": f.get("instruction_count", 0)}
        got = r.get("returned", len(fns))
        total = r.get("total_matching", len(out))
        off += got
        if got == 0 or off >= total:
            break
    return out


def load_fingerprints() -> dict:
    """{hash: {'category','name'}} of known library functions (the git-tracked signature DB)."""
    try:
        with open(FINGERPRINT_DB, encoding="utf-8") as fh:
            return json.load(fh).get("fingerprints", {})
    except (OSError, json.JSONDecodeError):
        return {}


def save_fingerprints(fp: dict) -> None:
    with open(FINGERPRINT_DB, "w", encoding="utf-8") as fh:
        json.dump({"source": "scope_tag_library.py",
                   "note": "normalized-opcode hash -> known library function",
                   "count": len(fp), "fingerprints": fp}, fh, indent=1, sort_keys=True)
        fh.write("\n")


def build_fingerprints(programs: list[str]) -> dict:
    """Record the normalized-opcode hash of every NAME-classified LIB_ function across the
    given programs into the fingerprint DB. Build from binaries where the CRT still has its
    default __ names; the hashes then match renamed copies elsewhere. Returns the merged DB."""
    global PROGRAM
    fp = load_fingerprints()
    before = len(fp)
    saved = PROGRAM
    for prog in programs:
        PROGRAM = prog
        hashes = _bulk_hashes(prog)
        hit = 0
        for _addr, info in hashes.items():
            tag = _classify(info.get("name") or "")
            h = info.get("hash")
            if tag and h and info.get("instrs", 0) >= FP_MIN_INSTRS:
                fp.setdefault(h, {"category": tag, "name": info.get("name")})
                hit += 1
        print(f"  {prog}: {len(hashes)} functions, {hit} name-classified LIB_ -> DB {len(fp)}",
              flush=True)
    PROGRAM = saved
    save_fingerprints(fp)
    print(f"fingerprint DB: {len(fp)} known library functions (+{len(fp) - before}) "
          f"-> {FINGERPRINT_DB}", flush=True)
    return fp


def classify_by_hash(hash_info: dict, fingerprints: dict) -> str | None:
    """LIB_ category if this function's normalized hash is a known library fingerprint."""
    if not hash_info:
        return None
    h = hash_info.get("hash")
    if h and hash_info.get("instrs", 0) >= FP_MIN_INSTRS and h in fingerprints:
        return fingerprints[h]["category"]
    return None


def run(apply: bool) -> int:
    fns = _all_functions()
    print(f"scanned {len(fns)} functions")
    hits, protected, already = [], [], 0
    for name, addr in fns:
        tag = _classify(name)
        if not tag:
            continue
        existing = _tags_of(addr)
        if existing & set(PROTECT_TAGS):
            protected.append((name, tag))          # game logic -- refuse to exclude
            continue
        if existing & set(LIB_TAGS):
            already += 1   # already LIB-tagged -- still process, to (re-)ensure the
                           # Scope property + retire any old SCOPE bookmark (idempotent)
        hits.append((name, addr, tag))

    by_lib: dict[str, int] = {}
    for _n, _a, t in hits:
        by_lib[t] = by_lib.get(t, 0) + 1
    print(f"\n{'WOULD TAG' if not apply else 'TAGGING'} {len(hits)} library/runtime functions:")
    for t in sorted(by_lib):
        print(f"  {t:14} {by_lib[t]}")
    for name, addr, t in hits:
        print(f"    {t:14} {addr}  {name}")
    if protected:
        print(f"\nPROTECTED (name looks library-ish but carries a rung -- left in scope): {len(protected)}")
        for name, t in protected:
            print(f"    ~{t:13} {name}")
    if already:
        print(f"of those, already LIB_-tagged (re-ensuring property + cleanup): {already}")

    if not apply:
        print("\nDRY RUN -- nothing written. Re-run with --apply to tag.")
        return 0

    try:                       # ensure the Scope property map (idempotent)
        _post("/create_property_map", {"name": "Scope", "type": "string", "program": PROGRAM})
    except OSError:
        pass
    ok = 0
    for name, addr, t in hits:
        try:
            _post("/add_function_tag", {"function": addr, "tags": t, "program": PROGRAM})
            # Scope property = the LIB_ category (which library = the reason). Queryable
            # as a set via list_properties(map=Scope).
            _post("/set_property", {"map": "Scope", "address": addr, "value": t, "program": PROGRAM})
            _post("/delete_bookmark", {"address": addr, "category": "SCOPE", "program": PROGRAM})
            ok += 1
        except OSError as e:
            print(f"  FAIL {name}: {e}")
    try:
        _post("/save_program", {"program": PROGRAM})
    except OSError:
        print("  [warn] save_program failed -- tags are in memory; save Ghidra manually")
    print(f"\ntagged {ok}/{len(hits)} functions LIB_* (+ Scope property map), saved program")
    return 0


def export() -> int:
    """List the LIB_* exclusions using list_function_tags' authoritative use_count."""
    try:
        tags = json.loads(_get("/list_function_tags")).get("tags", [])
    except (OSError, json.JSONDecodeError) as e:
        print(f"error: {e}")
        return 1
    total = 0
    for t in tags:
        if t.get("name", "").startswith("LIB_") and t.get("use_count"):
            print(f"  {t['name']:14} {t['use_count']}")
            total += t["use_count"]
    print(f"total excluded (LIB_*): {total}")
    return 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--apply", action="store_true", help="write tags (default is dry run)")
    ap.add_argument("--export", action="store_true", help="list already-excluded functions")
    ap.add_argument("--build-fingerprints", action="store_true",
                    help="hash every name-classified LIB_ function into lib_fingerprints.json "
                         "(across all open programs) so renamed copies can be matched by content")
    ap.add_argument("--programs", default=None,
                    help="comma-separated program paths for --build-fingerprints (default: all open)")
    args = ap.parse_args()
    if args.build_fingerprints:
        if args.programs:
            progs = [p.strip() for p in args.programs.split(",") if p.strip()]
        else:
            try:
                r = json.loads(_get("/list_open_programs"))
                progs = [p.get("path") or p.get("program") for p in
                         (r.get("programs") or r.get("open_programs") or []) if isinstance(p, dict)]
                progs = [p for p in progs if p]
            except (OSError, json.JSONDecodeError):
                progs = [PROGRAM]
        print(f"building fingerprints from {len(progs)} program(s)")
        build_fingerprints(progs)
        return 0
    if args.export:
        return export()
    return run(apply=args.apply)


if __name__ == "__main__":
    raise SystemExit(main())
