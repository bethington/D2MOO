# Data-table conformance plan (PD2-S12 vs D2MOO)

Extends the behavioral oracle (see `behavioral/README.md`, which proved the pure
coord + RNG families) to the **data-driven** functions (`DATATBLS_*`, most
`ITEMS_*ByItemId`, `DRLG_*`, monster/level/item lookups).

## The key insight

The **getters are trivial** — e.g. D2MOO's
`DATATBLS_GetMaxLevel(nClass) = sgptDataTables->pExperienceTxt->tMax.dwClass[nClass]`.
A getter is just an index + field read. So the real conformance target is the
**loader**: `DATATBLS_CompileTxt(hArchive, "<name>", pFieldSchema, ...)` parsing a
`.txt`/`.bin` from an MPQ into the `sgptDataTables` struct graph. If both sides
load the same source data into the same struct, the getters match trivially.

- PD2's `sgptDataTables` global = **`g_pDataTables @ 0x6FDE9E1C`** (D2Common; 505
  xrefs). Getters dereference it.
- PD2's source data lives in MPQs: `pd2data.mpq`, `patch_d2.mpq` (Excel `.txt` +
  compiled `.bin`) under `C:\Diablo2\ProjectD2`.
- D2MOO loader API: `DATATBLS_CompileTxt(HD2ARCHIVE, szName, D2BinFieldStrc*,
  int* pCount, size_t)` in `source/D2Common/src/DataTbls/DataTbls.cpp`; needs an
  MPQ archive handle + the per-table field schema (D2MOO defines these).

## Architecture

- **PD2 side (ground truth):** Frida-read `g_pDataTables` (0x6FDE9E1C) in the live
  game, walk the struct graph, dump a target table's rows. Authoritative, no
  D2MOO needed. (Also: call the getters directly — they're `__stdcall`, no
  trampoline — for `(id -> value)` golden vectors.)
- **D2MOO side:** a standalone harness that opens PD2's MPQ (StormLib) — or reads
  an extracted `.txt` via a minimal archive shim — runs D2MOO's `DATATBLS_CompileTxt`
  for a target table, populates a `sgptDataTables` subset, and reads it back.
- **Compare:** D2MOO-loaded rows == PD2-live rows, field by field.

## Milestones

1. **Extract PD2 data.** Pull the `.txt`/`.bin` for one small, high-signal table
   (e.g. `Experience` or `CharStats`) from `pd2data.mpq`/`patch_d2.mpq`
   (StormLib `MPQExtractor`, or Ladik's MPQ Editor). Confirm which MPQ wins
   (patch order).
2. **PD2 golden capture.** Frida: read `g_pDataTables` -> `pExperienceTxt` ->
   `tMax.dwClass[0..6]` (struct offsets from D2MOO headers); and call the
   `__stdcall` getters for `(id -> value)` vectors. Emit `pd2_datatbl_<name>.json`.
3. **D2MOO loader harness.** New target linking D2Common; provide an MPQ handle
   (StormLib) or an archive shim over the extracted `.txt`; call
   `DATATBLS_CompileTxt` for the target table + its field schema; read the parsed
   rows.
4. **Compare** D2MOO-loaded vs PD2 golden (field-by-field). Then generalize the
   harness table-by-table.

## Scope note

Milestone 3 (the loader harness) is the real work: it needs StormLib (or an
archive shim) and the correct field schemas, and D2MOO's `DATATBLS_CompileTxt`
must run standalone. Milestones 1-2 are tractable immediately and produce the
golden reference the loader is validated against. Recommend doing 1-2 first
(concrete PD2 golden data), then the loader harness as a focused build.
