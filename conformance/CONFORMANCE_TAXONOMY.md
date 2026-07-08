# Conformance tagging taxonomy

The canonical scheme for marking, **in Ghidra**, how far each function has
progressed. Two **orthogonal** tag families (a function carries at most one rung
from each). This is the source of truth (writeback-source-of-truth principle);
`conformance/proven_functions.jsonl` is the machine-readable mirror of the CONF
axis.

Query any rung with `search_functions_by_tag`.

## Axis 1 -- `DOC_*` : documentation maturity (how well the function is understood)

Untagged = **undocumented** (auto-name / decompiler only). Rungs are mutually
exclusive; promoting removes the previous one.

| Tag | Meaning |
|-----|---------|
| `DOC_DRAFT` | First-pass, unverified -- model or decompiler prose, plausible signature, not confirmed. |
| `DOC_REVIEWED` | Careful pass -- parameters/behavior understood, calling convention confirmed **from the disassembly** (asm is ground truth over the decompiler's annotation). |
| `DOC_VERIFIED` | Fully understood and ground-truthed -- signature, types, behavior all confirmed, no open questions. |

## Axis 2 -- `CONF_*` : equivalence proof (is there a D2MOO one-for-one, how tested)

Untagged = **no equivalent** yet. Rungs are mutually exclusive; promoting removes
the previous one.

| Tag | Meaning |
|-----|---------|
| `CONF_DRAFT` | A D2MOO equivalent exists but is **untested** (first-run draft/prototype). |
| `CONF_VECTORS` | Passes **offline/static** vectors (Ghidra `/emulate_function` or offline conformance vectors) -- no live game process. |
| `CONF_LIVE` | Proven bit-exact **live** via the D2Debugger direct-call oracle on chosen inputs against the running game. |
| `CONF_BATTLETESTED` | `CONF_LIVE` **plus** zero divergences under shadow mode during **real gameplay** -- genuinely exercised (hits > 0) over >= 1 substantial session. |
| `CONF_REGRESSION` | Real+synthetic vectors **frozen** into the offline suite (`conformance/regression/*.cases.json` -> `D2CommonTests`), so `ctest` re-checks the SHIPPED D2MOO function with **no running game** -- a permanent pre-merge gate. Requires the reimpl promoted into shipping D2Common. See `CONF_REGRESSION_OFFLINE_SUITE.md`. |

## Why orthogonal

Documentation quality and equivalence-proof strength move independently: a
function can be `DOC_VERIFIED` with no equivalent yet, or `CONF_LIVE` while its
prose docs are still `DOC_DRAFT`. Keeping them separate lets each stage of the
pipeline advance its own axis.

## Who sets what

- **Documentation workflow** sets the `DOC_*` rung. AUTOMATIC on a completed
  fun-doc `functions`-mode run (opt-in via `FUNDOC_DOC_TAGS=1`), from the score:
  audit/verify score >= 95 -> `DOC_VERIFIED`; final score >= 80 (good_enough) ->
  `DOC_REVIEWED`; else `DOC_DRAFT`. Wired in `fun_doc.py::process_function` ->
  `port_live_prove.set_doc_level()` (mutually exclusive; non-fatal; keys by
  address so it survives renames).
- **Proving** sets the `CONF_*` rung:
  - static harness (`port_pipeline.run_harness`) -> `CONF_VECTORS`;
  - live oracle (`port_live_prove.run_live_prove` -> `record_proof`) -> `CONF_LIVE`
    (AUTOMATIC on every successful live proof; removes lower CONF_ rungs);
  - a real-gameplay shadow monitor -> `CONF_BATTLETESTED` (future promoter; earned
    from the D2Debugger shadow hit/divergence counters).
  - freezing the proven vectors into the offline suite -> `CONF_REGRESSION`
    (`freeze_corpus.py`/`capture_to_corpus.py` -> `regression/*.cases.json` ->
    `gen_offline_tests.py`); earned once `ctest` guards the shipped function offline.
- Any ABI/name/type correction found while proving is written back to Ghidra at
  the source (`set_function_prototype`, `rename_function_by_address`, apply types)
  and `save_program`'d -- never left only in chat/markdown.

### Editing existing documentation: correct vs preserve

Status TAGS (`DOC_*`/`CONF_*`) are additive by nature. Everything else --
names, comments, variable names, types, signatures -- follows **correctness +
confidence**, NOT additive-by-default:

- **Wrong/false/misleading** (misnamed fn, false comment, wrong type, `iVar1`) ->
  **correct it**. Falsehoods must not survive.
- **Correct + useful** -> keep it; add new info alongside.
- **Missing** -> add. **Partly wrong** -> surgically fix the wrong part.
- **Gate:** only overwrite with GROUND TRUTH (disassembly, a passing proof,
  authoritative D2MOO naming/strings/xrefs/struct layout). Never replace
  correct-or-unknown content with a lower-confidence guess -- add it as a note.
  Generic machine output (`FUN_`/`Ordinal_`/`iVar1`/decompiler ABI guesses) is
  always fair to improve. Contradicting prior HUMAN docs -> leave an audit note
  with the evidence.
- **Ripple:** the resolve_table and profiler tables key on GHIDRA names, so
  renaming to a D2MOO canonical name means regenerating those snapshots.

### Function-rename impact checklist

A rename's blast radius, by category (do the FUNCTIONAL ones; the rest are
consistency/cosmetic). "Keyed by ADDRESS" artifacts never break on a rename.

**In Ghidra (source of truth):**
1. The function name -- `rename_function_by_address`. Must satisfy the project's
   naming convention (Ghidra REJECTS bad names; globals need `g_` + Hungarian
   prefix -- use `set_global`). Xrefs auto-follow (address-based).
2. The function's own plate comment, if it embedded the old name -- update.
3. RELATED SYMBOLS whose name/comment mention the old name: the globals it
   reads, callers/callees. Often a rename implies renaming a related data symbol
   too (evidence permitting) -- e.g. the town fn's array `g_dwBoundedArrayData`
   -> `g_anTownLevelIds`, and its comment named the old function.
4. `save_program`.

**Generated snapshots (regenerate; do NOT hand-edit the .gen files):**
5. `conformance/corrected_maps/<mod>.tsv` -- the ground-truth export, name column.
   Update it (or re-export from Ghidra); it is the source for:
6. `D2.Detours.patches/1.13c/D2Common_ResolveTable.gen.h`
   (`gen_resolve_table.py`) -- **FUNCTIONAL if the fn is resolved BY NAME**
   (reimpl imports via the MemoryModule resolver, or the oracle's name fallback).
   Aligning Ghidra->D2MOO names actually HELPS name-resolution consistency.
   Runtime effect needs a D2Common patch REBUILD.
7. `conformance/profiler/<mod>.functions.tsv` (`build_profiler_tables.py`) --
   cosmetic (profiler UI display name + auto category); offsets unaffected.

**Authored artifacts (edit if they state the CURRENT name; annotate if historical):**
8. `conformance/proven_functions.jsonl` -- update `name`, keep `ghidra_prev_name`.
9. The reimpl candidate + spec: the export name must match what's RESOLVED. If a
   spec resolves by OFFSET/ADDR (recommended), a rename can't break it.
10. Plans/READMEs/taxonomy stating the current name -- update. Historical/dated
    status notes -- leave, or annotate "(renamed ...)".

**Never touch:** append-only history (fun-doc `logs/runs.jsonl`, `ghidra_http.jsonl`).

**Verify fun-doc state keys by ADDRESS, not name**, so its per-function state
survives renames (the address is stable; the name is not).

## Current state (2026-07-06)

8 functions at `DOC_REVIEWED` + `CONF_LIVE` (migrated from the retired single
`CONFORMANCE_PROVEN` tag): DUNGEON_GameToClientCoords, DUNGEON_GetTownLevelIdFromActNo
(renamed from the decompiler's DATATBLS_GetBoundedArrayValue), GetSeedHi,
GetItemRandSeed, InitRngSeed, SetCoordPair,
SEED_GetRandomNumber, GetDataTableRowEntryCount. None `CONF_BATTLETESTED` yet
(needs a real-gameplay shadow run).
