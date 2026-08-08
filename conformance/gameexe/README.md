# Game.exe conformance — byte-match verification

Game.exe is the worst case for the existing CONF_ ladder: `CONF_BATTLETESTED`
is earned through call volume under live shadow dispatch, but a launcher's
functions run **once at startup** — and its error paths (bad registry,
missing ini, absent renderer DLL) may run **never**. No amount of live
observation accumulates.

So confidence comes from two places instead:

1. **Identity.** If our compiled function is byte-identical to the original
   (relocation sites masked), it *is* that function. Nothing remains to
   observe, which sidesteps the run-once problem entirely.
2. **Forced behaviour.** For what cannot byte-match, *make* the rare paths
   execute in both binaries and diff every observable side effect.

Rungs: `CONF_BYTEMATCH` > `CONF_TRACE` > `CONF_VECTORS` > `CONF_DRAFT`.

## Tools here

| Script | What it answers |
| --- | --- |
| `verify_bytematch.py` | Is this compiled function byte-identical to the original? Takes `--obj/--symbol/--address` or a `--manifest`. |
| `verify_bytematch_control.py` | Does the verification chain work *at all*? Compares Game.exe's CRT against VS2003's `libcmt.lib`, with VC6's as a negative control. |
| `measure_regargs.py` | Which functions *can* reach `CONF_BYTEMATCH`, and which are capped by LTCG custom conventions? |

All three read from a live Ghidra on `:8089` and reuse fun-doc's
`crt_identify` primitives (`coff_functions`, `mask_bytes`) rather than
reimplementing COFF parsing or relocation masking. Set `FUNDOC_DIR` if
fun-doc is not at its default path.

## Measured, 2026-08-08 (PD2-S12 Game.exe, 211 functions)

**The chain is proven.** 103/103 FID-identified CRT functions are
byte-identical to VS2003's `libcmt.lib`; the same comparison against VC6's
`LIBCMT.LIB` matches 11/103 (10.7%), and those 11 are tiny SEH/mbcs helpers
genuinely shared by both toolchains. Bodies up to 829 bytes with 46
relocations match exactly.

**Hand-written C reached an exact match.** `DATATBLS_FindConfigOptionIndex`
@ `0x004078e0` — 92 bytes, 1 relocation, **88/88 informative bytes
identical**; the only raw differences were inside the relocation field
(our `0x00000020` placeholder vs the linked `0x0040bc28`).

**What the binary is made of:**

| Category | Count | Cost |
| --- | --- | --- |
| Library (CRT) | 112 | free — link the era `libcmt.lib` |
| Import thunks | 35 | free — linker regenerates |
| Ghidra boundary fragments | 9 | not functions |
| **Authored — to reimplement** | **55** | 42 reachable (76.4%), 13 LTCG-capped |

55 functions / 2,709 instructions; 22 are ≤15 instructions.

## Two lessons that govern the work

**Read the disassembly first, then write the source.**
`FindConfigOptionIndex` matched on the first hypothesis because the tell was
extracted from the listing beforehand: the original ends its loop with `JB`
(unsigned) where `for (int i = 0; i < 57; i++)` emits `JL` and 4 extra
bytes. Any `ARRAY_SIZE` (sizeof ⇒ `size_t`) or `unsigned` counter matches.
Flags turned out permissive — `/O2`, `/Ox`, `/O2 /Ob0`, `/Gy`, `/GF`, `/Oa`
all give identical bytes — but **VC6 matched zero cells at any setting**.
The compiler identity is not a detail.

**D2MOO's existing source is a semantic starting point, not a byte-match
starting point.** `ParseCmdValue` from `source/Game/src/Main.cpp`,
de-modernized to C89 and compiled with the true toolchain across 24
(source-form × flag) cells, produced no match and was not close — 109 bytes
against the original's 170. The listing says why: the original bounds its
scans with an explicit `strlen` (`CMP ECX,EDI`), which a real `strspn` never
needs, and rebuilds a **4-element** separator array on the stack before each
loop. Hand-written loops over a `char[4]`, not `strspn`/`strcspn`. Same
behaviour, different code. Budget per-function source reconstruction.

## Gotchas the measurement tooling had to learn

* `PUSH` of a callee-saved register is a **save**, not an argument read.
  The standard `SUB ESP,N` + `PUSH EBX/ESI/EDI` prologue otherwise reads as
  three register arguments — a bogus `EBX,EDI,ESI` signature on 13
  functions. Pair each PUSH with its POP: a saved register is always
  restored.
* A `CALL` **defines** EAX/ECX/EDX. Otherwise every `MOV x,EAX` after a call
  reads a return value and is misfiled as an incoming argument.
* VS2003 tail-merges epilogues, so a function can `JMP` away with its `POP`s
  in another function's listing; and a lone `PUSH EAX` with no `RET` is a
  Ghidra boundary artifact, not a function.
* Ghidra's *signature* cannot answer the convention question:
  `GAME_ParseModStateFromCommandLine` reports a `Stack[0x4]` parameter while
  its first real instruction is `MOV EDI,EAX`. The disassembly is the
  authority.

Plan: `~/.claude/plans/game-exe-reimplementation.md`.
