# OpenD2 → PD2-S12 data integration plan

**Implementation target repo:** `C:\Users\benam\source\cpp\OpenD2` (the portable
engine), NOT this D2MOO repo. This plan lives here because `conformance/` is the
PD2-S12-facing planning hub, but every code change below edits **OpenD2** files.
D2MOO stays the byte-compatible drop-in / conformance oracle; OpenD2 is the
portable game engine that must learn to load PD2's data. See
`DATATABLE_CONFORMANCE_PLAN.md` for the D2MOO-side data-table work this
complements.

**Goal:** make OpenD2 boot and play against `C:\Diablo2\ProjectD2` (PD2-S12).

---

## Current state (verified 2026-07-04)

OpenD2 today runs against **vanilla** Diablo 2, not PD2. Evidence: its debug log
(`OpenD2/build_allegro/Debug/debug.log`, `debug_engine.log`) shows it loading
vanilla `d2char.mpq` DCC sprites (`data\global\chars\PA\...`,
`data\global\monsters\...`) and a level-94 Paladin save, reaching in-game with
player + monster rendering. So the engine, MPQ reader, DCC/DC6 decode, TBL string
tables, save loading, and unit rendering already work — against vanilla assets.

Two concrete blockers stop it from running on PD2's data, plus the existing
conformance work (Phase B) for faithfulness.

### Blocker 1 — MPQ archive names are hardcoded vanilla

`OpenD2/Engine/FileSystem_MPQ.cpp` `Init()` hardcodes vanilla archive names
(`d2data.mpq`, `d2char.mpq`, `d2sfx.mpq`, `d2exp.mpq`, `patch_d2.mpq`, ...).
`C:\Diablo2\ProjectD2` contains **none of the vanilla-named data archives** — it
ships a consolidated set instead:

```
pd2data.mpq  pd2assets.mpq  pd2maps.mpq  patch_d2.mpq   (+ d2gl.mpq, D2MultiRes.mpq, SGD2FreeRes.mpq, ver-IX86-1.mpq)
```

Only `patch_d2.mpq` overlaps by name. Pointing `+basepath` at ProjectD2 today
fails to open 9 of 10 hardcoded archives → almost no assets load.

### Blocker 2 — most data tables are on a dead `.bin`-only path

`OpenD2/Modcode/Common/DataTables.cpp` `DataTables_Init()`: only **3** tables
(Levels, LvlTypes, LvlPrest) load via the real MPQ-aware `TXT_ParseFile`. ~15
others call `DataTables_Load(...)`, which **only reads precompiled `.bin`** — its
`.txt` fallback (`DataTables.cpp` ~line 231) builds a path string and `return 0`
without parsing. PD2 ships `.txt`, not `.bin` (confirmed in
`DATATABLE_CONFORMANCE_PLAN.md`), so those tables load **zero records** against
PD2 — silently, with no error.

---

## Phase A — Boot & play on PD2 data (the unlock; ~days)

### A0 — Zero-code smoke test (do first; calibrates A1/A2)

`LoadDirectoryMPQs` (`OpenD2/Engine/FileSystem_MPQ.cpp:132-189`) already scans a
**modpath** directory for *any* `*.mpq` and adds each to the search stack. So:

```
game.exe -w '+basepath="C:/Diablo2/ProjectD2"' '+modpath="C:/Diablo2/ProjectD2"'
```

loads PD2's `pd2data/pd2assets/pd2maps/patch_d2` with **no recompile**. Run it,
watch `debug.log` for which files/tables fail to resolve, and screenshot how far
it boots. This converts A2's scope from estimate to a concrete failure checklist.
(Use the OpenD2 `run` + `screenshot` skills.)

**Caveat that motivates A1:** directory-scan order is filesystem order, so MPQ
priority isn't guaranteed. It'll boot but may pick the wrong copy of a file that
exists in multiple archives (see A1).

**A0 results (run 2026-07-04):** worse than "picks the wrong copy" — **it never
boots past a black screen.** Launched with `+logflags=31` (all priorities) to
capture as much as possible. Observed:

- The window opens and renders (title bar present, process responsive), but the
  client area is solid black with only the hardcoded fallback string
  `Diablo II 1.10f` in the corner — no trademark logo, no menu. Three separate
  click attempts at the trademark-screen click point, spread over several
  seconds, produced **zero change** across three screenshots. This is a stall,
  not a slow load.
- `debug.log`/`debug_engine.log` show only the startup banner and
  `[GENERAL][INFO] OpenD2 client initializing` — nothing further. The client
  never got far enough to reach `DataTables_Init` (Blocker 2 is real but
  **unreachable from this state** — the black-screen stall happens first).
- Confirms Blocker 1 is a **hard boot-blocker**, not a partial-content gap: with
  none of the hardcoded vanilla names resolvable, essentially nothing loads
  (not even the splash screen), so the client can't advance at all.

**Bonus finding — a real, independent logging-order bug:** `FSMPQ::Init()`'s
`Log::Print` calls (which would show exactly which archives opened/failed) run
during `FS::Init()` (`OpenD2/Engine/diablo2.cpp:460`), but `Log::InitSystem()` —
which sets the log-flags gate from config — doesn't run until line 461, *after*.
So those messages are **structurally unloggable today regardless of
`+logflags`** — passing `31` (all priorities) made no difference. Fix: swap the
call order (`Log::InitSystem` before `FS::Init`) so archive-open results are
visible during A1 verification instead of having to infer them from a black
screen. Small, safe, does this session alongside A1.

**Testing gotcha found along the way:** a stale `D2.ini` in
`Documents\My Games\Diablo II\` (`BASEPATH=c:/Users/benam/source/cpp/OpenD2`,
left over from an old test run) is read by `ReadGameConfig()` — which runs
*after* `FS::Init()` already opened archives, so it didn't affect this
particular run's archive search, but `INI::ReadConfig` unconditionally
overwrites fields present in the file. Delete or ignore this file before future
test runs to avoid it silently confusing basepath-dependent code that runs later
in the init sequence.

### A1 — PD2 archive set with correct priority (small code)

Edit `OpenD2/Engine/FileSystem_MPQ.cpp` `Init()` to register PD2's archives
explicitly so **basepath** works (not just the modpath workaround) and priority
is deterministic. Keep the vanilla names too, so both installs remain testable
(a missing archive already fails gracefully — `AddSearchPath` returns null and
`Init` ignores it).

**Priority is load-bearing.** The search path is a stack: *last added = searched
first*. PD2 stores overrides in `patch_d2.mpq` that must beat `pd2data.mpq` —
e.g. `difficultylevels.txt` exists in **both** with different `StaticFieldMin`,
and the game reads patch_d2's copy (proven in `DATATABLE_CONFORMANCE_PLAN.md`,
the DifficultyLevels finding). So add in this order (base first, patch last so it
wins):

```
pd2data.mpq → pd2assets.mpq → pd2maps.mpq → patch_d2.mpq
```

Optional polish: auto-detect which set is present (vanilla vs PD2) by probing for
`pd2data.mpq`, so basepath alone "just works" for either target without a flag.

**Bundled into this slice:** the logging-order fix from A0's findings above
(`Log::InitSystem` before `FS::Init` in `diablo2.cpp`) — small, and makes this
slice's own verification legible (archive open/fail results show in the log
instead of being inferred from screen behavior).

**A1 landed and verified (2026-07-05).** Implementation notes and results:

- `FileSystem_MPQ.cpp` `Init()`: added `pd2data.mpq` → `pd2assets.mpq` →
  `pd2maps.mpq` right before the existing `patch_d2.mpq` registration (so
  patch_d2 stays highest-priority of the base set, per the ordering above,
  without a second registration of the same name).
- **Fixed a real, independent bug found while implementing this:**
  `AddSearchPath`'s failure check was `if (pNew->pArchive == nullptr)` — dead
  code, since that pointer is malloc'd successfully just above and
  `MPQ::OpenMPQ` (which returns `void`) never nulls it. The actual success
  signal is `pMPQ->bOpen` (set only on full success in `MPQ::OpenMPQ`). Fixed
  to check `bOpen` and added a `Log::Print` line per archive (opened/failed).
  Not a crash risk — `FetchHandle` already guards on `bOpen` before use, so an
  unopened archive was silently harmless in the search stack — but it meant
  archive open failures were never visible or cleaned up. Now they are.
- **Logging-order fix implemented as a split, not a reorder:** `Log::InitSystem`
  needs `FS::Init`'s home/base/mod path setup to know where to write the log
  file, so a blind reorder would have broken log-file creation. Instead,
  `FSMPQ::Init()` (the archive-loading call) was pulled out of `FS::Init()` into
  a new `FS::InitMPQ()`, called from `diablo2.cpp` *after* `Log::InitSystem()`.
  Verified: the homepath dated log (`Documents\My Games\Diablo II\D2YYMMDD.txt`)
  went from 0 bytes to a full archive-by-archive open/fail report.
- **Verification (rebuilt all 8 targets, d2conform 81/81, then live-ran against
  ProjectD2):** the log confirmed the fix works exactly as designed —
  ```
  FSMPQ: Failed to open 'd2data.mpq' (D2DATA)          [... 9 more vanilla misses ...]
  FSMPQ: Opened 'pd2data.mpq' (PD2DATA)
  FSMPQ: Opened 'pd2assets.mpq' (PD2ASSETS)
  FSMPQ: Opened 'pd2maps.mpq' (PD2MAPS)
  FSMPQ: Opened 'patch_d2.mpq' (PATCH_D2)
  FSMPQ: Skipping 'patch_d2.mpq' (already loaded as base)   [priority confirmed]
  FSMPQ: Skipping 'pd2assets.mpq' (already loaded as base)
  FSMPQ: Skipping 'pd2data.mpq' (already loaded as base)
  FSMPQ: Skipping 'pd2maps.mpq' (already loaded as base)
  ```
  All 10 vanilla names correctly miss; all 4 PD2 archives open via the
  hardcoded base list (not just the A0 modpath workaround); the later
  modpath directory-scan correctly deduplicates them instead of double-loading.
  Renderer and audio subsystems then initialized successfully against
  ProjectD2 (`Renderer_Allegro initialized (basePath: C:/Diablo2/ProjectD2/)`).

**Blocker 3 — missing font DC6s — ROOT-CAUSED AND FIXED (2026-07-05).**

*Investigation.* With A1 landed the boot got much further than A0's flat black
screen, then hit a stall in `DC6::LoadImage` (`Log_WarnAssert`, non-fatal —
logs + a blocking Windows `MessageBox`, then continues). Probing PD2's
archives directly with `mpyq` (Python MPQ reader; hash-table lookups work even
though its listfile decryption doesn't) confirmed this is a **genuine PD2-S12
content gap, not a path/case bug**: of the six fonts OpenD2 eager-loads at
client init (`fontExocet10`, `font16`, `font30`, `font42`, `fontFormal12`,
`fontRidiculous`), only **`font16.dc6`** exists anywhere in PD2's archive set
(`pd2assets.mpq`). The other five are absent from all of `pd2data.mpq`,
`pd2assets.mpq`, `pd2maps.mpq`, `patch_d2.mpq`, and the modpath-scanned extras
(`d2gl`/`D2MultiRes`/`SGD2FreeRes`/`ver-IX86-1.mpq`). Cross-checked against
vanilla `d2data.mpq`, which does contain all six (confirming the paths/names
are correct standard D2 assets — PD2 just doesn't ship the larger sizes).

*Why "faithful" isn't just a fallback.* Per the project's own principle of
matching PD2-S12's real behavior, the real `D2Win.dll` (D2's UI/font module —
not `D2Client.dll`, which has no font-related strings or named functions at
all) was decompiled via Ghidra (`diablo2` project,
`/Mods/PD2-S12/D2Win.dll`) to see how the actual game avoids this:

- `D2Win_InitializeFontSystem` (0x6f8f3110) eager-loads only **one** always-
  needed font resource at boot (the plate comment identifies it as a
  MonsterIndicators/glyph-lookup resource, not any of the six named sizes
  above).
- `D2WIN_SelectFont(nFontIndex)` (0x6f8f2fe0) is a **lazy, cache-on-first-use**
  dispatcher: it checks a font-cache table entry, and only calls
  `BuildFontDataPath` + `LoadArchiveFile` + `D2WIN_LoadFontFromArchive`
  (0x6f8f2b40) the *first time* a given font index is actually requested by a
  screen being drawn, then caches it (with an LRU eviction after ~240s idle).

So the real client **never eager-loads every font size at startup** — it only
loads whichever sizes the screens the player actually visits need, when they
first need them. OpenD2's `D2Client_InitializeClient` diverged by loading all
six unconditionally at client init (`D2Client.cpp:39-48`), which is exactly
why it hit PD2's missing-file gap immediately, before ever reaching a menu.

*The fix (`Modcode/Client/D2Client.cpp`/`.hpp`):* matches the real
architecture — kept `font16` eager (confirmed present, used almost
everywhere), converted the other five to lazy cache-on-first-access getters
(`D2Client_GetFont30()`, `GetFont42()`, `GetFontFormal12()`,
`GetFontExocet10()`, `GetFontRidiculous()`), each checking the file's
existence via `engine->FS_Open` before calling `LoadFont` and falling back to
`font16` (confirmed present) if the specific size's file doesn't exist —
avoiding `DC6::LoadImage`'s assert path entirely rather than fixing it after
the fact. Updated all 16 call sites across 11 files (CharCreate, Cinematics,
TCPIP, Trademark, CharacterScreen, Inventory, QuestLog, SkillTree, TCPIPJoin,
Button, TextEntry) from direct `cl.fontXxx` field access to the getters.

*Verification:* full rebuild (8/8 targets), `d2conform` 81/81, then a live run
against `C:\Diablo2\ProjectD2` — zero font-related assertions or log lines
fired this time (previously: repeated `Assertion Failure ... DC6.cpp` +
blocking MessageBoxes). Confirms the fix eliminates the specific stall.

**Blocker 4 — extensively investigated (2026-07-05); several real, verified
bugs found and fixed along the way, but the black screen itself is now known
to be a separate, pre-existing, basepath-independent issue, not yet fixed.**

*The investigation trail (each step independently verified correct):*

1. **Testing-environment gotcha, worth flagging first:** a separate,
   already-running `Game.exe` (the real PD2 client, a leftover from earlier
   ghidra-mcp live-debugging work) has the *same process name* as OpenD2's
   build output, so `Get-Process game` in PowerShell silently matches both.
   Always target an explicit PID (`Get-Process -Id <pid>`) once a launch is
   confirmed.
2. **Log-file confusion, also worth flagging:** `engine->Print` (the engine's
   general logger) and `D2LOG`/`D2Log::Write` (the client's category logger)
   are two *different* streams — the former lands in the homepath's dated
   `D2YYMMDD.txt`, the latter in `debug.log`. Several diagnostics below looked
   like they "never fired" simply because the wrong file was checked. Also:
   both streams use plain buffered `fwrite` with no `fflush` — a `taskkill /F`
   loses unflushed output; always close the window gracefully
   (`PostMessage(hwnd, WM_CLOSE, 0, 0)`) before reading a log from a test run.
3. **Root cause of the font-adjacent stall:** PD2-S12 ships `pal.pl2` for
   every palette but **no separate `pal.dat`** (confirmed via `mpyq` probing
   — `ACT1\pal.dat` and `SKY\pal.dat` are absent from all 4 core archives,
   `ACT1\pal.dat.pl2` and `SKY\pal.pl2` are present). `Palette.cpp`'s
   `RegisterPalette` required both files and failed outright without the
   `.dat`. Decompiling the real `D2Win.dll` (Ghidra) confirmed the actual
   client already works this way: `LoadPalette`/`D2WIN_LoadActPalette` call
   `WIN_InitPaletteFromArchive` with **only** the `.pl2` path. **Fixed:**
   `RegisterPalette` now derives the base 256-color table from `.pl2`'s
   embedded `pRGBAPalette[256]` (BGR-per-entry, same layout as `pal.dat`,
   confirmed against the known-correct `dc6_to_png.exe`/`DCC_DecodeCallback`
   conversion) when `.dat` is absent. Also fixed `Pal::Init()`, which bailed
   entirely on the *first* palette that failed (`ENDGAME`, before ever
   reaching `SKY`) — it now tries every palette rather than stopping early,
   same "one missing resource shouldn't gate the whole subsystem" lesson as
   the font fix.
4. **Real architecture gap found via user steer, not a workaround:** OpenD2's
   `AttachCompositeTextureResource`/`AttachTextureResource` never decode DC6
   pixels at runtime — they resolve a path to a **pre-converted PNG file on
   disk** (`ResolvePNGPath` + `al_load_bitmap`) that only exists because
   OpenD2's own bundled `data/` folder happens to ship both the `.dc6` and a
   pre-made `<name>/N.png` side by side. Pointed at a fresh install like
   ProjectD2, no such PNG cache exists and nothing draws. Per direction from
   the user (asked to weigh building a runtime decoder vs. bulk pre-converting
   PD2's assets vs. deferring — see the conversation, not reproduced here):
   implemented on-demand decode. `Renderer_Allegro::LoadOrGetBitmap` now falls
   back to a new `DecodeDC6FrameToBitmap` (reuses `DC6::LoadImage` +
   `GetPixelsAtFrame`/`PollFrame`, same BGR palette conversion as
   `dc6_to_png.exe`) when the PNG isn't found, and **writes the result back to
   disk** (`al_save_bitmap` + a `MakeDirsForFile` helper mirroring
   `dc6_to_png.cpp`'s `MakeDirs`) so subsequent loads hit the fast path.
   Verified independently: the generated `trademarkscreenEXP/1.png` shows
   legible "PROJECT / SEASON 12 / SUFFERING" text; a direct dump of the
   composite bitmap right after assembly (before it's ever drawn to screen)
   was pixel-correct.
5. **A real, previously-invisible bug found in `DC6::LoadImage`:** on `FS::Open`
   failure it logged a warning but **continued anyway**, parsing whatever was
   left in `gpReadBuffer` — a `static` buffer never cleared between calls —
   as if it were the requested file's header/frames. Since Trademark's four
   logo/flame DC6s (`D2LogoFireLeft/Right.dc6`, `D2LogoBlackLeft/Right.dc6`)
   are *also* absent from every PD2 archive, this could produce an
   arbitrary garbage image reusing a previous successful decode's leftover
   data. **Fixed:** bail out cleanly on open failure, leaving the image
   zeroed so callers' existing bounds checks correctly treat it as "no data."
6. **A real first-frame timing bug found:** `cl.dwMS` was left at its `memset`
   default of 0 until the first `D2Client_RunClientFrame()`, so the very first
   `deltaMs` computation reflected the *entire* elapsed startup time (asset
   loading, the new on-demand PNG conversion, etc.) instead of one frame's
   worth. Large enough, this could exhaust a menu's own display timer (e.g.
   Trademark's 10s) before it ever drew once. **Fixed:** `cl.dwMS` is now
   initialized to "now" alongside `cl.dwStartMS`.
7. **After all six fixes above, the screen was still black** — but a targeted
   `Draw()`-call diagnostic (once pointed at the *correct* log file per #2)
   confirmed `Trademark::Draw()` *does* fire, every frame, drawing a
   *proven-correct* composite bitmap. So every layer up to and including the
   draw call is now independently verified correct.
8. **The decisive test: vanilla (no PD2, default basepath) was retested and
   is ALSO a solid black screen** — including at the already-merged commit
   from *before* any of today's uncommitted work (confirmed by `git stash`
   and rebuilding). **This means the black screen is not a PD2 content gap,
   not caused by anything in this session, and was never actually fixed by
   items 3–6 above** (each of those is still a real, independently-verified
   bug worth having fixed) — it's a pre-existing, basepath-independent
   rendering/presentation issue that has apparently been present all along.
   The earlier "vanilla reaches in-game" evidence from this project's history
   was **log-based** (debug.log showing DCC/TOKEN decode messages), never an
   actual screenshot — so it's plausible this presentation-layer gap was
   simply never visually noticed before.

**RESOLVED (2026-07-05, follow-up session): the "needs a graphics debugger"
conclusion above was WRONG — the pipeline was fine all along.** A primitive
test (magenta filled rect + synthetic green bitmap drawn directly in
`Present()`) proved backbuffer/flip/textured-draws all work; draw-time
instrumentation showed the composite draw had perfect state (valid video
bitmap, white tint, identity transform, target==backbuffer). The actual
root-cause chain, three interlocking pieces:

1. **The disk PNG cache was poisoned by an all-zero palette.** PD2 ships NO
   `SKY` palette (the trademark screen's choice). After the try-all
   `Pal::Init` fix, `gbPalettesInitialized` became true, so `GetPalette(SKY)`
   returned a pointer to SKY's never-filled **all-zero** table instead of
   null → the on-demand decoder mapped every pixel to opaque black → those
   black frames were **written to the disk PNG cache** → every later run
   loaded them verbatim and never re-decoded. Everything downstream was
   "correct" — faithfully drawing black-on-black. Pixel-level analysis of a
   cached PNG (91% opaque black + 8% transparent, exactly 2 colors) vs the
   repo's known-good copy (rich grayscale) confirmed it. An image viewer
   rendering transparency as white had earlier masked this ("white text"
   was alpha=0 holes). Fixes: per-palette `bRegistered` tracking;
   `GetPalette` falls back to the first registered palette instead of
   handing out zeros; the renderer only persists decoded PNGs when the
   *requested* palette genuinely registered (`Pal::IsRegistered`), so a
   fallback decode can never poison the disk cache; poisoned dirs deleted.
2. **The pl2-derived palette had red/blue swapped.** `pl2.pRGBAPalette`
   stores R,G,B(,pad); `pal.dat`/`D2Palette` consumers use B,G,R. Verified
   against ACT1's known-good loose `pal.dat`+`Pal.PL2` pair: 256/256 entries
   match byte-reversed (only the 43 grayscale entries match as-is). The
   user's eye caught this ("might be the wrong palette") when the first
   working render showed blue window-glow that should be crimson.
3. **The earlier "vanilla is black at the old commit too" bisect was
   confounded by `D2.ini`.** `WriteGameConfig` re-creates `D2.ini` in the
   homepath on every graceful exit with the *current* basepath; the renderer
   reads `szBasePath` AFTER `ReadGameConfig` applies the INI. Historical
   runs worked because the old INI pointed at the repo root (where bundled
   PNGs live); deleting it (and subsequent test runs rewriting it with
   exe-dir/other paths) made even "vanilla" runs black for cache-resolution
   reasons, faking a pre-existing regression. There was never an Allegro5
   presentation bug.

**Verified working:** PD2-S12's real trademark screen (SEASON 12 SUFFERING
cathedral, crimson glow) renders from `C:\Diablo2\ProjectD2` data, confirmed
by screenshot and by the user.

**THE UNIFYING DISCOVERY (2026-07-05, same session): PD2 is an OVERLAY
install, and every "PD2 content gap" above shared this one root cause.**
The frontend button DC6s, logo flame/black animations, font30/42/
fontFormal12, and the SKY palette are all absent from PD2's archives because
PD2's archives only carry *changed/new* files — the vanilla base comes from
the PARENT directory's LOD install (`C:\Diablo2\ProjectD2` sits inside
`C:\Diablo2`, which has all the vanilla MPQs). The real client works exactly
this way: this repo's own `D2WIN_OpenMpqArchives` trace shows every core
archive opened with a literal `..\` prefix (`..\d2data.mpq` etc.). OpenD2
simply never loaded the parent. Fixed in `FileSystem_MPQ.cpp` by registering
the same parent-relative vanilla set before the PD2 archives (mod files keep
priority; plain-vanilla installs fail the parent opens gracefully).
**Verified: the full main menu — stone button plates, DEBUG MENU / SINGLE
PLAYER / BATTLE.NET / OTHER MULTIPLAYER — renders over PD2's Season 12
background.** This also retroactively softens Blocker 3: the "missing" fonts
exist in the parent `d2data.mpq`, so the lazy font getters now find the real
font30/42/fontFormal12 instead of falling back to font16. The font fallback
and palette fallback remain correct defense-in-depth for genuinely
standalone asset sets.

**Palette-selection faithfulness note (from D2Win.dll decompiles, not
guessed):** the real client's frontend hardcodes ACT1's palette
(`LoadPalette` default path = `palette\act1\pal.dat`/`.pl2`), and in-game
levels select palettes via **Levels.txt's `Pal` column** →
`D2WIN_LoadActPalette(n)` (`palette\act%d\`). PD2 shipping only act palettes
corroborates this. So the ACT1 fallback reproduces real-client behavior
exactly for the frontend; a future cleanup should make OpenD2's menus select
ACT1 explicitly rather than legacy `PAL_SKY`, and in-game palette choice
should flow from the already-parsed `D2LevelsTxt::nPal`.

### A2 — Wire the remaining data tables through TXT_ParseFile (the bulk)

For each table still on the BIN-only path, add a `TxtColumnDef[]` and call
`TXT_ParseFile` — exactly the pattern already working for Levels/LvlTypes/
LvlPrest in `DataTables.cpp:250-288`. The parser maps columns **by header name**
(`TxtParser.cpp:13-24 FindColumnDef`), so PD2's extra/reordered columns are
ignored automatically — you only define the fields OpenD2 actually reads.

Tables to convert (all currently `DataTables_Load(...)` in
`DataTables.cpp:277-343`):

| Group | Tables | Struct |
|---|---|---|
| Levels (remainder) | lvlsub, lvlwarp, lvlmaze | D2LvlSubTxt / D2LvlWarpTxt / D2LvlMazeTxt |
| Items | weapons, armor, misc | D2ItemsTxt (shared) |
| Item meta | itemstatcost, itemtypes | D2ItemStatCostTxt / D2ItemTypesTxt |
| Uniques/Sets | uniqueitems, setitems, sets | D2UniqueItemsTxt / D2SetItemsTxt / D2SetsTxt |
| Skills | skills, skilldesc | D2SkillsTxt / D2SkillDescTxt |
| Char | charstats | D2CharStatsTxt |
| Props | properties | D2PropertiesTxt |

Per table: (1) confirm the struct fields OpenD2 consumes (grep usages of the
`sgptDataTables->pXxx` pointer), (2) write the column def mapping those fields to
PD2's `.txt` headers, (3) swap the `DataTables_Load` call for `TXT_ParseFile`
with a `DataTables_Load` `.bin` fallback if desired. Keep the `.bin` path as the
fallback so vanilla-with-bin still works.

Notes / gotchas:
- Excel txt path prefix is `D2DATATABLES_DIR` = `DATA\GLOBAL\EXCEL\`
  (`D2Common.hpp:8`). PD2 stores its `.txt` there inside `pd2data.mpq`.
- `TXT_ParseFile` reads through the MPQ FS (`engine->FS_Open`), so once A1 opens
  PD2's archives, table files resolve automatically.
- Column-name tolerance means PD2's *added* columns are free; only *renamed*
  columns for fields OpenD2 needs would require attention (unlikely for the core
  1.13c columns OpenD2 reads).

### Phase A gate

Boot → main menu → load/create a character → enter Blood Moor → move & fight,
all against PD2 assets + data. Verify by driving the app (run skill) and
screenshotting, and by confirming `debug.log` shows non-zero record counts for
the converted tables. Fix load failures A0 surfaced.

**Branch/workflow:** do this on `feature/pd2-data-support` in the OpenD2 repo,
per the per-slice branch + `merge --no-ff` workflow.

---

## Phase B — Faithful to PD2, not just running on it (weeks)

Running on PD2 data ≠ behaving like PD2. This is the existing
`EMULATION_CONFORMANCE_PLAN.md` (OpenD2 `docs/`) + this repo's conformance
factory. State: RNG (Layer 0) and melee to-hit are proven; still open —
- wire the proven RNG / to-hit through the rest of combat resolution,
- port + prove damage / AR / defense,
- finish the treasure-class drop golden vector (EMULATION_CONFORMANCE_PLAN §9.3
  is still a stub — capture the output items),
- DRLG generation cell-for-cell.

D2MOO stays the drop-in oracle here: mint PD2-S12 vectors, prove OpenD2's port
(and/or D2MOO's function) matches. Sequence after A so there's real data to prove
against.

---

## Phase C — Playability completeness (OpenD2 `docs/IMPLEMENTATION_PLAN.md` S12+)

HUD / life-mana orbs (S12 — the belt panel is the start), items/loot (S13–16),
DRLG proper (S17–20), Act 1 completion (S21–24). Mostly additive once A/B land.

---

## Sequencing & risks

- **A is the unlock** — B and C test against empty/wrong tables until A2 lands. Do
  A first.
- Presentation (Allegro5 / editor) proceeds in parallel, verified by playing, not
  vectors.
- **Main risk:** PD2 table/struct drift. Mitigated for *booting* by the parser's
  column-name tolerance; caught *semantically* by Phase B conformance.
- Keep vanilla working as a fallback test target throughout (both archive-name
  sets registered; `.bin` fallback retained).

## Quick reference — files to touch (all in OpenD2 repo)

- `Engine/FileSystem_MPQ.cpp` — A1 (archive list + priority); A0 needs no edit.
- `Modcode/Common/DataTables.cpp` — A2 (column defs + TXT_ParseFile calls).
- `Modcode/Common/D2Common.hpp` — struct fields, `D2DATATABLES_DIR`.
- `Modcode/Common/TxtParser.cpp/.hpp` — parser (reference only; already works).
- Verify: OpenD2 `run` + `screenshot` skills; `build_allegro/Debug/debug.log`.
