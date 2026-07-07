# Dynamic profiler plan — instrument every function, count real hits

Goal: turn the live-dispatch cockpit (LIVE_DISPATCH_FRAMEWORK_PLAN.md Phase 4)
into a **whole-engine dynamic profiler**. Hook every function of every game DLL
with a count-only passthrough, play PD2, and read back exactly which functions
(and whole subsystems) the real game executes and how often. Result: an
**objective, data-driven conformance backlog** -- work top-down by real usage
instead of guessing at 1,172+ ordinals blindly.

## Two decoupled tiers (the key insight)

- **Tier 1 -- universal hit counting.** In Original mode a hook is a pure
  passthrough, so a *counting* hook needs NO D2MOO reimplementation -- it just
  increments a counter and jumps to the original. So we can instrument EVERY
  function (exported or internal), regardless of whether D2MOO has reimplemented
  it. This is the "what's actually exercised" profiler.
- **Tier 2 -- conformance dispatchers** (already built): Original/Reimpl/Shadow,
  only for functions D2MOO has reimplemented + address-matched (the 5 coord fns
  today). A small subset of Tier 1.

Tier 1 tells you which Tier 2 work to prioritize: hook everything cheaply, play,
and the millions-of-hits functions are the priorities; zero-hit functions are
dead/inlined/unused and can be deprioritized.

## Mechanism: the universal counting stub

The scaling problem (each function needs a hook of the right calling convention)
is solved for *counting* by a tiny generated machine-code stub that never touches
the arguments, so it is calling-convention-agnostic:

```
inc dword ptr [my_counter]      ; 6 bytes  (per-function counter slot)
jmp dword ptr [my_trampoline]   ; 6 bytes  (Detours trampoline -> real fn)
```

12 bytes/function. Detours redirects the target's prologue to this stub; the stub
counts, then jmps to the trampoline (saved prologue + jmp back), which runs the
real function; its own `ret` returns to the original caller. Correct for
`__stdcall`/`__fastcall`/register-ABI/any arg count, because we only inc+forward.
Overhead = one `inc` per call (negligible even at 15M calls/session). Generate N
stubs in one executable page (~14 KB for all of D2Common). Counter slots live in
a parallel array the UI reads.

(64-bit counters via `add [c],1; adc [c+4],0` if 32-bit wrap becomes a concern in
long sessions; non-atomic is fine for a profiler -- an occasional lost increment
under thread contention doesn't change "hot vs cold".)

## Data source: Ghidra function tables

- **Exports** (~1,300 across the main DLLs): already in `corrected_maps/*.tsv`
  (`ordinal -> address -> ghidra_name`).
- **Internals** (tens of thousands): Ghidra's full function list per binary
  (`list_functions` -- address + name). D2Common alone has ~2,712 functions;
  D2Client ~5,738; D2Game ~5,581; plus D2Win/Storm/Fog/D2Launch/... A tooling
  script dumps each binary's functions to a data file
  (`profiler/<binary>.functions.tsv`: `address  name  category`) that the
  profiler reads at startup. (Exclude ProjectDiablo.dll ~16K and libcrypto ~12K
  -- the mod loader + OpenSSL, not the game engine.)

## Taxonomy (navigation)

- **L1 Binary**: D2Common, D2Game, D2Client, D2Win, Storm, Fog, D2Net, D2Sound,
  D2CMP, D2Launch, D2MCPClient, D2gfx.
- **L2 Subsystem**: the function-name **prefix** (D2MOO/Ghidra already encode it):
  `DRLG_`, `DUNGEON_`, `DATATBLS_`, `ITEMS_`, `UNITS_`, `PATH_`, `COLLISION_`,
  `STATLIST_`, `SKILLS_`, `MONSTER_`, `INVENTORY_`, `QUESTS_`, `SEED_`, ...
  Unnamed internals (`FUN_xxxxxxxx`) bucket into "Unnamed" (or by address range).
- **L3 Function** (leaf): name, ordinal (if exported), address, hits, and -- if it
  also has a Tier-2 dispatcher -- mode + divergence.

UI = a collapsible ImGui tree. Each Binary/Subsystem node shows an **aggregate hit
count** (sum of descendants) so you see "DRLG: 40M hits / 80 fns, ITEMS: 0" at a
glance. Sort by hits, filter by name, dim zero-hit rows. Reset-counters button.

## Home: D2Debugger

D2Debugger already links Detours, has the standalone-window UI + thread + the
cross-DLL bridge. It becomes the profiler: reads the function tables, generates
counting stubs, mass-Detours-attaches by runtime address (module base + (table
addr - preferred base), rebased for ASLR), renders the tree. Tier-2 dispatchers
stay in patch `D2Common.dll`; the profiler READS their counts/mode via the
existing `D2MOO_LiveDispatch_*` bridge and SKIPS hooking those 5 addresses itself
(no double-hook).

## Safety & the incremental rollout (why not 20K hooks on launch)

Each count-only hook is safe in isolation, but 20K hooks against PD2's shimmed
(AcLayers), BH.dll-injected, anti-tamper-ish stack carries real *aggregate* risk:
some prologues are un-hookable (<5 bytes, jump targets, mid-instruction), some
"functions" are thunks sharing code, and one bad hook can crash the game. So:

- **Graceful skip**: every DetourAttach failure is caught and the function marked
  "un-hookable"; never abort the batch. Report hooked/skipped counts per binary.
- **Selective enable from the UI**: counting is enabled per-binary (and later
  per-subsystem), not all-at-once. Start with D2Common exports, confirm stable,
  then expand outward. The tree's Binary/Subsystem nodes get an "instrument"
  checkbox.
- **Batch installs** off the UI thread; suspend nothing the game needs.

## Live findings (2026-07-06) -- Phase 1 built + proven against real PD2

Built the counting-stub mechanism + tree UI in D2Debugger, driven off
`profiler/D2Common.functions.tsv`. Live results:

- **Works: 45 subsystems instrumented, real hit data.** e.g. `SKILLS_GetActive
  SkillAnimData` @10828 = **210 M hits**, whole subsystems ranked by real usage.
  Confirms the whole approach -- objective hot-path data from live PD2.
- **Crash #1 -- mass install (all 1,172 at once) crashes PD2.** Two root causes,
  both now fixed:
  - **Aliased ordinals**: D2Common has multiple ordinals -> the SAME address
    (documented in D2.Detours). Hooking an address twice double-patches the
    prologue -> crash. **Fix:** dedup by address (`g_hookedAddrs`).
  - **DATA exports**: the export table includes non-code exports -- notably
    `g_pDataTables` @11170 (the global data-tables root @ 0x6fde9e1c). Detours-
    "hooking" a data address patches its bytes with a jump, corrupting the
    pointer; the game dereferences garbage on next access -> whole-process
    crash. This one crashed at CALL time, AFTER a clean install, which is why it
    took a per-function flushed log to pin down. **Fix:** an executable-memory
    gate (`IsExecutableAddr` via `VirtualQuery`) skips every non-code export.
- **Safe install model:** per-subsystem, one transaction each, with dedup +
  data-gate + graceful-skip. Small blast radius; a bad subsystem isolates
  instead of wiping the session. (Startup hooking via D2.Detours -- see
  FUNDOC/unification discussion -- is the eventual home; it's *safer* still, as
  the game isn't executing the targets during the patch.)

## Phases

1. **Foundation (safe slice).** Universal counting-stub mechanism + the tree UI,
   proven on **D2Common exports** (1,172, bounded). Data from `corrected_maps`.
   Skip the 5 dispatcher-owned addresses (read via bridge). Live-test: play, see
   which subsystems light up. This proves the entire mechanism + UI + rebasing +
   graceful-skip on a safe set.
2. **Internals for D2Common.** Add Ghidra's full D2Common function list
   (~2,712). Categorize; enable per-subsystem. Measures the "which internal
   functions are hot" signal the exports can't (inlined-past-the-export cases).
3. **All game binaries.** Extend the Ghidra dump + hooking to D2Game, D2Client,
   D2Win, Storm, Fog, etc. Per-binary enable. This is the full tens-of-thousands.
4. **Prioritization output.** Export a ranked "conformance backlog": functions by
   hit count, joined with proof status (from registry.json) -- the objective
   focus list. Feed the hottest un-proven functions into the Tier-2 pipeline.

## What it gets you

A 20-minute play session yields an objective heat-map of the entire engine: which
subsystems and functions PD2 actually runs, ranked. Data-driven conformance --
prove the hot path first, ignore the dead code. And it directly answers the
question that started this ("which coord fns get hits, how do I trigger them"):
the counters make it observable for every function at once.
