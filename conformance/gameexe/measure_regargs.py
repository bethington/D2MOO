"""How much of Game.exe can actually reach CONF_BYTEMATCH?

Per-translation-unit compilation cannot reproduce a function whose arguments
arrive in registers the ABI does not name — that is link-time codegen (/GL
+ /LTCG) inventing a private calling convention across TUs. Those functions
cap at CONF_TRACE / CONF_VECTORS, so their share is the ceiling on the
byte-match tier and needs measuring before the port sweep, not after.

GHIDRA'S SIGNATURE CANNOT ANSWER THIS. Measured 2026-08-08:
`GAME_ParseModStateFromCommandLine` @ 0x00407e00 reports a single
`Stack[0x4]` parameter while its first real instruction is `MOV EDI,EAX` —
reading EAX before anything wrote it. The disassembly is the authority (the
same rule the falsify checks and the shadow-dispatch manifest follow).

METHOD — read-before-write over the whole listing, in address order:
  * `PUSH <callee-saved>` at the top is a register SAVE, not a read
  * padding idioms (`LEA ECX,[ECX]`, `LEA ESP,[ESP]`, `MOV EDI,EDI`, NOP)
    are not reads -- VS2003 emits `8d 49 00` purely for alignment, and
    counting it made a clean stdcall function look like it took ECX
  * a CALL DEFINES the caller-saved registers (EAX/ECX/EDX). Without this
    every `MOV x,EAX` after a call reads a RETURN VALUE and would be
    misfiled as an incoming argument
  * any other read of a GPR that nothing has written yet is an incoming
    register argument

Two classifications kept out of the denominator entirely:

  THUNK   a lone `JMP dword ptr [IAT]` -- a linker-generated import stub,
          not authored code. 38 of them sit in one block at 0x004074xx and
          the first cut of this script filed every one as UNKNOWN, which
          understated byte-match reach by more than a third.
  LIBRARY anything the byte lane or FID already identified as CRT.

ABSTAIN RATHER THAN GUESS: a function whose listing we cannot read is
UNKNOWN. "Cannot tell" is not "clean" -- the CONF_BLOCKED rule.

ECX / ECX+EDX are NOT flagged: that is __fastcall, a real ABI convention
the compiler reproduces from a `__fastcall` declaration.

VALIDATION: `DATATBLS_FindConfigOptionIndex` (0x004078e0), independently
PROVEN byte-matchable, must classify STANDARD; `GAME_ParseModState-
FromCommandLine` (0x00407e00) must classify LTCG_CUSTOM on EAX. Both are
asserted at the end of the run -- a measurement whose known answers drift
is reporting noise.
"""
from __future__ import annotations

import json
import re
import sys
import urllib.parse
import urllib.request
from collections import Counter

GHIDRA = "http://127.0.0.1:8089"
PROGRAM = "/Mods/PD2-S12/Game.exe"

GPRS = {"EAX", "EBX", "ECX", "EDX", "ESI", "EDI"}
CALLEE_SAVED = {"EBX", "EBP", "ESI", "EDI"}
FASTCALL_REGS = {"ECX", "EDX"}          # a real convention, reproducible
SUB32 = {                                # 16/8-bit views of the same register
    "AX": "EAX", "AL": "EAX", "AH": "EAX", "BX": "EBX", "BL": "EBX", "BH": "EBX",
    "CX": "ECX", "CL": "ECX", "CH": "ECX", "DX": "EDX", "DL": "EDX", "DH": "EDX",
    "SI": "ESI", "DI": "EDI",
}
PADDING = re.compile(
    r"^(NOP|LEA\s+(ECX,\[ECX\]|ESP,\[ESP\]|EDI,\[EDI\])|MOV\s+(EDI,EDI|ESI,ESI))",
    re.I,
)


def get(path: str, **params):
    params["program"] = PROGRAM
    url = f"{GHIDRA}/{path}?" + urllib.parse.urlencode(params)
    with urllib.request.urlopen(url, timeout=90) as r:
        return json.loads(r.read().decode())


def canon(tok: str):
    tok = tok.strip().upper()
    if tok in GPRS:
        return tok
    return SUB32.get(tok)


def regs_in(text: str) -> set:
    """Every GPR named anywhere in an operand string."""
    out = set()
    for tok in re.findall(r"\b[A-Z]{2,3}\b", text.upper()):
        c = canon(tok)
        if c:
            out.add(c)
    return out


def analyse(instructions: list) -> tuple:
    """(incoming_regs, status) for one function."""
    # A lone indirect JMP is an import thunk, not authored code.
    if len(instructions) == 1:
        only = (instructions[0].get("instruction") or "").upper()
        if only.startswith("JMP") and "[" in only:
            return set(), "thunk"

    # A saved register is always RESTORED. Pre-scanning for the matching POP
    # is what separates "PUSH ESI to preserve it" from "PUSH ESI to pass it".
    # Without this, the standard MSVC prologue `SUB ESP,N` followed by
    # PUSH EBX/ESI/EDI read as three incoming register arguments — which is
    # exactly the bogus EBX,EDI,ESI signature the first run produced on 13
    # functions, most of them plainly ordinary CRT code.
    restored = {
        (ins.get("instruction") or "").upper().partition(" ")[2].strip()
        for ins in instructions
        if (ins.get("instruction") or "").upper().startswith("POP ")
    } & CALLEE_SAVED

    texts = [(ins.get("instruction") or "").upper() for ins in instructions]
    has_ret = any(t.startswith("RET") for t in texts)
    if not has_ret:
        # No epilogue in this listing — the function tail-jumps into a shared
        # one (VS2003 tail-merges epilogues heavily), so the matching POPs are
        # in a DIFFERENT function's instructions and the pre-scan above cannot
        # see them. Measured: `CopyStringOptimized` @ 0x00404670 is
        # `PUSH EDI / MOV EDI,[ESP+8] / JMP` — a textbook save, misread as an
        # EDI argument. Assume preservation rather than invent a convention.
        restored = set(CALLEE_SAVED)
        if len(instructions) <= 2:
            # One or two instructions with no return is a Ghidra boundary
            # artifact, not a function (`ResolveProcAddress` @ 0x00407ec0 is
            # a lone `PUSH EAX`). Out of the denominator entirely.
            return set(), "fragment"

    written, incoming = set(), set()
    for ins in instructions:
        text = (ins.get("instruction") or "").strip()
        if not text or PADDING.match(text):
            continue
        upper = text.upper()
        mnem, _, rest = upper.partition(" ")
        rest = rest.strip()

        if mnem in ("PUSH", "POP") and rest in GPRS:
            if mnem == "POP":
                written.add(rest)
            elif rest in restored:
                written.add(rest)      # preserved across the call, not consumed
            elif rest not in written:
                incoming.add(rest)     # pushed as an argument, never defined
            continue

        # A CALL DEFINES the caller-saved registers. Reading EAX afterwards
        # consumes a return value, not an argument.
        if mnem == "CALL":
            incoming |= regs_in(rest) - written     # indirect target via reg
            written |= {"EAX", "ECX", "EDX"}
            continue
        if mnem in ("RET", "LEAVE", "INT3", "HLT"):
            continue
        if mnem.startswith("J") or mnem == "LOOP":
            # Branch: the condition was set by an earlier compare we already
            # accounted for. Indirect targets still read their register.
            incoming |= regs_in(rest) - written
            continue

        ops = [o.strip() for o in rest.split(",")] if rest else []
        dst = ops[0] if ops else ""
        srcs = ops[1:]

        # xor/sub reg,reg is a zeroing idiom: a write, never a read.
        if mnem in ("XOR", "SUB") and len(ops) == 2 and ops[0] == ops[1]:
            d = canon(dst)
            if d:
                written.add(d)
            continue

        # Reads: every register named in a source operand, plus any register
        # used for addressing inside the destination.
        read = set()
        for s in srcs:
            read |= regs_in(s)
        if "[" in dst:
            read |= regs_in(dst)
        elif mnem in ("INC", "DEC", "NEG", "NOT", "ADD", "SUB", "AND", "OR",
                      "CMP", "TEST", "IMUL", "SHL", "SHR", "SAR"):
            read |= regs_in(dst)       # read-modify-write, or a pure compare

        for r in read - written:
            incoming.add(r)

        # Writes
        if "[" not in dst and mnem not in ("CMP", "TEST", "PUSH"):
            d = canon(dst)
            if d:
                written.add(d)
    return incoming, "ok"


def main() -> int:
    funcs = get("list_functions", limit=500)
    items = funcs.get("functions") or funcs.get("data", {}).get("functions") or []
    print(f"{len(items)} functions in Game.exe\n")

    # The authored set = everything the byte lane did NOT identify as CRT.
    lib_addrs = set()
    for b in get("list_bookmarks", limit=10000)["bookmarks"]:
        if b.get("category") == "Function ID Analyzer":
            lib_addrs.add(b["address"].lower())
    try:
        for t in get("search_functions_by_tag", tag="LIB_CRT", limit=1000).get("functions", []):
            lib_addrs.add(str(t.get("address", "")).lower().replace("0x", ""))
    except Exception:
        pass

    tally, rows = Counter(), []
    for f in items:
        addr = str(f.get("address", "")).lower()
        if addr in lib_addrs:
            tally["library(skipped)"] += 1
            continue
        try:
            d = get("disassemble_function", address=addr)
            ins = d.get("instructions") or d.get("data", {}).get("instructions") or []
        except Exception:
            tally["unreadable"] += 1
            continue
        if not ins:
            tally["unreadable"] += 1
            continue
        incoming, status = analyse(ins)
        exotic = incoming - FASTCALL_REGS
        if status == "thunk":
            verdict = "THUNK"
        elif status == "fragment":
            verdict = "FRAGMENT"
        elif status != "ok":
            verdict = "UNKNOWN"
        elif exotic:
            verdict = "LTCG_CUSTOM"
        elif incoming:
            verdict = "FASTCALL"
        else:
            verdict = "STANDARD"
        tally[verdict] += 1
        rows.append((addr, f.get("name", ""), verdict, sorted(incoming)))

    print(f"{'addr':10} {'name':44} {'verdict':13} regs")
    for addr, name, verdict, regs in rows:
        if verdict in ("LTCG_CUSTOM", "UNKNOWN"):
            print(f"{addr:10} {name[:44]:44} {verdict:13} {','.join(regs)}")

    # Known answers must not drift.
    by_addr = {a: (v, r) for a, _n, v, r in rows}
    checks = [("004078e0", "STANDARD", "proven byte-matchable"),
              ("00407e00", "LTCG_CUSTOM", "reads EAX at entry")]
    print("\n=== CONTROLS ===")
    ok = True
    for addr, want, why in checks:
        got = by_addr.get(addr, ("<missing>", []))[0]
        flag = "PASS" if got == want else "FAIL"
        ok &= got == want
        print(f"  {flag}  {addr} expected {want:13} got {got:13} ({why})")

    print("\n=== AUTHORED-SET TALLY ===")
    for k, v in tally.most_common():
        print(f"  {k:20} {v}")
    # Thunks and library code are linker/library output — not code we write.
    authored = tally["STANDARD"] + tally["FASTCALL"] + tally["LTCG_CUSTOM"] + tally["UNKNOWN"]
    reachable = tally["STANDARD"] + tally["FASTCALL"]
    if authored:
        print(f"\n  authored functions to reimplement: {authored}")
        print(f"  byte-match reachable:               {reachable}/{authored} "
              f"({reachable / authored * 100:.1f}%)")
        print(f"  capped at CONF_TRACE/VECTORS:       {tally['LTCG_CUSTOM']}")
    json.dump([{"address": a, "name": n, "verdict": v, "regs": r} for a, n, v, r in rows],
              open("regargs_report.json", "w", encoding="utf-8"), indent=2)
    print("\nwrote regargs_report.json")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
