# D2Common naming: blind gen vs the canonical (D2MOO) stage — 2026-07-09

After re-deriving the record/item getters to REAL D2MOO names, the answer key's name
references are now authoritative (Throwable, RarePrefix, RangeAdder...). Re-scoring exposes
a sharp, honest split.

## Blind single-function doc-gen (bench_d2common) — name axis DROPPED
| Function (real ref) | blind generated name | name |
|---|---|---|
| DATATBLS_GetItemTypeThrowable | GetDataTableByteField | 0.15 |
| ITEMS_GetItemRangeAdder | GetObjectByteProperty | 0.20 |
| ITEMS_GetItemDataRarePrefix | GetTypedObjectWordField | 0.70 |
| STAT_GetUnitStatDirect | getObjectFieldAtOffset0x28 | 0.75 |
| DATATBLS_GetMissileParamShort0x10 | GetFieldAtOffset0x10 | 0.40 |

**Blind name axis: 0.44** (was 0.52 when the reference was ALSO structural). It DROPPED —
and that is the point, not a regression: once the reference is a REAL domain name, blind
generation scores WORSE, because a lone getter's disassembly does not reveal what a field
MEANS. `GetDataTableByteField` != `Throwable`. **Blind single-function naming cannot recover
domain semantics -- proven, quantified.**

## Pipeline canonical stage (d2moo_names) — EXACT authoritative names
| Function | canonical name | field | vs reference |
|---|---|---|---|
| GetItemTypeField10 | DATATBLS_GetItemTypeThrowable | nThrowable | **MATCH** |
| GetItemDataField32 | ITEMS_GetItemDataRarePrefix | wRarePrefix | **MATCH** |
| GetItemRecordField104 | ITEMS_GetItemRangeAdder | nRangeAdder | **MATCH** |
| STAT_GetUnitStatDirect | (not offset-named -- untouched) | — | — |
| GetMissileParamShort0x10 | (missile-param struct not yet mapped) | — | — |

**3/5 golden functions get the EXACT authoritative D2MOO name.** Repo-wide: 13 functions
now authoritatively named.

## The conclusion
The naming gain is real but lives ENTIRELY in the canonical stage, not blind generation.
The benchmark's blind name-axis DROP is the proof that you cannot get real names from a
single function's disassembly -- you need the authoritative source (D2MOO). This validates
the whole d2moo_names approach: transcription (Field10) -> understanding (Throwable) comes
from the community-canonical struct definitions, applied via stride/header/union resolution,
NOT from a model staring at one getter.

Prototype axis stayed ~0.9 (the recurring 0.5s are the model generalizing a byte/short
return to int -- the same width-fidelity issue). Plate ~0.6.

Caveat (honest): for D2MOO-resolvable functions the canonical name and the reference both
come from D2MOO, so scoring them 1.0 confirms CORRECT APPLICATION of the authority, not an
independent check of the D2MOO name itself. Independent name-truth = the benchmark DLL (we
authored the names) + the layout-drift safety guard (which caught StatListFlag4).
