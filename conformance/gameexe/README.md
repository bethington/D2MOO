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
| `measure_regargs.py` | Which functions take arguments in registers the ABI does not name? |
| `phase2_order.py` | What do we actually have to WRITE, and what can be PROVEN? Consumes the above and produces the work order. |

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
| Library (CRT), byte-identified | 112 | free — link the era `libcmt.lib` |
| CRT the byte lane missed (below `0x407000`) | 37 | free — also linked, not written |
| Import thunks | 35 | free — linker regenerates |
| Ghidra boundary fragments | 9 | not functions |
| **Launcher — the code we write** | **18** | 1,284 instructions |

So Game.exe is **"link the VS2003 CRT and write 18 functions"**. The 37
below `0x407000` are unmistakably runtime — `CRT_strtok`,
`__security_check_cookie`, `__ismbblead`, fourteen copies of
`_unlock_fhandle`, `__doserrno`, malloc/realloc, the locale and codepage
machinery — and land in the "authored" bucket only because the byte lane's
evidence was weak or ambiguous. Several wear game-flavoured names a
documentation pass gave them (`ALLOC_AllocateMemory`,
`D2LANG_LocaleMapString`); `doc_lint` cannot flag those today because they
carry neither a `LIB_*` tag nor a FID match.

**Of the 18, only 4 can be proven by identity.**

| Tier | Count | Functions |
| --- | --- | --- |
| `CONF_BYTEMATCH` | 4 | `FindConfigOptionIndex` ✅, `TrimWhitespaceDelimiters` ✅, `GAME_MigrateBetaRegistryKeys`, `ApplyProcessSecurityRestrictions` |
| `CONF_TRACE` / `CONF_VECTORS` | 14 | everything else, incl. the whole main flow |

**2 of 4 done.** Sources in `src/`, built to `build/`, verified via
`manifest.json`:

```
python verify_bytematch.py --manifest manifest.json
0x004078e0   _FindConfigOptionIndex@4               BYTEMATCH
0x004079d0   _TrimWhitespaceDelimiters@4            BYTEMATCH
CONF_BYTEMATCH: 2/2
```

Build with the era compiler (`/O2`), one object per source file:

```
VS7/Bin/cl.exe /nologo /c /O2 /Fobuild/parse.obj src/parse.c
```

### LTCG contaminates callers, not just callees

This is why the number is 4 and not 8. A function taking only stack
arguments still cannot be byte-matched if it **calls** one that takes
arguments in unnamed registers — the call-site setup is part of its bytes.
`GameEntryPoint` (WinMain) is the clean example: ordinary stack parameters,
but it ends

```
LEA ECX,[ESP]        ; ECX = &argv
MOV EAX,0x2          ; EAX = 2
CALL GAME_InitializeAndStartGame
```

MSVC cannot be asked to emit that from a declaration — `__fastcall` gives
ECX/EDX and there is no spelling for "argument in EAX". The cap propagates
along call edges, and it caught `GameEntryPoint`, `FlushFileDescriptor`,
`ParseAllCommandLineOptions` and `GAME_LoadConfigFromIniFile`.

Worth measuring rather than assuming for those four: they differ from the
original only in the argument-setup instructions before an LTCG call, so
`verify_bytematch.py`'s `DIFF` output (which reports the differing offsets)
is far stronger evidence than a generic behavioural pass. Record the diff
count instead of discarding the comparison.

## Reconstruction playbook

Read the emitted code against the original and let the *differences* name
the next hypothesis. These four diagnostics did all the work so far:

| Symptom | What it means |
| --- | --- |
| `JB`/`JAE` where you emit `JL`/`JGE` | the loop bound is **unsigned** in the source (`sizeof`-based or an `unsigned` counter) |
| original saves **more** callee-saved registers than you | it has more simultaneously-live values — your structure is too simple |
| a constant materialised **twice**, or held in a register across blocks | a helper **inlined at two call sites**, its loop-invariant init then hoisted out of each enclosing loop |
| same instructions, different **order** (e.g. `INC` before vs after a store) | a `for`-increment lands after the body; an explicit `i++` in the body lands where you put it |

Measured iteration cost, `TrimWhitespaceDelimiters` (68 instructions):

| # | Hypothesis | Result |
| --- | --- | --- |
| 1 | D2MOO's `strspn`/`strcspn` source, de-modernised | 109 B vs 170 — wrong shape |
| 2 | explicit loops, one shared separator init | 145 B, 108 differ |
| 3 | separator test as an inlined helper | **170 B**, 5 differ |
| 4 | `i++` before the store, not a `for`-increment | **EXACT** |

Four hypotheses, each one read off the previous diff. Flags stayed
permissive throughout — the match holds under `/O2`, `/Ox`, `/O2 /Gy`,
`/O2 /GF`, `/O2 /Ob2`, `/O2 /Oa`, `/O2 /Gs` — so when something does not
match, suspect the source shape, not the flags.

## In progress: `GAME_MigrateBetaRegistryKeys` @ 0x00407ee0 (279 B)

Structure is understood; two specific gaps remain. Ghidra's boundary spans
**three consecutive statements** of D2MOO's `GameInit`, not just
`MoveBetaSettingsToRelease`: the beta-registry migration, then
`FOG_GetInstallPath` (a `__fastcall`, `ECX`=buffer / `EDX`=0x104), then a
`SetCurrentDirectoryA` guarded on a global. Constants confirm the buffers —
0x104 = `MAX_PATH`, 0x400 = `MAX_REG_KEY`, and 260 + 1024 + 260 + slack ≈
the 0x624 frame. The IAT loads hoisted into EDI/EBP/ESI are loop-invariant
motion the compiler does on its own; nothing to write for them.

First attempt: 310 B vs 279, 241 differing. Open questions:

1. **`AND ESP,0xfffffff8`** — the original realigns the stack to 8 and its
   frame is `0x624` where an unaligned build gives `0x61C`, exactly the 8
   bytes of padding. MSVC emits this for a local needing 8-byte alignment,
   and nothing in the reconstruction requires one. Which local is it?
   Do not paper over this by inventing a `double`; find the local.
2. **`char szPath[MAX_PATH] = {0}` is zeroed AFTER the registry block**,
   on the path both branches join, not at entry.

Cost signal worth carrying into the estimate: pure-logic functions
(`FindConfigOptionIndex`, `TrimWhitespaceDelimiters`) landed in 1 and 4
hypotheses. API-heavy functions with large frames and many relocations are
materially harder — the diagnostics still work, but there are more
independent variables per iteration.

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
