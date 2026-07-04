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

### D2Game is scrambled too (verified 2026-07-04)

The same divergence holds for **D2Game**, confirmed by function-identity
decompilation (not just the ordinal-space overlap, which is too weak to decide).
Sampled 8 ordinals; Ghidra's per-function ordinal stamp (`/* 0x… NNNNN */`)
confirms each address, yet **7 of 8 disagree** with the `.def` name:

| Ordinal | `.def` claims | Real PD2-S12 function (Ghidra-stamped) |
|---------|---------------|-----------------------------------------|
| @10002  | `GAME_InitGameDataTable`        | `GAME_EnqueueTimerEvent` |
| @10005  | `GAME_UpdateClients`            | `GAME_CleanupSessionItems` |
| @10010  | `GAME_SetInitSeed`              | DRLG bounded-random-pair generator |
| @10012  | `GAME_GetGamesCount`            | `Unwind_6fd179c0` |
| @10014  | `GAME_GetGameInformation`       | `GameStubReturnTrue` (stub) |
| @10016  | `GAME_GetPlayerUnitsCount`      | `InitializePerformanceFrequency` |
| @10020  | `GAME_GetStatistics`            | `GameStubReturnTrueAlt` (stub) |
| @10023  | `GAME_SetServerCallbackFunctions` | `StubNoOp` (stub) |

(@10006 `GAME_CloseAllGames` coincidentally landed on a close-all-games function.)
Note several real D2Game exports are **stubs** (`@10014/@10020/@10023`) — a
PD2-specific trait. Conclusion: trust `pd2s12_d2game_ordinals.tsv` + Ghidra, not
`D2Game.1.13c.def`, for D2Game drop-in wiring too.

## Authoritative maps (all three binaries)

`ordinal → runtime address`, parsed straight from each PE export directory:

| File                             | Binary      | Image base    | Exports |
|----------------------------------|-------------|---------------|---------|
| `pd2s12_d2common_ordinals.tsv`   | D2Common.dll| `0x6fd50000`  | **1172**|
| `pd2s12_d2game_ordinals.tsv`     | D2Game.dll  | `0x6fc20000`  | **61**  |
| `pd2s12_d2client_ordinals.tsv`   | D2Client.dll| `0x6fab0000`  | **4**   |

Regenerate any of them with:

```sh
DLL_PATH="C:\Diablo2\ProjectD2\D2Common.dll" python tools/dump_exports.py        # all
DLL_PATH="...\D2Common.dll" python tools/dump_exports.py 10375 11158 11026       # specific
```

To enrich a row with the documented function name, look the address up in the Ghidra
project (its plate comment also restates the ordinal as a cross-check).

### The export gradient drives drop-in feasibility (1172 → 61 → 4)

The three binaries are addressable to *very* different degrees, which is exactly why
the drop-in order is **D2Common → D2Game → D2Client**:

- **D2Common (1172 exports)** — every function is reachable through the export table,
  so it is fully **ordinal-swappable**: flip a `D2.Detours` patch by ordinal and the
  whole game picks up the replacement. Ideal first target. (This is also where D2MOO
  has the most reimplemented code.)
- **D2Game (61 exports)** — a narrow public API; most of its logic is internal and
  reached by direct address. Only the 61 exported entry points are ordinal-swappable;
  deeper functions need address patching.
- **D2Client (4 exports, `@10001–10004`)** — effectively **not** ordinal-swappable.
  Its functions are invoked by hardcoded addresses from the engine, so a D2Client
  drop-in requires inline-address hooking (version-specific and brittle), not export
  replacement. Last in the order for good reason.

## Coordinate family — verified so far (in `PD2S12Conformance.cpp`)

| D2MOO function                    | real ordinal | address     | Ghidra name                  | status                    |
|-----------------------------------|--------------|-------------|------------------------------|---------------------------|
| `DUNGEON_GameTileToClientCoords`  | `@10375`     | `0x6fd9dac0`| `TileToScreenCoordsInPlace`  | ✅ bit-exact              |
| `DUNGEON_GameTileToSubtileCoords` | `@11158`     | `0x6fd9d8a0`| `MultiplyValuesBy5`          | ✅ bit-exact              |
| `DUNGEON_ClientToGameCoords`      | `@11026`     | `0x6fd9d8c0`| `TransformToIsometric`       | ✅ bit-exact              |
| `DUNGEON_GameToClientCoords`      | `@10132`     | `0x6fd9db40`| `DUNGEON_GameToClientCoords` (was `TransformIsometricInverse`) | ✅ bit-exact **after fixing D2MOO** — PD2 uses `SUB;SAR`/`ADD;SAR` = `(x-y)>>1,(x+y)>>2`; D2MOO's `/2,/4` diverged on negatives. Fixed to `>>`. |
| `DUNGEON_GameSubtileToClientCoords`| `@11087`    | `0x6fd9db70`| `DATATBLS_ConvertTileToPixel`| ✅ bit-exact — plain `16(x-y),8(x+y)` (the `-16/+16` centered variant is a separate export, e.g. `ConvertTileToPixelCentered @0x6fd9d970 @10912`). |

**Coordinate family: complete (5/5 proven bit-exact).** Full suite 6/6 cases, 61/61
assertions. The `GameToClient` row is the conformance loop's first D2MOO **bug fix**
(commit `26ed87f`).
