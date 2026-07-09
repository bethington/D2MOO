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

### What a live proof licenses you to change (and what it does NOT)

A bit-exact live proof is strong evidence, but it proves a SPECIFIC thing: the
function's observable BEHAVIOR (return value + written bytes) on the tested inputs,
and the ABI the oracle called it with (calling convention, arg count, the field
OFFSETS the proven reimpl reads). It does NOT establish the function's semantic
NAME or higher-level purpose. So a proof licenses **different confidence per
artifact** -- do not let confidence in behavior leak into a rename:

| Artifact | A passing proof licenses | Still needs separate evidence |
|---|---|---|
| Calling convention, arg count | **HIGH** -- set it to what proved | (the proof is ground truth) |
| Integer widths, field OFFSETS the reimpl reads | **HIGH** -- the reimpl encodes them | (the proof is ground truth) |
| Parameter kind (scalar vs live-struct pointer) | **MEDIUM** -- a deref/index pattern shows struct-ness | the struct layout to name the exact type |
| Plate / behavior comment | **MEDIUM** -- describe the proven algorithm | -- |
| Function NAME / semantic purpose | **LOW** -- behavior is not a name | a string, xref, D2MOO canonical, or unambiguous domain meaning |
| `DOC_*` rung | evidence toward `DOC_REVIEWED` (ABI confirmed by behavior) | `DOC_VERIFIED` needs full type + name confidence |

The trap: a proof makes you CONFIDENT, so it is tempting to also "fix" the name.
Resist unless the name has its OWN ground truth. Bit-exactness on 8 live units
tells you WHAT the code does, not what it is CALLED.

### Renaming: ground-truth-to-ground-truth, else FLAG

Rename ONLY when the new name is backed by evidence stronger than the current
name's basis. Evidence for a name, strongest first:
1. A **D2MOO canonical name** the proven reimpl matches (same offsets/algorithm as
   a known D2MOO function).
2. A **string constant / format** the function references, or a clearly-named callee.
3. An **xref pattern** -- called from an already-named, understood site.
4. **Unambiguous domain behavior** (e.g. "returns the town level id for an act" ->
   `DUNGEON_GetTownLevelIdFromActNo`, which we DID rename from the decompiler guess).
5. Decompiler auto-name (`FUN_`/`Ordinal_`) -- no basis; always fair to replace.
6. A prior **model guess** (plausible-but-unverified) -- weak; do NOT replace with
   ANOTHER guess.

Decision tree:
- Current name is `FUN_`/`Ordinal_`/generic -> **rename freely** once behavior is clear.
- Current name is descriptive AND matches the proven behavior -> **keep**.
- Current name CONTRADICTS the proven behavior AND you have a ground-truth name
  -> **rename** (run the blast-radius checklist below).
- Current name contradicts the proven behavior but you have NO ground-truth name
  -> **DO NOT rename. FLAG it** with an audit note in the plate:
  `[AUDIT] name implies <X>, but proven behavior is <Y>; verify -- possible mis-name.`
  A wrong name is bad, but a second guess replacing it is not progress. A flag is
  non-destructive, loses no information, and lets the next pass (or a human) resolve
  it with real evidence. **Annotate-don't-overwrite is the default whenever your
  confidence is below ground truth.**

Worked example (2026-07-08): `HaveLightResBonus` proved 8/8, but the NAME implies a
boolean while the PROVEN behavior returns an int data field from the unit's `+0x2C`
sub-struct, type-dispatched (the plate itself says "type-dependent class data"). No
ground-truth replacement name existed -> FLAGGED with an `[AUDIT]` note, not renamed.

### When to REMOVE

Remove content only when it is FALSE or MISLEADING (a wrong comment, a bogus type,
a hallucinated xref). Never delete correct-or-uncertain content to "tidy up" --
downgrade it to a note if unsure. A later pass silently losing a TRUE fact is worse
than a little clutter. Status tags and append-only history are never removed by hand.

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
