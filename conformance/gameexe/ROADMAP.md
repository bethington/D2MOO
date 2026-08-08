# Byte-exact reconstruction — from manual proof to one-click workflow

**Written 2026-08-08.** Big-picture roadmap for turning the Game.exe
byte-match reconstruction into a repeatable, UI-kicked workflow that can be
pointed at another binary (e.g. D2Gfx.dll) and run to completion.

Five decisions locked with Ben this session drive everything below:

1. **Finish Game.exe by hand first** — one complete worked binary is the
   automation's target and its test oracle. Solve the hardest core functions
   manually so the agent design is grounded, not guessed.
2. **Agentic worker, iterate-to-match** — an LLM reads a function's
   disassembly, writes C, the harness compiles+scores, the LLM reads the
   byte-diff and iterates until byte-match or budget. The manual loop,
   automated.
3. **New fun-doc worker mode `reconstruct`** — reuse the binary picker,
   fleet, provider routing, dashboard. Select a binary, click start, a fleet
   works the manifest.
4. **Tiered ladder for "done"** — `CONF_BYTEMATCH` where reachable,
   behavioural/trace for what isn't, CRT linked-and-proven. 100% byte-exact
   is not the gate; highest reachable tier per function is.
5. **Roadmap doc (this), then reconstruct GameInit** as the first core
   function.

---

## Why this is uniquely automatable

The reward signal is **cheap, objective, and binary**: does the compiled
function equal the original's bytes (relocations masked)? No LLM judge, no
human review in the loop — ground truth on every compile. That is the ideal
condition for an agentic loop, and it is what separates this from fun-doc
documentation (which needs a fuzzy quality judge). The agent literally cannot
fool itself; `verify_tu.py` grades every attempt.

The honest caveat: the *reconstruction* step is real reverse engineering. The
agent does substantial work per function (read disasm, hypothesise source,
iterate on diffs), at real token cost, and a tail of functions — float
constants, jump tables, allocator idiosyncrasies (`MigrateBeta`'s
frame-pointer spill), genuine RTM/SP1-class differences — will not fully
byte-match from C. Those drop to the behavioural tier, which is exactly what
the tiered ladder is for.

## What is already built (the verification half — essentially done)

| Piece | Where | State |
| --- | --- | --- |
| Static byte-match scoreboard | `D2MOO/conformance/gameexe/verify_tu.py` | compiles the TU, scores every function vs original, MATCH/DIFF/short/LARGER |
| Manifest recovery (`.text` contiguity) | the manifest-build scripts | recovers function boundaries; **finds functions Ghidra missed** (2 so far) |
| Byte-compare primitives | `fun-doc/crt_identify.py` (`coff_functions`, `mask_bytes`) | COFF parse + relocation masking, reused everywhere |
| Reliable disassembler | `scratchpad/dis.py` (COFF-wrap + era dumpbin) | works where Ghidra truncates custom-convention functions |
| Patch-in + trace harness | `conformance/gameexe/patch_function.py`, `trace/` | whole-binary behavioural verification; hermetic, self-controlled |
| Exact toolchain | `C:\VS2003` (SP1, cl 13.10.6030) via `vcvars32.bat` | D2's exact compiler + Platform SDK |
| Playbook | `conformance/gameexe/README.md` | the diagnostics: js-vs-jl, epilogue sharing, unsigned loop bounds, TU/convention rules |

**The reconstruction half is the LLM-agentic work** — currently me, by hand.

## Game.exe completion path (finish this first)

Launcher TU = 24 functions at `0x407550..0x4085af`, one `Main.c`. Status:
**7 byte-exact**, ~6 bodies reconstructed and caller-pending, rest to write.

Reconstruct **bottom-up, but the core comes as one interconnected push** —
register-arg conventions only settle once the real callers exist:

```
leaves ✓  →  register-arg mid-layer (parsers, RENDER, SaveCmdLine,
             OpenServiceManager, ResolveProcAddress, GetInstallRoot)
          →  GameStart (683B)  →  GameInit (504B)  →  WinMain
```

Writing `GameInit`/`GameStart` pins the largest set of pending conventions at
once; a cluster of near-matches should snap to exact together on the final
compile. Known hard spot: `MigrateBeta`'s frame-pointer-EBP spill (source /
allocator, not toolchain — SP1-tested).

**Completion also means the behavioural gate**: a linked Game.exe (era CRT +
our objects) that traces identically to the original and launches PD2. The
trace harness is ready for this.

## The `reconstruct` worker (build after Game.exe is done)

**Loop per function (the agent's job):**
1. Fetch the function's disassembly (via `dis.py`, era-accurate).
2. Read `README.md`'s playbook + the diff from the previous attempt.
3. Write/patch its C in the TU's `Main.c`.
4. `verify_tu.py` compiles + scores → MATCH / DIFF(n)+offsets / LARGER / short.
5. If not MATCH, **read the SIDE-BY-SIDE disasm** (`cmp_fn.py <name> <addr>
   <len>`), not just the scoreboard number, and go to 3. The scoreboard's
   `short -N`/`LARGER +N` is a *total-length* delta and routinely lies about
   the cause — a byte-exact prologue can sit under a length gap (learned from
   `ParseCommandLineOption`: `szCommand[48]` fixed the frame, the −24 was
   register allocation, and the scoreboard showed neither).
6. On MATCH: record `CONF_BYTEMATCH`. Otherwise apply the **stopping rule**.

**The stopping rule (budget control — the answer to "when does the agent give
up?").** The residuals fall in two classes and the agent must tell them apart
from the side-by-side disasm:

- **Source-reachable** — different instructions, wrong branch sense, wrong loop
  bound, a missing/undersized buffer, a call the original makes and you don't.
  Keep iterating; the playbook rows name the fix.
- **Optimiser noise — STOP and drop to `CONF_TRACE`.** Same CALL targets, same
  memory operations, same control flow, but a different register assignment or
  a spill the original has and you don't (return value in a callee-saved reg vs
  `[esp+N]`; direct `[eax+esi]` vs stack-relative `[esp+edi+N]`). This is the
  optimiser's *global* scheduling and is generally not reachable by
  restructuring C. Grinding it is pure token burn. This is the hard tail the
  tiered ladder exists for — recognising it early is what keeps the fleet
  affordable.

The one caveat: a `RET n`/`RET 0` mismatch *looks* like optimiser noise but is
a **caller-pinned convention** — resolvable only by reconstructing the in-TU
caller, never by editing the callee (confirmed: removing/adding a second caller
of `ParseCommandLineOption` moved bytes but never pinned `ret 8`). So that one
belongs to the interconnected-core stage, not the per-function loop.

**Fleet / interconnection.** Leaves and standard-convention functions
parallelise cleanly (embarrassingly parallel, one agent each). The
interconnected core is NOT — it needs whole-TU convention pinning, so it runs
as a single coordinated stage (one agent holding the TU, or a
write-all-then-iterate pass). The manifest's call graph decides which.

**fun-doc integration (decision 3).** New worker `mode="reconstruct"` beside
functions/globals/port in `web.py`'s `start_worker`. Reuses the header binary
picker (`PROGRAM`), fleet cap, provider routing, dashboard heartbeats, roster
restore. New dashboard panel = the byte-match scoreboard (like the
Falsifiability panel). Per-binary state: a manifest + per-function tier in the
SQL store (a new lane, or reuse the CONF_ columns).

**Tiered ladder (decision 4).** Report per function:
`CONF_BYTEMATCH` (exact) > `CONF_BYTEMATCH_ABI` (exact but for LTCG call
setup) > `CONF_TRACE` (behaviourally verified) > `CONF_VECTORS` > unreached.
"Binary done" = every function at its highest reachable tier + the byte-exact
% reported; never gated on 100%.

## Scaling to D2Gfx.dll (and beyond)

- **Generalises directly**: manifest recovery, `verify_tu`, the toolchain,
  the CRT-link-for-free (link SP1 `libcmt`, ~half the binary proven).
- **New per-binary work**: TU-grouping recovery is harder for a big DLL
  (~344 functions, many objects) than a 22-function launcher — the
  `.text`-contiguity walk + padding boundaries + Rich-header object count are
  the signals. Data/struct/table recovery is per-binary. Ordinal exports
  (D2's DLLs) are the naming baseline, already understood.
- **Scale is a real jump**: 22 → ~344 functions, EXE → DLL. Prove the worker
  on Game.exe's remainder before pointing it at D2Gfx.
- **The dream flow**: select D2Gfx.dll in the fun-doc header → Start
  reconstruct → fleet works the manifest → dashboard shows the byte-match %
  climbing → a linked, trace-verified D2Gfx.dll drops out. Realistic as a
  *target*; the reconstruction tail always needs judgement.

## Risks / honest limits

- The hard tail never fully automates; the behavioural tier is the honest
  floor, not a failure.
- Token cost is real — an agent iterating a 683B function is many turns.
  Budget per function, and let cheap functions subsidise the scoreboard.
- Interconnected conventions can oscillate; the core needs a coordinated pass,
  not naive per-function parallelism.
- A wrong "match" is impossible (ground-truth verifier), but a wrong
  *behavioural* pass is not — keep the trace differ's normalisation visible
  and self-controlled, as built.
