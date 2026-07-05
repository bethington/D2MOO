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

## Progress (2026-07-04)

- **Milestone 2 tooling built** (`behavioral/pd2_frida_datatbl.js` + `_capture.py`):
  read-only Frida dump of PD2's live `g_pExperienceTxtRecords` (verified via
  disassembly: `MOV [0x6FDF0B50],EAX` in `LoadExperienceTxtTable`, D2Common
  offset `0xA0B50`). No MPQ parsing needed on our side for this milestone --
  the running game already parsed it.
- **Finding: data tables load LATER than D2Common's code.** At the main menu,
  `g_pExperienceTxtRecords` is NULL -- D2Common's CODE is mapped at process
  start, but its data-table globals populate only once you go past the menu
  (character select or in-game). Capture needs a session at that point.
- **Milestone 3 (loader harness) confirmed harder than a simple call:**
  `ARCHIVE_OpenFile`'s `hArchive` param is **effectively unused** (see
  `D2Hell/include/Archive.h` comment) -- real file resolution goes through
  Fog's own global archive search across whatever MPQs are currently open, in
  priority order. So the harness must replicate D2's real MPQ-open sequence
  (open `pd2data.mpq`/`patch_d2.mpq`/etc via Fog/Storm in the right order)
  before calling any `DATATBLS_Load*Txt`, not just pass a handle.
- Top-level loader entry point found: `DATATBLS_LoadAllTxts(hArchive, a2, a3)`
  in `D2Common/src/DataTbls/DataTbls.cpp` calls every `DATATBLS_Load*Txt` in
  sequence (Experience via `DATATBLS_LoadSomeTxts` -> ... ); a fuller list of
  per-table loaders is there for whichever table gets targeted first.

## Milestone 3 rescoped (2026-07-04): it's bigger than a CMake target

Investigated the actual file-I/O path `DATATBLS_CompileTxt` depends on:
`ARCHIVE_OpenFile` -> `FOG_FOpenFile`. In D2MOO's `Fog.h`,
`FOG_FOpenFile` is declared via `D2FUNC_DLL(FOG, FOpenFile, ...)` — a macro
that **imports it directly from the real Fog.dll** (not a D2MOO
reimplementation). Fog.h has **108** such real-import declarations vs only
**22** `.cpp` files actually reimplementing Fog logic — so Fog is mostly a
real-DLL passthrough, and the real Fog.dll in turn resolves files by searching
whatever MPQs the game itself opened at startup (via the real Storm.dll),
in priority order. `DATATBLS_LoadAllTxts` also has no standalone
`DATATBLS_LoadExperienceTxt` — the Experience.txt load is INLINE inside
`DATATBLS_LoadAllTxts`, after ~30 other table loaders (ItemTypes, MonType,
Sounds, ItemStatCost, Properties, Missiles, States, Skills, CharStats, Items,
affixes, Sets, Gems, ..., then Experience). No precedent for this exists yet in
D2MOO (no test/tool currently initializes `sgptDataTables` at all); `sgptDataTables`
itself is a plain static global, so no dynamic init is needed there.

**Net: the loader harness is now "link the REAL Fog.dll + Storm.dll (from
ProjectD2) + D2MOO's D2Common, replicate Game.exe's own MPQ-open sequence
(which MPQs, what order/priority), then call `DATATBLS_LoadAllTxts`."** That
requires either (a) reverse-engineering Game.exe's own bootstrap via Ghidra to
get the exact archive list/order, or (b) finding it already reimplemented in
D2MOO's own Game/D2Launch modules (not yet checked). This is a genuine,
scoped reverse-engineering + build task -- bigger than originally estimated,
worth a dedicated session rather than being squeezed into an already long one.

## Milestone 3: the real MPQ-open sequence (found, 2026-07-04)

Traced it via the ghidra-mcp native tools (not the raw HTTP bridge). Game.exe
and D2Launch.dll are both thin (Game.exe has no MPQ-related strings at all --
pure CRT bootstrap). The real archive management lives in **D2Win.dll**:

- **`D2WIN_OpenMpqArchives`** (D2Win.dll, Ghidra VA `0x6F8EAA29`) is the
  top-level "open all core data archives" entry point -- an EXPORTED function
  called directly from another module's entry sequence (xref shows
  `From Entry Point [EXTERNAL]`). It opens, in this order, each via
  `AllocateAndOpenArchiveWithRetry(0x6f8fc9a8, path, ..., priority=0, NULL)`:
  1. `..\d2data.mpq`   -> `g_pMpqArchiveHandle0`
  2. `..\d2sfx.mpq`    -> `g_pMpqArchiveHandle2`
  3. `..\d2speech.mpq` -> `g_pMpqArchiveHandle1`
  4. `..\d2delta.mpq`  -> `g_pMpqArchiveHandle3`
  5. `..\d2kfixup.mpq` -> `g_pMpqArchiveHandle4`
  6. `..\d2video.mpq`  -> `g_pMpqArchiveHandle6` (via `g_szD2VideoVideoMpq`)
  7. `..\d2exp.mpq`     -> `g_pMpqArchiveHandle5` (only required if
     `FindAndValidateD2ExpMpq()` says the expansion is present)

  Required-for-success: d2data, d2sfx, d2speech all non-NULL, plus d2exp if
  expansion detected. On failure of any required archive, returns 0 (game
  won't start). **`patch_d2.mpq` is never named explicitly** -- consistent
  with known D2/Storm behavior where `SFileOpenArchive` transparently checks
  for and layers a `patch_*.mpq` counterpart when opening its base archive, so
  it doesn't need a separate open call.
- `AllocateAndOpenArchiveWithRetry` (D2Win.dll, VA `0x6F8E7E60`, source
  `D2Win\Archive.cpp`) allocates a 0x108-byte archive struct and calls
  **`ARCHIVE_OpenWithFallback`** (D2Win's own archive primitive, distinct
  from D2MOO's `D2Hell::ARCHIVE_OpenFile`) -- with a retry loop for
  CD-ROM-style prompts (`pfnRetryCallback`), harmless to skip for a harness
  (pass `NULL`).
- Separately, **`D2WIN_OpenCharacterAndMediaArchives`** (VA `0x6F8EA890`)
  opens `d2char.mpq`/`d2music.mpq` (+ `d2Xmusic.mpq`/`d2Xtalk.mpq`/
  `d2Xvideo.mpq` for expansion) -- character/media archives, NOT the core
  data archives Experience.txt lives in. Not needed for data-table
  conformance.

**Implication for the harness:** since `DATATBLS_CompileTxt`'s file I/O goes
through the real Fog.dll -> real Storm.dll (see rescope above), and Storm's
own archive-priority search only sees whatever's been opened via
`SFileOpenArchive`, the harness needs to open the same archive list *in the
same order* via the real Storm.dll (either by calling
`D2WIN_OpenMpqArchives` directly if D2Win.dll can be loaded standalone, or by
replicating its 7 `SFileOpenArchive`-equivalent calls directly against
Storm.dll) before calling `DATATBLS_LoadAllTxts`. This is now a concrete,
buildable spec rather than an open question.

## Scope note

Milestone 3 (the loader harness) is the real work: it needs StormLib (or an
archive shim) and the correct field schemas, and D2MOO's `DATATBLS_CompileTxt`
must run standalone. Milestones 1-2 are tractable immediately and produce the
golden reference the loader is validated against. Recommend doing 1-2 first
(concrete PD2 golden data), then the loader harness as a focused build.
