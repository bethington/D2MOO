# Fresh-D2Common Documentation Baseline — 2026-07-09

**Non-destructive** benchmark on the REAL binary. `/testing/D2Common.dll` is a fresh
Ghidra auto-analysis (FUN_/Ordinal names, `undefined` types, no plates) of the actual PD2
`D2Common.dll` — byte-identical to our annotated `/Mods/PD2-S12` copy, so they map by
address. The MODEL doc-generation runs BLIND on the fresh copy; an INDEPENDENT judge
scores its output vs the annotated answer key. Our real work is never touched.

Why the model stages (not translators): fresh ≡ annotated bytes, so deterministic
translators are trivially/circularly 100% here. The non-trivial thing on real codegen is
the model's name/plate/prototype recovery.

## Baseline (5 proven golden functions) — overall **0.703**
| Function | prototype | name | plate | generated name (blind) |
|----------|-----------|------|-------|------------------------|
| DATATBLS_GetMissileParamShort0x10 | 1.0 | 0.7 | 0.65 | GetField0x10AsWord |
| STAT_GetUnitStatDirect | 1.0 | 0.5 | 0.75 | getFieldAtOffset0x28 |
| ITEMS_GetItemDataField32 | 0.5 | 0.5 | 0.65 | GetObjectAttributeField |
| ITEMS_GetItemRecordField104 | 1.0 | 0.4 | 0.55 | GetObjectKind |
| DATATBLS_GetItemTypeField10 | 1.0 | 0.5 | 0.85 | GetCharacterByteField |

**Axis means: prototype 0.90 · name 0.52 · plate 0.69.**

## Findings (the point of the benchmark)
1. **Prototype recovery is strong (0.90).** Param-count perfect (5/5). Ret-width mostly
   right; the ONE miss (ITEMS_GetItemDataField32, 0.5) is a GENUINE model over-
   generalization — it returned `int` for a proven `short` field. Same width-fidelity
   issue we fixed in the translators, now visible in the model path.
2. **Semantic NAME recovery is the weak axis (0.52) — the key finding.** Blind getters
   yield STRUCTURAL names (GetField0x10AsWord, GetObjectKind, GetCharacterByteField) that
   capture the MECHANISM but miss the DOMAIN (Missile/Stat/Item/ItemType). A lone getter's
   disassembly does not reveal what a field MEANS. => domain naming must come from CROSS-
   CONTEXT (callers/xrefs, struct-family, string refs), not single-function doc-gen. This
   is the concrete next lever to raise the score.
3. **Plate/structure is good (0.69) with model variance.** The algorithm/behavior is
   captured well; run-to-run wording variance is expected (nondeterministic model).
4. **The benchmark itself works and is honest**: it localizes exactly WHERE the pipeline
   is weak (naming) vs strong (prototype/structure), on production codegen, without risk.

## Bugs the benchmark caught (its purpose)
- Scope guard: a Bash `VAR=/testing` env gets MSYS path-mangled -> set the scope IN PYTHON.
- Scorer under-counted ret-width for unrecognized type spellings (`undefined`, `uint16_t`,
  WORD, ...) -> broadened `_TYPE_W`; proto axis 0.6 -> 0.9 after the fix (measurement, not
  model). Now stores the generated prototype for transparency.

## Re-run (after any pipeline change)
    python bench_d2common.py --extract --names <csv>   # PD2 scope; refresh answer key
    python bench_d2common.py --score                   # /testing scope; generate+judge
Compare `d2common_score.json` vs this baseline. Two complementary beds:
  * THIS (fresh D2Common) = production-fidelity MODEL-stage + regression, real codegen.
  * bench_core.dll = authoritative NAME truth + translator generality (other codegen).
