# Live dispatch & shadow-verification framework plan

Goal: hook each conformance-relevant function **once, permanently**, then route each
call at runtime to one of three modes — **Original**, **Reimpl** (D2MOO), or
**Shadow** (run both, compare, use Original's result) — toggled per-function from a
UI, with zero live code-editing on toggle. This is simultaneously (a) the strongest
verifier we have (shadow soak-test on real gameplay) and (b) the seed of a mod
interface (a Reimpl allowed to intentionally diverge).

Builds directly on what exists:
- **D2.Detours** already does prologue hooking via Microsoft Detours
  (`DetourAttach`, `external/D2.Detours/source/src/DetoursPatch.cpp:113/120`) and
  hands back a **trampoline** pointer (`realPatchedFunctionPtr`) that calls the
  original cleanly. We do NOT hand-roll `E9 rel32` / prologue splicing — Detours
  owns that, thread-safely, inside its transaction.
- **`PatchAction`** enum (`DetoursPatch.h:5`) is where a new mode slots in.
- **`corrected_maps/*.tsv` + `pd2s12_*_ordinals.tsv`** are the verified
  `ordinal → address` map — the registry keys off these, NEVER the scrambled
  `.def` ordinals (see `ORDINAL_RECONCILIATION.md`).
- **D2Debugger** (ImGui, `source/D2Debugger/`) is the UI host.
- **Conformance harnesses** (`d2moo_conform.cpp`, `PD2S12Conformance.cpp`,
  `behavioral/`) produce the proof records that become registry entries.

---

## Core design decision: hook once, toggle a flag (not install/uninstall)

Naive toggling re-patches bytes on every flip → must suspend all threads each time,
and races a thread mid-prologue. Instead:

**Install one permanent Detours hook per registered function that jumps to a
per-function dispatcher. The dispatcher reads an atomic mode flag and routes.**

Toggling = one atomic write to `mode[fn]`. Instant, thread-safe, reversible
mid-session, no runtime code editing.

```
original prologue --(Detours)--> dispatcher_fn:
    switch (atomic_load(mode[fn])):
      ORIGINAL: tailcall trampoline[fn]        // = real function, ends in its own ret
      REIMPL:   tailcall d2moo_fn[fn]           // our replacement, ends in its own ret
      SHADOW:   snapshot inputs;
                r_orig = call trampoline[fn](inputs);      // game ALWAYS uses this
                r_moo  = call d2moo_fn[fn](snapshot);
                compare(r_orig+writes, r_moo+writes);
                on mismatch: log a self-contained vector;
                return r_orig;
```

Key properties:
- `SHADOW` is just a third route through the same stub — shadow verification and the
  mod toggle are **one mechanism**, not two.
- The game only ever observes Original's result in SHADOW, so a wrong Reimpl can't
  corrupt the session. Reimpl is a passenger until promoted.
- Promotion pipeline is a per-function state machine driven from the UI:
  `ORIGINAL → SHADOW (N hours clean) → REIMPL`.

### Why inline (Detours) and not IAT
Intra-DLL calls (D2Common→D2Common) are direct-address, not through the export
table; IAT patching would miss them. Detours' prologue hook catches **all** callers.
This is also why deep functions can't be reached by export-ordinal swap alone
(the 1172/61/4 export gradient in `ORDINAL_RECONCILIATION.md`).

---

## The dispatcher stub problem (the one real piece of new low-level code)

Each hooked function needs its own dispatcher because Detours points the hook at a
fixed address and the stub must know *which* function's mode/trampoline/reimpl to
use. Two ways to generate N per-function stubs:

- **Option A — templated C++ thunks (preferred, no asm).** A function template
  parameterized by a compile-time registry index generates a distinct dispatcher
  per function; a table of `&Dispatcher<0>, &Dispatcher<1>, ...` gives N unique
  addresses. Works cleanly for the common ABIs (`__stdcall`/`__fastcall`,
  stack/ECX-EDX). Route ORIGINAL/REIMPL as a normal call+return (not a raw
  tailcall) — correct, marginally more overhead, avoids naked-asm entirely.
- **Option B — generated asm trampolines** for Blizzard **register ABIs** (args in
  EAX/ESI/EDI — the same ones `behavioral/pd2_frida_rng_family.js` needed a
  trampoline for). A tiny machine-code stub per function, preserving callee-saved
  regs. Only needed for the register-ABI minority; reuse the exact register
  discipline already proven in the Frida trampolines.

Start with Option A and the coord family (pure `void __stdcall(int*,int*)`) — zero
ABI complications, and it's already proven bit-exact so any divergence is a framework
bug, not a target bug.

---

## Snapshotting & comparison (the correctness crux of SHADOW)

Most D2 functions are **not pure**. SHADOW must:
1. **Deep-copy the read+write set before Original runs**, and feed the *copy* to
   Reimpl — otherwise Reimpl reads state Original already mutated (false agreement)
   or writes over live game state (corruption).
2. **Compare the full write-set**, not just the return value: return reg, every
   output pointer/struct, and any globals the function writes. The RNG family is the
   canonical trap — identical return, diverged `nHighSeed`, bug only visible on the
   *next* call.
3. **Emit divergences as `vectors/*.json` entries** (same schema as
   `conformance/vectors/rng.json`): `{fn, in, out_original, out_reimpl, callstack}`.
   Every live divergence becomes a permanent offline regression vector for free.

Per-function metadata the registry must carry to make this possible: the input
surface (which args + which fields behind pointer args are read) and the write set.
This comes straight from the Ghidra decompilation used to reimplement the function.

Guards:
- **Reentrancy:** per-thread `in_shadow` flag; if a Reimpl internally calls another
  hooked function, suppress nested shadowing.
- **Hot paths:** SHADOW doubles work; make it opt-in per function and support
  sampling (compare every Nth call) for functions called thousands of times/frame.

---

## Registry — the shared artifact of conformance AND the mod interface

One record per hookable function, and it IS both the conformance ledger and the UI's
data source:

```
{ verified_address,           // from corrected_maps/*.tsv — NEVER the .def ordinal
  real_ordinal,               // for display / cross-ref
  d2moo_symbol,               // the reimpl to route to
  abi,                        // stdcall / fastcall / register-ABI (stub selection)
  input_surface, write_set,   // for SHADOW snapshot+compare (from Ghidra decomp)
  proof_status,               // vectors-passed | shadow-clean:<hours> | unproven
  default_mode }              // ORIGINAL until proven
```

Populated from the existing conformance outputs. As each function reaches
bit-identical, its record flips proof_status and becomes eligible for REIMPL default.

---

## Phased implementation

### Phase 0 — spike: does hooking survive PD2? ✅ DONE (2026-07-06) — PASS
PD2 has anti-debug/integrity behavior (the INT3 flood the external debugger hit).
Detour ONE harmless already-proven function (a coord fn), run the game, confirm no
detection/crash and the hook fires. If integrity checks trip, resolve here before
building anything on top. **This gates the whole plan.**

**Result: PASS on both probes** against live PD2 Game.exe (D2Common base
`0x6fd50000`, target `DUNGEON_GameTileToClientCoords` @ `0x6fd9dac0`). Harness:
`behavioral/pd2_phase0_hook_spike.js` + `pd2_phase0_spike.py` +
`run-phase0-spike.ps1` (self-elevating, self-driving from the main menu — no
gameplay needed). Verdict in `behavioral/phase0_verdict.json`.

- **Probe A (`Interceptor.attach`, observe-only prologue splice + trampoline):**
  installed, fired 5/5 on driven coord vectors, outputs bit-exact
  (`(5,3)→(160,320)`, `(-7,2)→(-720,-200)`, …), process alive.
- **Probe B (`Interceptor.replace`, full replacement → trampoline → original,
  the dispatcher REIMPL/SHADOW route):** installed, fired 5/5, outputs bit-exact,
  process alive.
- **Bonus:** `attachHits` climbed 5→10 during the 2 s idle between probes — the
  **game itself** called the hooked function while the hook was live and kept
  running correctly. Tolerance holds for the game's own calls, not just ours.

Frida's `Interceptor` uses the same prologue-splice + trampoline mechanism as
Microsoft Detours (which D2.Detours wraps), so this is direct evidence Detours
will be tolerated too. **The framework is unblocked → proceed to Phase 1.**

(Gotcha fixed mid-spike: Frida's runtime has no `global` object — retain a
`NativeCallback` via module scope / `globalThis`, not `global`.)

### Phase 1 — dispatch layer on ONE proven function (days) — IN PROGRESS (2026-07-06)

**Live native (D2.Detours) test against real PD2, Original mode: PASS, with a
real caveat found.**

Built a hand-written (not yet templated) dispatcher on
`DUNGEON_GameTileToClientCoords` (`D2Common+0x4dac0`), wired via one
`ExtraPatchAction` in `D2.Detours.patches/1.13c/D2Common.patch.cpp` --
`LiveDispatch_GameTileToClientCoords.h`. Mode (Original/Reimpl/Shadow) is an
atomic, toggled for this spike via `D2MOO_DISPATCH_MODE` env var (Phase 4
replaces this with a D2Debugger panel); a throttled heartbeat + any Shadow
divergence write to a fixed log file (`behavioral/phase1_dispatch_log.txt`) so
results are readable without a debugger (Game.exe has no console; the rest of
D2.Detours logs via `OutputDebugStringA/W`, invisible without DebugView).

**Bug found and fixed en route:** the first live attempt showed the hook
never installing at all (`trampoline` stayed null, zero hits, ever) even
though PD2 booted fine. Root cause: the legacy ordinal-sweep API
(`GetPatchAction` over all 1172 `D2Common` ordinals) runs *before*
`ExtraPatchAction`s in `DetoursPatchModule`, and aborts the ENTIRE patch pass
(skipping `ExtraPatchAction`s too) the moment any single one of those 1172
default entries returns `PatchAction_BadInput`/`PatchAction_PatchFailed` --
plausible here since this may be the first time the 1.13c ordinal array +
`D2.DetoursLauncher` combo had been exercised live against real PD2 at all
(prior testing was the standalone loader harness or Frida, never this path).
Fixed by switching registration to `DllPreLoadHook` -- the modern API
(`DetoursPatch.h` itself recommends it over the ordinal-sweep API) -- which
runs unconditionally, first, independent of the legacy array's outcome.

**Confirmed after the fix, live, in a real single-player PD2 session:**
- Hook installs (`trampoline` populated with a real address) and fires on the
  GAME'S OWN call, not just a synthetic one.
- Original mode is a **faithful no-op**: gameplay screenshot shows correct
  isometric rendering (camp scene, NPCs, torches, full HP/mana), no crash, no
  corruption -- indistinguishable from an unpatched game.

**Caveat found (expected, and already named as a risk in this plan):** hits
stayed at exactly 1 despite ~a minute of continuous on-screen tile/NPC
rendering during real play. The heartbeat only fires from inside `Thunk`, so
this is a faithful signal, not a harness bug: **the hot per-frame rendering
path is not calling this exported symbol.** Most likely the compiler inlined
a copy of the same math directly into the renderer, and only a cold/setup-time
caller goes through the address we hooked -- exactly the "inlined originals"
risk called out below. Contrast with the Phase 0 Frida spike, where the same
address DID accumulate hits (5->10) during idle at whatever state the game
was in then; the two observations aren't necessarily in conflict (different
game state/screen can exercise different call sites), but it means **export
hooking this function alone will not intercept every logical tile->client
conversion the renderer performs.**

**Implication for the framework, not just this function:** SHADOW/REIMPL
verification is only as complete as the call sites that actually reach the
hooked export. Before trusting a function's proof-of-conformance as covering
"the game's real behavior," confirm (via Ghidra decompilation of the hot
path, or by watching hit counts during the specific gameplay action that
should trigger it) that the hot path isn't bypassing the export via inlining.
This doesn't block the framework -- Original-mode safety and native-hook
mechanics are both proven -- but it changes what a future REIMPL/SHADOW result
on THIS function would actually certify (setup-time calls only, not
necessarily the per-frame renderer).

**Update (2026-07-06, later same session): Reimpl/Shadow crashed live -- root
cause found and fixed, unrelated to the new dispatcher.**

Live-tested Reimpl mode: game crashed at character load ("Diablo II Error:
Halt, Unrecoverable internal error 6fd95d60"), same exact address+line every
time. Re-tested Shadow mode (Debug, then a full Release rebuild of patch DLL +
D2.Detours.dll + launcher to rule out Debug-CRT/FPU-state theories) --
IDENTICAL crash both times, with `divergences=0` logged before the crash.
Ghidra confirmed `0x6fd95d60` sits inside `ITEMS_CheckItemTypeFilter`
(`0x6fd95d30`-`0x6fd95e49`, `BOOL __stdcall(UnitAny*, ItemTypeFilterEntry*)`).

**Root cause (confirmed, not just hypothesized):** the pre-existing legacy
1172-ordinal `patchActions[]` sweep in `D2Common.patch.cpp` (unrelated to this
session's new dispatcher code) defaults every ordinal to
`FunctionReplacePatchByOriginal` -- believed a safe no-op because it only
patches D2MOO's *own* exports to redirect to the real game's SAME RAW ORDINAL.
But `D2Common.1.13c.def` maps ordinal `@10110` to
`DUNGEON_GameTileToClientCoords`, while PD2-S12's REAL `@10110` is
`ITEMS_CheckItemTypeFilter` -- the exact scrambled-ordinal case
`ORDINAL_RECONCILIATION.md` already documented. So the "safe" sweep silently
patches D2MOO's OWN `DUNGEON_GameTileToClientCoords` symbol to jump into
`ITEMS_CheckItemTypeFilter` -- dormant unless something calls that D2MOO
symbol DIRECTLY (exactly what the Reimpl/Shadow dispatch paths do). When they
did, `ITEMS_CheckItemTypeFilter` dereferenced our `int*` coord pointers as
`UnitAny*`/`ItemTypeFilterEntry*`, PD2 hit an internal consistency check, and
halted. This explains every observation: Original mode never calls D2MOO's own
symbol directly (only routes through the NEW hook's own trampoline into the
REAL function) -> never trips the stale cross-wiring -> safe. Reimpl/Shadow
both call the symbol directly -> both hit it -> identical crash regardless of
Debug/Release. `divergences=0` was very likely a coincidence (the misrouted
BOOL filter-check probably never writes through its pointer args, leaving our
output ints at their seeded input values, which happened to match for that
one load-time call) -- NOT a real conformance result.

**This is the first LIVE reproduction of the exact danger
`ORDINAL_RECONCILIATION.md` predicted in theory** ("A drop-in that patches the
live PD2-S12 game by the `.def`'s ordinals would replace the wrong
functions") -- and it is a bug in the pre-existing legacy patch scheme, not in
this plan's new dispatcher design.

**Fix applied:** `#define DISABLE_ALL_PATCHES` in `D2Common.patch.cpp` (was
commented out) -- turns off the legacy ordinal sweep entirely; the new
`DllPreLoadHook`-based dispatcher doesn't depend on it. Rebuilt clean
(Release). Re-verifying Shadow mode live with this fix is the immediate next
step.

**Generalizable implication:** re-enabling the legacy sweep's default action
for ANY ordinal requires first re-deriving that ordinal's cross-wire target
from the verified `corrected_maps/*.tsv` (ordinal->address, ground truth), not
trusting `.def` name correspondence -- exactly the guidance
`ORDINAL_RECONCILIATION.md` already gives, now with a live incident behind it.

**Confirmed (2026-07-06, same session): clean Shadow-mode pass with the fix.**
Relaunched live PD2 with `DISABLE_ALL_PATCHES` set (Release build). Loaded
into the same single-player character/area as every prior test. Result:
`hits=1, divergences=0`, **no crash** -- screenshot confirms correct
rendering (camp scene, full HP/mana), game fully responsive. This time the
Shadow call actually reached D2MOO's real `DUNGEON_GameTileToClientCoords`
(the cross-wiring bug is gone), so `divergences=0` is now a **trustworthy**
result, not the earlier coincidence.

**Confirmed (2026-07-06, same session): clean Reimpl-mode pass with the fix
-- Phase 1 COMPLETE.** Relaunched live PD2, Reimpl mode, same fixed Release
build. Loaded into the same character/area. Result: `hits=1`, no crash,
screenshot confirms correct rendering (camp scene, full HP/mana, fully
responsive) -- the call actually ran D2MOO's own
`DUNGEON_GameTileToClientCoords` in place of the real function this time, and
the game is indistinguishable from an unpatched run.

**Phase 1 status: all three modes proven live against real PD2-S12, on one
already-proven function:**
- **Original** -- faithful no-op under extended real gameplay (first test,
  pre-fix, unaffected by the ordinal-sweep bug since it never calls D2MOO's
  own symbol directly).
- **Shadow** -- zero divergence, game always safe (never touched by a wrong
  value even during the pre-fix crash investigation).
- **Reimpl** -- full replacement, game runs correctly and indistinguishably
  from vanilla.

Plus a real, generalizable bug found and fixed along the way (the legacy
ordinal-sweep's dormant cross-wiring from `.def` ordinal scrambling) --
independently valuable beyond this spike. **Proceed to Phase 2** (dispatcher
+ divergence-vector-emission generalization) and Phase 3 (registry +
register-ABI functions) per the plan below.

### Phase 1 (original scope, still applicable) — dispatch layer on ONE proven function
- Add `PatchAction::FunctionDispatch` (toggleable) alongside the existing enum
  (`DetoursPatch.h:5`); or an `ExtraPatchAction`-based registration so it's additive
  and doesn't disturb the ordinal-array API.
- Implement the Option-A templated dispatcher + the atomic `mode[]` table +
  trampoline capture (reuse `realPatchedFunctionPtr` from `ApplyPatchAction`).
- Register one coord function; verify all three modes by hand (ORIGINAL == stock
  game; REIMPL == game still correct; SHADOW logs zero divergences).
- Validating against a known-good target proves the *framework*, not the target.

### Phase 2 — SHADOW comparison + vector emission ✅ DONE (2026-07-06)
- Snapshot/compare/log for the coord family (trivial input surface).
- Wire divergence output into the `vectors/*.json` schema; round-trip one synthetic
  divergence through the offline `d2moo_conform` harness to confirm the loop closes.
- Add reentrancy guard + sampling.

**What was built:**
- `LiveDispatch_CoordFamily.h` replaces the Phase 1 single-function spike:
  `D2MOO_DEFINE_COORD_DISPATCH(NS, FN)` macro generates a distinct
  atomic-mode + trampoline + `Thunk` per function (macro instead of a C++
  template -- this project's MSVC config doesn't opt into C++17, and Phase 1
  already hit that wall once with `inline` static members). All 5 coord
  functions now hooked via `DllPreLoadHook` in `D2Common.patch.cpp`, by
  verified offset (`0x4dac0/0x4d8a0/0x4d8c0/0x4db40/0x4db70`), never the
  scrambled `.def` ordinals.
- **Reentrancy guard:** `LiveDispatch::tl_inDispatch`, a single thread-local
  flag shared across all 5 dispatchers -- a nested re-entry into ANY of them
  on the same thread forces Original, never nests a Shadow comparison inside
  another (satisfies the plan's snapshotting-guard requirement).
- **Divergence emission:** Shadow-mode mismatches append a self-contained
  JSON object, in the exact `vectors/*.json` per-entry schema
  (`{"fn","in":{"x","y"},"out":{"x","y"},"reimpl_out":{"x","y"},"note"}`), to
  `behavioral/live_shadow_divergences.jsonl` (newline-delimited so concurrent/
  frequent appends stay trivially safe; join+wrap in `[...]` for a proper
  vectors array).
- **Closing the loop:** `d2moo_conform.cpp` gained coord-family dispatch
  cases (guarded by `D2MOO_CONFORM_LINK_D2COMMON`, defined only by the new
  `d2moo_conform` CMake target in `source/D2Common/tests/CMakeLists.txt`,
  which links the REAL compiled `D2Common` -- mirrors the existing
  `d2moo_fingerprint`/`d2moo_loader_harness` pattern exactly). This calls
  D2MOO's actual shipping functions, not a duplicated formula -- a genuine
  conformance re-check, and the replay target for any real live divergence.
- **`vectors/coord_family.json`** -- all 29 proven cases from
  `PD2S12Conformance.cpp`, now also runnable standalone/offline:
  `d2moo_conform.exe vectors/coord_family.json` → **29/29 passed**.
- **`vectors/coord_family_synthetic_bad.json`** -- one deliberately wrong
  entry (claims `(5,3)→(999,999)`, real value is `(160,320)`), proving the
  harness actually detects and reports a mismatch when one exists:
  `d2moo_conform.exe vectors/coord_family_synthetic_bad.json` →
  `FAIL ... got=(160,320) exp=(999,999)`, exit code 1. **The loop closes**:
  a real live SHADOW divergence, in this exact schema, would be caught this
  same way, entirely offline, no game process needed.
- **Sampling:** not added -- deferred. Hits on this family are extremely rare
  in practice (1 per session in every Phase 1 live test), so SHADOW's 2x
  overhead is a non-issue here; revisit if/when a hot-path function is added.

**Note on runtime deps:** `d2moo_conform.exe` links D2MOO's own `D2Common.dll`
(a real DLL import, not static), so it needs `D2Common.dll` +
`D2CMP.dll`/`Fog.dll`/`Storm.dll`/`D2Lang.dll` copied alongside it
(`build-1.13c/source/D2Common/tests/Release/`) -- otherwise it fails with
`STATUS_DLL_NOT_FOUND` (exit `-1073741515`). Also hit and fixed: MSVC's
`/Wall`-style build here elevates `C4820` (struct padding) to an error on the
tiny JSON reader's `JVal`/`JP` structs -- suppressed via
`#pragma warning(disable: 4820)`, harmless for this leaf tool.

### Phase 3 — registry + generalize (1–2 weeks) — registry loader DONE (2026-07-06)

**Registry loader built:** `tools/build_registry.py` + `registry.json`. A
curated manifest (not an attempt to auto-populate all 1172 D2Common ordinals
-- unrealistic and not what the plan asks for; it says "populated from the
EXISTING conformance outputs", i.e. incrementally) lists the 8 functions
proven so far (5 coord + 3 RNG-family variants), each cross-referenced
against `corrected_maps/D2Common.tsv` (the ground-truth ordinal->address map)
to auto-fill `real_ordinal`/`ghidra_name` and flag any function with NO
corresponding ordinal entry.

Real signal from running it: the coord family all resolved correctly
(e.g. `DUNGEON_GameTileToClientCoords` -> ordinal `10375`,
`TileToScreenCoordsInPlace`, matching `ORDINAL_RECONCILIATION.md` exactly) --
but **all 3 RNG variants have NO entry in `corrected_maps/D2Common.tsv`** at
all. They're reached only by address/trampoline (per
`behavioral/pd2_frida_rng_family.js`), not independently ordinal-exported --
a real, useful registry finding: it means the RNG family can never be
drop-in-wired by ordinal at all (Detours must hook by raw VA, which the live
dispatcher already does correctly by design -- this is confirmation, not a
blocker).

Registry schema exactly matches the plan's: `verified_address, real_ordinal,
d2moo_symbol, abi, input_surface, write_set, proof_status, default_mode` (plus
`ghidra_name`/`def_name_at_this_ordinal` for cross-check display).
`proof_status` distinguishes `live-shadow-clean` (1 function -- the original
Phase 1 target, actually observed live) from `vectors-passed` (7 functions --
proven offline/Frida-oracle only, either never hit during this session's live
play, or not yet live-dispatched at all for the RNG family). `default_mode` is
conservative: only `live-shadow-clean` defaults past Original, and even then
only to Shadow, never Reimpl -- promotion is a decision this registry records,
not makes (Phase 4 UI's job).

**Remaining Phase 3 scope, not yet done:**
- Extend to register-ABI functions (Option B asm trampoline stubs) using the
  RNG family as the first non-trivial target -- registry already has the
  exact per-variant register ABIs recorded (`ecx=pSeed, eax=max` etc., from
  `pd2_frida_rng_family.js`), but no live dispatcher has been built for them
  yet. This is real new low-level work (hand-written trampoline stubs, not
  the templated-thunk path used for the coord family) and would need its own
  live-fire verification pass against PD2 -- a materially bigger, riskier
  undertaking than Phase 1/2's stdcall functions. Scope this with the user
  before starting.
- Expand input-surface/write-set descriptors to struct-taking functions
  (start with the smallest: single `D2UnitStrc*` readers) -- not started.

### Phase 4 — D2Debugger panel (days, parallel with 3) — registry VIEWER done (2026-07-06)

**Built:** `D2Debugger.LiveDispatch.cpp` -- a new ImGui window ("Live Dispatch
Registry") wired into the existing per-frame hook
(`D2Debugger.patch.common.cpp`'s `GAME_UpdateProgress_WithDebugger`, alongside
the existing `D2DebugGame` panel). Renders `conformance/registry.json` as a
table: Symbol, Ordinal, ABI, Proof status (color-coded: green
`live-shadow-clean`, yellow `vectors-passed`), Default mode, with notes on
hover. Reload button re-reads the file. Ships its own tiny JSON reader
(duplicated from `d2moo_conform.cpp`'s shape -- the two live in separate
binaries with no shared utility target). Builds clean.

**Real architectural finding (not yet solved, scoped for later): this panel
is READ-ONLY, and can't simply become live-control-capable by adding
functions.** `D2Debugger` is *build-time* linked against D2MOO's own
`D2Common` (`target_link_libraries(D2Debugger PRIVATE D2Common ...)`), but at
RUNTIME its imports resolve against whichever module is already loaded under
the name `D2Common.dll` in the game process -- which is the REAL game's
`D2Common.dll` (loaded first, by the game's own `LoadLibrary` call), not
D2MOO's separately side-loaded PATCH copy that `D2.Detours` loads via
`TrueLoadLibraryW` into the `patch/` folder identity. This is deliberate
elsewhere in D2Debugger (it wants to read REAL live game state via
`UNITS_GetCoords` etc.) -- but it means the live dispatcher's atomics
(`mode`/`hits`/`divergences`, `static` inside `D2Common.patch.cpp`, private to
OUR patch-copy module) are simply unreachable via normal linking from here.

**What actually toggling live state would require:** an explicit exported API
from the patch DLL (e.g. `D2MOO_LiveDispatch_SetMode`/`GetCounters`) PLUS
discovering that specific module's `HMODULE` by full path (since both copies
share the base name `D2Common.dll` -- `GetModuleHandle("D2Common.dll")` alone
is ambiguous/order-dependent), then calling through `GetProcAddress` instead
of normal static linking. This is real, scoped, buildable follow-up work --
just not attempted this session, since it needs its own live-fire
verification pass (does the cross-DLL discovery actually find the right
module in the real injected process?) and the user explicitly chose Phase 4
this round specifically to avoid new live-testing risk.

**Not yet done:**
- Live mode toggle (dropdown) + live hit/divergence counters in the table --
  blocked on the cross-DLL bridge above.
- Global controls ("shadow all proven-clean", "promote all shadow-clean-for-
  N-hours to reimpl", export accumulated divergence vectors).

**Live visual check attempted (2026-07-06): D2Debugger's ImGui overlay does
not render at all against real PD2 -- a pre-existing gap, NOT caused by this
session's panel.** Staged `D2Debugger.dll` in the patch folder, launched with
both `-debug` (command-line) and `D2_DEBUGGER=1` (env var) -- game booted and
ran normally (character load, full HP/mana, correct rendering), but NEITHER
the new "Live Dispatch Registry" window NOR the pre-existing "Game" debug
window ever appeared. Since both panels share the identical render call path
(`GAME_UpdateProgress_WithDebugger` -> `D2DebuggerNewFrame/EndFrame` wrapping
`D2DebugGame` + `D2DebugLiveDispatch`), and NEITHER shows, the fault is
upstream of this session's code, in D2Debugger's rendering setup itself.

**Root cause (strong circumstantial evidence, not yet confirmed by
decompilation):** `C:\Diablo2\ProjectD2` contains `d2gl.mpq`/`d2gl.json` +
`D2Glide.dll`/`glide3x.dll`/`D2DDraw.dll`/`ddraw.dll` -- strong evidence PD2
renders through a custom OpenGL layer (`d2gl`), not vanilla D3D9. Meanwhile
`D2Debugger.imgui.d3d9.cpp`'s `CreateDeviceD3D` creates an INDEPENDENT D3D9
device against the game's window handle -- it does not hook whatever renderer
the game actually uses. If PD2 draws via OpenGL to that same HWND, D2Debugger's
separate D3D9 `Present()` either fails to attach meaningfully or gets
immediately overdrawn by the game's own OpenGL present each frame -- either
way, invisible. This would mean D2Debugger's ImGui UI has likely never been
live-verified to render against real PD2 at all (as opposed to vanilla D2 with
D3D9), independent of anything built this session.

**RESOLVED (2026-07-06): the registry panel now renders live against PD2** via
a standalone top-most window (see below). The rendering-gap investigation and
its resolution:

**Investigation (via Ghidra MCP + direct binary import analysis).** PD2's real
renderer is **d2gl** (`glide3x.dll`, a 3 MB Glide->OpenGL 4.6 wrapper;
`renderer=opengl`, `gl_ver 4.6`). D2Debugger created its OWN D3D9 device --
which can't share the window with the game's GL renderer. First attempted the
industry-standard fix: **present-hook overlay** -- Detours-hook the game's
present and render Dear ImGui (imgui_impl_opengl3) into the game's own GL
context. Built it fully (`D2Debugger.overlay.gl.cpp`, imgui_impl_opengl3 added
to the imgui target, Detours+opengl32 linked). But live diagnostics proved
**PD2's present is un-hookable via any standard entry point**: hooked
`opengl32!wglSwapBuffers`, `gdi32!SwapBuffers`, `opengl32!wglSwapLayerBuffers`,
AND d2gl's Glide-level `glide3x!grBufferSwap` (all resolved + attached,
`commit=0`) -- and NONE fired while the game was visibly presenting frames.
PD2's stack (d2gl + cnc-ddraw + `SGD2FreeDisplayFix.dll` + the `AcLayers`
app-compat shim) presents through a non-standard path that bypasses all of
them. (Full module dump + per-hook fire logs in
`behavioral/overlay_gl_log.txt`.)

**Resolution: standalone top-most debugger window (renderer-agnostic).**
Pivoted to a SEPARATE window with its OWN D3D9 device, driven by its OWN
thread + message pump (`D2Debugger.imgui.d3d9.cpp`: `StandaloneThread` /
`D2Debugger_StartStandalone`, kicked off from `DllMain`). It never touches the
game's renderer, so it works no matter how PD2 presents. This also fixed the
two reasons the ORIGINAL D2Debugger never showed on PD2, now understood: it
was pumped only from the `GAME_UpdateProgress` hook (dead on PD2 -- scrambled
D2Game ordinal, so `D2DebuggerInit` was never even called) and positioned
BEHIND the borderless-fullscreen game. The standalone thread pumps it
independently and `SetWindowPos(HWND_TOPMOST, ...)` makes it visible.

**Confirmed live (screenshot):** the "D2Debugger" window renders top-left
alongside PD2 in-game, showing the "Live Dispatch Registry" table with all 8
functions from `registry.json` -- correct ordinals (`@10375` etc.), ABIs,
color-coded proof status (green `live-shadow-clean`, yellow `vectors-passed`),
default modes. Game runs normally beside it. **Phase 4 registry viewer: DONE
and visually verified against real PD2.** (The present-hook overlay code is
retained but dormant -- `D2Overlay_GL_StartInstall` is no longer called; kept
as a reference / potential future path if PD2's present is ever pinned down.)

**Live control + counters DONE (2026-07-06): the cross-DLL bridge is built and
verified live.** The patch `D2Common.dll` now exports C bridge functions
(`D2MOO_LiveDispatch_GetCount/GetName/GetMode/SetMode/GetHits/GetDivergences`,
in `LiveDispatch_CoordFamily.h`) that read/write the dispatcher atomics from
inside the patch module. D2Debugger enumerates loaded modules, finds the ONE
exporting them (disambiguating the two same-named `D2Common.dll`s -- the real
PD2 one doesn't have these exports), and calls them. The panel now shows, per
coord function: an editable **Live mode** combo (Original/Reimpl/Shadow) wired
to `SetMode`, and live **Hits**/**Diverg** counters from `GetHits/GetDivergences`
-- plus global **Shadow all** / **Original all** buttons. RNG rows correctly
render as non-live (no dispatcher). Confirmed live against PD2 (screenshot):
"Live bridge: CONNECTED (5 dispatchers)", counters actively climbing.

**Real finding from the live counters:** `DUNGEON_GameSubtileToClientCoords`
(@11087) showed **~15.5 MILLION hits** in a short in-game session, while
`DUNGEON_GameTileToClientCoords` (@10375) showed only ~750. So @11087 is the
actual hot per-frame render path -- this resolves the Phase 1 mystery (why
@10375 only fired once): @10375 is a cold/setup-time call, @11087 is the
renderer's real workhorse. The cockpit now answers "which function is actually
hot / actually exercised" at a glance -- exactly the visibility the plan wanted.

**Phase 4: DONE.** Registry viewer + live per-function mode toggle + live
hit/divergence counters + global promotion controls, all rendering in a
renderer-agnostic standalone window alongside real PD2. Remaining niceties (not
blocking): "promote all shadow-clean-for-N-hours to reimpl" automation, and
exporting accumulated divergence vectors from the UI (the dispatcher already
writes them to `behavioral/live_shadow_divergences.jsonl`).

---

## What this proves (and doesn't)
Bit-identical over hours of real play = very strong evidence of observable
equivalence **on the played input distribution**. It does NOT cover inputs gameplay
never generates (negative coords, max-level edges, adversarial seeds). So keep the
three layers complementary: Ghidra decompile-diff (algorithm-level), hand-minted
vectors (edge/negative), shadow (breadth on real traffic). Together ≈ as close to
proof as we get without formal methods.

## Risks / gotchas (carried from the mechanism analysis)
- **Inlined originals** — if the game inlined a small fn into callers, there's no
  single entry point; "replaced" ≠ "all uses replaced". Confirm a distinct body in
  Ghidra before trusting a replacement.
- **Sub-5-byte prologues** — can't hold a `jmp rel32`; rare, handle specially or
  IAT-patch those.
- **PD2 integrity checks** — Phase 0 gate.
- **Hot-path cost** — sampling + opt-in SHADOW.
- **Resolve by verified address only** — the scrambled-`.def` finding means keying
  the registry off ordinals would hook the wrong function.

## First concrete step
Phase 0 spike on `DUNGEON_GameTileToClientCoords` (proven `@10375` / `0x6fd9dac0`,
`PD2S12Conformance.cpp`): one permanent Detours hook → dispatcher → confirm it fires
and PD2 tolerates it. Everything else is gated on that.
