#!/usr/bin/env python3
"""gen_offline_tests.py -- CONF_REGRESSION tier codegen.

Reads the FROZEN corpora in conformance/regression/*.cases.json (each a set of
{input -> expected-output} cases captured from the REAL game -- see
freeze_corpus.py -- plus synthetic boundary cases) and emits a single doctest
.cpp that drives D2MOO's REAL compiled functions and asserts they still produce
those outputs. That generated file is compiled into D2CommonTests, so `ctest`
replays the whole corpus OFFLINE -- no running game -- and any code change that
breaks a proven function fails the build. This is the pre-merge regression gate.

The corpus is the source of truth; the generated .cpp is a build artifact
(checked in so the test build needs no Python). Re-run this after editing a
corpus:

    python conformance/tools/gen_offline_tests.py

ABIs supported:
  coord2  -- void __stdcall(int* pX, int* pY); case = {"in":{x,y},"out":{x,y}}

New ABIs are added by extending EMITTERS below (scalar-in/scalar-ret is next).
"""
from __future__ import annotations

import glob
import json
import os
import re

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))  # repo root
CORPUS_DIR = os.path.join(ROOT, "conformance", "regression")
OUT_CPP = os.path.join(ROOT, "source", "D2Common", "tests", "PD2S12Conformance.gen.cpp")


def emit_coord2(fn: str, fdef: dict) -> str:
    """void __stdcall(int* pX, int* pY): mutate the (x,y) pair in place."""
    cases = fdef["cases"]
    n = len(cases)
    nreal = sum(1 for c in cases if c.get("src") == "real")
    nsyn = n - nreal
    rows = []
    for c in cases:
        i, o = c["in"], c["out"]
        rows.append(f"\t\t{{ {i['x']}, {i['y']}, {o['x']}, {o['y']} }},")
    body = "\n".join(rows)
    return f"""// {fn}: {n} frozen cases ({nreal} real, {nsyn} synthetic)
TEST_CASE("CONF_REGRESSION: {fn} [coord2] ({nreal} real + {nsyn} synthetic)")
{{
\tstruct {{ int x, y, ex, ey; }} cases[] = {{
{body}
\t}};
\tfor (auto& c : cases)
\t{{
\t\tint x = c.x, y = c.y;
\t\t{fn}(&x, &y);
\t\tCHECK(x == c.ex);
\t\tCHECK(y == c.ey);
\t}}
}}
"""


def emit_scalar_ret(fn: str, fdef: dict) -> str:
    """RET __<cc>(scalar args...): pure integer-in/integer-ret. corpus function
    carries "args" (ordered param names); each case = {"in":{name:val,...},"ret":val}.
    Ints are passed straight to the real params (implicit narrowing to uint8_t etc.
    matches the ABI); ret is compared as long long so u32/i32 both fit."""
    argnames = fdef["args"]
    cases = fdef["cases"]
    n = len(cases)
    nreal = sum(1 for c in cases if c.get("src") == "real")
    nsyn = n - nreal
    fields = "".join(f"long long a{i}; " for i in range(len(argnames))) + "long long ret;"
    rows = []
    for c in cases:
        vals = ", ".join(f"{c['in'][a]}LL" for a in argnames)
        rows.append(f"\t\t{{ {vals}, {c['ret']}LL }},")
    body = "\n".join(rows)
    call = f"{fn}(" + ", ".join(f"c.a{i}" for i in range(len(argnames))) + ")"
    return f"""// {fn}({', '.join(argnames)}): {n} frozen cases ({nreal} real, {nsyn} synthetic)
TEST_CASE("CONF_REGRESSION: {fn} [scalar_ret] ({nreal} real + {nsyn} synthetic)")
{{
\tstruct {{ {fields} }} cases[] = {{
{body}
\t}};
\tfor (auto& c : cases)
\t\tCHECK((long long){call} == c.ret);
}}
"""


def emit_handle_getter(fn: str, fdef: dict) -> str:
    """RET __stdcall(void* pStruct): a getter that reads ONLY fixed offsets of the
    struct its pointer points to (no sub-pointer deref). There is no shipped D2MOO
    equivalent, so we EMBED the proven reimpl (from the candidate) as a static local
    and replay each frozen case against a SYNTHETIC zeroed struct with the captured
    field bytes written at their offsets. Guards the proven reimpl; regenerate after
    editing the candidate."""
    cases = fdef["cases"]
    size = int(fdef.get("struct_size", 256))
    n = len(cases)
    nreal = sum(1 for c in cases if c.get("src") == "real")
    nsyn = n - nreal
    src = os.path.join(ROOT, "conformance", fdef["reimpl_src"])
    if not os.path.exists(src):
        raise SystemExit(f"{fn}: reimpl_src not found: {src}")
    code = open(src, encoding="utf-8").read()
    code = re.sub(r'#include\s+"[^"]*provider_runtime\.h"\s*', "", code)   # resolver header not needed
    code = re.sub(r'//\s*D2MOO_REIMPL_EXPORT:.*\n', "", code)              # drop the build marker
    code = code.replace("__declspec(dllexport)", "")
    code = re.sub(r'\bextern\s+"C"\s*', "static ", code)                  # local linkage in the test TU
    code = re.sub(r'\b' + re.escape(fn) + r'\s*\(', f"reimpl_{fn}(", code)  # rename the definition
    code = code.strip()
    rows = []
    for c in cases:
        writes = " ".join(f"*(unsigned*)(buf+{off}) = {val}u;" for off, val in c["in"].items())
        rows.append(f"\t\tmemset(buf, 0, sizeof(buf)); {writes}\n"
                    f"\t\tCHECK((long long)reimpl_{fn}(buf) == {c['ret']}LL);")
    body = "\n".join(rows)
    return f"""// {fn}: embedded proven reimpl + {n} frozen struct-cases ({nreal} real, {nsyn} synthetic)
{code}

TEST_CASE("CONF_REGRESSION: {fn} [handle_getter] ({nreal} real + {nsyn} synthetic)")
{{
\tunsigned char buf[{size}];
{body}
}}
"""


EMITTERS = {
    "coord2": emit_coord2,
    "scalar_ret": emit_scalar_ret,
    "handle_getter": emit_handle_getter,
}


def main() -> int:
    corpora = sorted(glob.glob(os.path.join(CORPUS_DIR, "*.cases.json")))
    if not corpora:
        print(f"[gen_offline_tests] no corpora in {CORPUS_DIR}")
        return 1

    headers: set[str] = set()
    blocks: list[str] = []
    total_fns = total_cases = 0

    for path in corpora:
        with open(path, encoding="utf-8") as f:
            corpus = json.load(f)
        abi = corpus["abi"]
        emit = EMITTERS.get(abi)
        if emit is None:
            raise SystemExit(f"{os.path.basename(path)}: unknown abi {abi!r}; "
                             f"extend EMITTERS in gen_offline_tests.py")
        if corpus.get("header"):
            headers.add(corpus["header"])
        blocks.append(f"// ==== {os.path.basename(path)} "
                      f"(frozen {corpus.get('frozen_date', '?')}, {corpus.get('frozen_from', '?')}) ====")
        for fdef in corpus["functions"]:
            blocks.append(emit(fdef["fn"], fdef))
            total_fns += 1
            total_cases += len(fdef["cases"])

    inc = "\n".join(f'#include "{h}"' for h in sorted(headers))
    out = f"""// ============================================================================
//  PD2S12Conformance.gen.cpp -- GENERATED by conformance/tools/gen_offline_tests.py
//  DO NOT EDIT BY HAND. Edit the frozen corpora in conformance/regression/*.cases.json
//  and re-run the generator.
//
//  CONF_REGRESSION tier: replays {total_cases} frozen {{input -> real-game-output}}
//  cases across {total_fns} function(s) against D2MOO's REAL compiled D2Common,
//  entirely offline (no running game). A failure is a regression -- a code change
//  that made a proven function diverge from the captured ground truth.
// ============================================================================
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4820)  // padding added after struct member -- irrelevant for test tables
#pragma warning(disable: 4242 4244 4365)  // narrowing when a corpus int feeds a narrow param (uint8_t etc.)
#endif
#include <doctest.h>
#include <cstdint>
#include <cstring>

{inc}

{chr(10).join(blocks)}
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
"""
    with open(OUT_CPP, "w", encoding="utf-8", newline="\n") as f:
        f.write(out)
    print(f"[gen_offline_tests] {len(corpora)} corpus file(s) -> {total_fns} fn, "
          f"{total_cases} cases -> {os.path.relpath(OUT_CPP, ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
