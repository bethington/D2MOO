# Benchmark DLL — ground-truth documentation benchmark

**Idea (user, 2026-07-09):** compile functions we FULLY understand (we authored them)
into a stripped DLL, load it fresh into Ghidra (auto-analysis only — Ordinal/FUN_
names, no types), feed it BLIND to the documentation pipeline, and score the output
against the known answer key. An objective measure of *how true* the generated
documentation is — because we know the answer.

## Layout
- `src/bench_core.c` — the ground-truth source (5 exported + 1 un-exported fn, real
  struct defs, rich doc comments). Exports are ordinal-only (NONAME) via `bench_core.def`,
  mirroring PD2's D2Common so fresh Ghidra shows no helpful names.
- `answer_key/bench_core.answer.json` — machine-readable ground truth (structs, fields,
  prototypes, reads, gates, plate summaries). What the pipeline must recover.
- `build.bat` → `build/bench_core.dll` — stripped 32-bit, `/O2` (realistic codegen).
- `fun-doc/bench_score.py` — runs the REAL production translators on the fresh binary and
  scores recovery vs the answer key. `python bench_score.py --program bench_core.dll`.

## Phase 1 baseline (2026-07-09) — mechanical axes, OFFLINE (no game, no model)
Aggregate **0.667** (n=5). Axes per fn: ret_width, field_reads recall, gate.

| Function | Shape | Recovered | Score |
|----------|-------|-----------|-------|
| bench_GetUnitLife | flat 2-level | flat_getter | **1.0** |
| bench_GetPlayerMaxLife | type-gated 2-level | flat_getter | **1.0** |
| bench_GetMonsterSpeed | global-table | BAIL | 0.333 |
| bench_GetMonsterSound | delegate (inlined→global-table) | BAIL | 0.333 |
| bench_ClampValue | pure computed | BAIL | 0.667 |

## What Phase 1 PROVED / FOUND (the point of the exercise)
1. **The loop works**: known-truth source → stripped DLL → fresh Ghidra (FUN_/Ordinal
   only) → production tooling → objective score vs answer key. Fully offline.
2. **The flat/type-gated getter translators are excellent** — perfect recovery of chain,
   width, and the dwType gate on a compiler they were NOT tuned against.
3. **Concrete generality gap** (the honest finding): our global-table + delegate
   translators are fit to PD2's codegen (EAX-based param, `IMUL; ADD`), and BAIL on
   MSVC-2022 `/O2` idioms (index in ECX, scaled-index `[base+idx*stride+off]`). Bail
   reason: "writes ECX, not the EAX chain". Extending the translators to handle ECX-base
   + scaled-index would generalize them AND lift this score toward 1.0.
4. **Compiler inlining is real**: `bench_GetMonsterSound`'s call to the un-exported
   `bench_LookupMonster` was INLINED, turning a delegate into a global-table getter. The
   field offsets/widths/behavior are preserved even when the SHAPE changes — so scoring
   anchors on offsets/widths, not the shape label.
5. **Codegen caveat is real & measured**: a DLL from our compiler ≠ PD2's disassembly, so
   benchmark translator-misses do not equal production failures. Value here = a controlled,
   objective doc-quality track; production fidelity stays on the real PD2 binary.

## Next (Phase 2+)
- Add the prose/name axes: run the model doc-gen + an INDEPENDENT judge vs the answer-key
  plate/names (covers ClampValue and semantic-name recovery).
- Roll in breadth (existing equivalents → clean standalone C).
- Optionally extend translators for ECX-base + scaled-index (generality + score lift).
