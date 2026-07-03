# PD2-S12 D2Common export ordinal reconciliation

## Critical finding

**D2MOO's `source/D2Common/definitions/D2Common.1.13c.def` ordinals do NOT match the
actual Project Diablo 2 S12 `D2Common.dll` export table.** They are a *different*
scheme. A drop-in that patches the live PD2-S12 game by the `.def`'s ordinals would
replace the **wrong** functions.

Proof (all cross-checked three independent ways — the PE export directory parsed from
`C:\Diablo2\ProjectD2\D2Common.dll`, Ghidra's own preserved `Ordinal_NNNNN` labels,
and the per-function ordinal Ghidra stamps in each plate comment, e.g. `/* 0x45d30
10110 */`):

| Function (by decompiled formula)         | D2MOO `.def` says | PD2-S12 **real** ordinal | Address     |
|------------------------------------------|-------------------|--------------------------|-------------|
| `GameTileToClient` `(x-y)*80,(x+y)*80>>1`| `@10110`          | **`@10375`**             | `0x6fd9dac0`|
| `GameTileToSubtile` `x*5, y*5`           | `@10113`          | **`@11158`**             | `0x6fd9d8a0`|
| `ClientToGame` `2y+x, 2y-x`              | `@10109`          | **`@11026`**             | `0x6fd9d8c0`|
| `sgptDataTables` (data ptr)              | `@10042`          | **`@11170`**             | `0x6fde9e1c`|

And what actually lives at the `.def`'s claimed ordinals in the real binary:

| `.def` ordinal | D2MOO name there            | PD2-S12 real function at that ordinal          |
|----------------|-----------------------------|------------------------------------------------|
| `@10110`       | `DUNGEON_GameTileToClient`  | `ITEMS_CheckItemTypeFilter` (`0x6fd95d30`)     |
| `@10113`       | `DUNGEON_GameTileToSubtile` | a `StubNoOp` (`0x6fd9dbc0`)                     |
| `@10042`       | `sgptDataTables`            | `GetStatListField64` (`0x6fd84c10`)            |

The coordinate family, which the `.def` places in a tidy contiguous block
`@10107–@10117`, is in reality **scattered** across the export table
(`@10095, @10132, @10375, @10474, @10512, @10841, @11026, @11071, @11087, @11158`).

## What this means

- **Conformance results are unaffected.** The `PD2S12Conformance.cpp` tests match
  D2MOO's functions to PD2-S12 by *decompiled formula / signature*, never by ordinal.
  Every bit-exact result stands regardless of this mismatch.
- **The drop-in wiring must use the empirical map, not the `.def`.** When flipping a
  `D2.Detours` patch to replace a live PD2-S12 export, resolve the target by the
  authoritative ordinal below (or re-derive it), *not* by the `.def` ordinal.

## Authoritative map

`pd2s12_d2common_ordinals.tsv` — every PD2-S12 D2Common export (1172 rows),
`ordinal → runtime address` (image base `0x6fd50000`), parsed straight from the PE
export directory. Regenerate with:

```sh
DLL_PATH="C:\Diablo2\ProjectD2\D2Common.dll" python tools/dump_exports.py        # all
DLL_PATH="...\D2Common.dll" python tools/dump_exports.py 10375 11158 11026       # specific
```

To enrich a row with the documented function name, look the address up in the Ghidra
project (its plate comment also restates the ordinal as a cross-check).

## Coordinate family — verified so far (in `PD2S12Conformance.cpp`)

| D2MOO function                    | real ordinal | address     | Ghidra name                  | status                    |
|-----------------------------------|--------------|-------------|------------------------------|---------------------------|
| `DUNGEON_GameTileToClientCoords`  | `@10375`     | `0x6fd9dac0`| `TileToScreenCoordsInPlace`  | ✅ bit-exact              |
| `DUNGEON_GameTileToSubtileCoords` | `@11158`     | `0x6fd9d8a0`| `MultiplyValuesBy5`          | ✅ bit-exact              |
| `DUNGEON_ClientToGameCoords`      | `@11026`     | `0x6fd9d8c0`| `TransformToIsometric`       | ✅ bit-exact              |
| `DUNGEON_GameToClientCoords`      | (open)       | inline `0x6fd85b00` | `CalcIsometricScreenCoords` | ⚠️ divergence: PD2 projection uses arithmetic `>>1,>>2`; D2MOO uses truncating `/2,/4` (differ on negative deltas). Standalone export not yet located. |
| `DUNGEON_GameSubtileToClientCoords`| (open)      | —           | plain `16(x-y),8(x+y)` not yet located (`ConvertTileToPixelCentered @0x6fd9d970` is the centered `-16/+16` draw variant) | ⏳ |
