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

## Milestone 3 built (2026-07-04): compiles, links, runs -- one architectural blocker left

`conformance/behavioral/d2moo_loader_harness.cpp` (CMake target
`d2moo_loader_harness` in `source/D2Common/tests/CMakeLists.txt`, following
the proven `d2moo_fingerprint` pattern) links the REAL compiled D2Common
(static) and calls `SFileOpenArchive`/`DATATBLS_LoadAllTxts` directly. Note:
PD2 uses its OWN consolidated archive set (`pd2data.mpq`, `pd2assets.mpq`,
`pd2maps.mpq`, `patch_d2.mpq`), NOT vanilla `d2data.mpq`/etc -- ProjectD2's
directory has no vanilla-named MPQs at all.

Build note: the Storm import is `D2FUNC_DLL_NP` ("No Prefix"), so the real
symbol is `SFileOpenArchive`, not `STORM_SFileOpenArchive`.

**Confirmed D2MOO's own freshly-built Storm.dll doesn't implement real MPQ
parsing** (every open fails; matches the Fog finding -- these modules are
mostly real-DLL passthroughs). Running with it hit `DATATBLS_LoadAllTxts`'s
own `D2_ASSERT(pData)` exactly where expected (no archive open -> null data)
-- validates the harness's call flow is correct.

**Swapped in the REAL Storm.dll/Fog.dll/D2CMP.dll/D2Lang.dll** (copied from
ProjectD2) to get real MPQ parsing. Hit `DLL_NOT_FOUND` -> `dumpbin
/dependents` revealed PD2's `Storm.dll` hard-depends on **`PD2_EXT.dll`**
(a PD2-specific module, likely Detours-based hooks). Added it -> progressed to
`ERROR_DLL_INIT_FAILED` (1114). Isolated via direct `LoadLibrary` calls in
**32-bit** PowerShell (`SysWOW64\WindowsPowerShell`, since these are 32-bit
DLLs and default PowerShell is 64-bit): `PD2_EXT.dll` itself returns 1114 when
loaded outside the real game process -- it's almost certainly a hook-installer
requiring genuine game context, and Storm.dll's dependency on it cascades the
failure to everything that in turn depends on Storm.

**This is the SAME class of blocker as "D2 game DLLs can't load standalone"
(known from Tier-2 work) -- now confirmed to also apply to PD2's shipped
Storm.dll transitively, via PD2_EXT.dll.** The harness itself is complete and
correct up to this point; the remaining blocker is architectural, not a bug.

## MILESTONE 3 DONE (2026-07-04): D2MOO loader proven == PD2 live, standalone

**The whole data-table conformance loop is closed.** D2MOO's real
`DATATBLS_CompileTxt`, parsing PD2's real `experience.txt` out of `pd2data.mpq`
with NO game process, is **BIT-EXACT** vs the live-game golden capture
(`behavioral/pd2_experience.json`) across all 12 rows we have golden data for.
Artifacts: `behavioral/d2moo_loader_harness.cpp` (CMake target
`d2moo_loader_harness`), `behavioral/run_loader_harness.ps1` (runtime-dep
staging), `behavioral/d2moo_experience.json` (committed proof).

**The PD2_EXT.dll breakthrough:** `dumpbin /imports` on PD2's `Storm.dll` showed
it imports exactly **3** symbols from `PD2_EXT.dll`, all by name:
`GetFileVersionInfoA`, `GetFileVersionInfoSizeA`, `VerQueryValueA` -- i.e.
`version.dll` APIs. `PD2_EXT.dll`'s own exports are all pure forwarders to
`version.dll`. So `PD2_EXT.dll` is a **`version.dll` DLL-hijack** the mod uses to
bootstrap (its `DllMain` installs PD2's hooks / loads further modules, and fails
with `ERROR_DLL_INIT_FAILED` 1114 outside `Game.exe`). **Fix: replace
`PD2_EXT.dll` with a plain copy of the Windows `version.dll` (renamed).** That
satisfies Storm's 3 imports with zero mod bootstrap, so PD2's real Storm.dll
loads and parses MPQs cleanly. No StormLib, no vanilla-Storm hunt needed.

**Other keys:**
- PD2's archives are **stock MPQ v1** (PKWARE implode, unencrypted) -- custom
  contents, stock container -- so real Storm parses them fine (verified by
  reading the MPQ headers directly).
- Call `DATATBLS_CompileTxt("experience")` directly (the exact call the game
  makes), NOT `DATATBLS_LoadAllTxts` -- the latter stack-overflows deep in its
  ~30-table chain on some PD2-modified table. Set `DATATBLS_LoadFromBin = FALSE`
  to parse the `.txt` (PD2 ships `.txt`, not pre-compiled `.bin`).
- Runtime deps staged next to the exe: real `Storm.dll`/`Fog.dll`/`D2CMP.dll`/
  `D2Lang.dll` (D2MOO's own rebuilt Storm/Fog are stubs that don't parse MPQs) +
  `version.dll`-as-`PD2_EXT.dll`.
- **`experience.txt` exists in BOTH `pd2data.mpq` and `patch_d2.mpq` with
  DIFFERENT payloads** -- MPQ priority is load-bearing. Our open order resolves
  to the same copy the live game uses (proven by the bit-exact match).

## Generalizing (next, straightforward)

The harness is now a template: to conform ANY other table, add its
`DATATBLS_CompileTxt(name, fieldSchema, sizeof(record))` call (the field schemas
are all in `DATATBLS_Load*Txt` in `DataTbls.cpp`), and capture the matching PD2
golden via a Frida read of the corresponding `sgptDataTables->pXxxTxt` global.
The two hard problems (open PD2's MPQs standalone; get PD2 golden data) are both
solved and reusable.

## Corrections from review (2026-07-04)

An independent review of this series found the work substantively real (the
RNG golden vectors were independently re-derived from the LCG formula across
all 185 captured calls, zero mismatches; the PD2_EXT.dll/version.dll-hijack
discovery is well-evidenced) but flagged two claims above that overstate what
was actually shown:

- **"Caught+fixed a real MPQ patch-priority divergence" (the DifficultyLevels
  commit) changed only this harness's `SFileOpenArchive` open order** —
  no D2MOO product/shipping code was touched. If D2MOO is ever used as a
  drop-in that opens its own archives (rather than always running under the
  real game's already-opened archive set), this same patch-priority class of
  divergence is still live there and unaddressed. Read "fixed" here as "fixed
  in the conformance harness," not "fixed in D2MOO."
- **"Milestone 3 DONE" / "the whole data-table conformance loop is closed"
  overgeneralizes.** It's proven bit-exact for exactly two files
  (`experience.txt`, `difficultylevels.txt`), captured live with PD2_EXT's
  hooks active but replayed here with those hooks *absent* (the harness
  substitutes a plain `version.dll` for `PD2_EXT.dll` specifically to avoid
  the mod bootstrap). If PD2_EXT hooks file resolution or archive layering at
  all (common for mod loaders), other files could resolve differently even
  though these two didn't. The harness's "open `patch_d2.mpq` first" rule is
  also an *empirical* fit for these two files, not a proven general model of
  Storm's priority search — the real game (per this doc's own §"Milestone 3:
  the real MPQ-open sequence") never opens `patch_d2.mpq` explicitly at all;
  Storm auto-layers it. Treat "the loop is closed" as true for this
  demonstrated slice, not as a general guarantee for arbitrary tables/files
  until more of them are captured and diffed.
- **No automated comparator re-checks the "bit-exact" claim.** The committed
  `d2moo_experience.json`/`d2moo_datatables.json` vs `pd2_experience.json`/
  `pd2_datatables.json` do match today (independently re-verified during
  review), but nothing in CI or a script re-asserts that on future changes —
  `run_loader_harness.ps1` references a "compare_fp.py-style diff" that
  doesn't exist for datatables. Worth adding before generalizing further.
- **A real divergence was found and routed around, not filed:** the harness
  comment notes `DATATBLS_LoadAllTxts` stack-overflows partway through PD2's
  ~30-table chain on some PD2-modified table, which is why the harness calls
  `DATATBLS_CompileTxt` per-table instead. That's exactly the kind of
  divergence this project exists to catch — it should be tracked as an open
  issue (which table, why it overflows), not just silently avoided.

## Scope note

Milestone 3 (the loader harness) is the real work: it needs StormLib (or an
archive shim) and the correct field schemas, and D2MOO's `DATATBLS_CompileTxt`
must run standalone. Milestones 1-2 are tractable immediately and produce the
golden reference the loader is validated against. Recommend doing 1-2 first
(concrete PD2 golden data), then the loader harness as a focused build.
