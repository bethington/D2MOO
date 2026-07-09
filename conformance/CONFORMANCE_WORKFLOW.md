# The conformance workflow — one ordered ladder, end to end

This is the **operational runbook** for taking a Project Diablo 2 function from
"never looked at it" to "bit-exact D2MOO reimpl, gated in CI." It ties together
every stage that lives scattered across `conformance/tools/*.py` and
`fun-doc/*.py` into a single ordered sequence, says which script drives each
stage, what the entry/exit criteria are, and — critically — **what is automated
vs. what needs you** (launching the elevated game, playing, the human ship gate).

For the *meaning* of each tag see [`CONFORMANCE_TAXONOMY.md`](CONFORMANCE_TAXONOMY.md).
For the *why* of the vetting rungs see [`SHIPPING_PROMOTION_PLAN.md`](SHIPPING_PROMOTION_PLAN.md).
This file is the *how, in order*.

---

## The two axes and the ship ladder, in one picture

A function carries at most one rung from each of two **orthogonal** axes, plus a
vetting marker:

```
DOC axis   (understanding):   (none) → DOC_DRAFT → DOC_REVIEWED → DOC_VERIFIED
CONF axis  (equivalence):     (none) → CONF_DRAFT → CONF_VECTORS → CONF_LIVE → CONF_BATTLETESTED → CONF_REGRESSION
vetting:                                                          └→ vetted:adversarial (V1)
```

The **through-line**: nothing is trusted on a model's word. Every rung is a
comparison against the REAL original — static vectors, then the live game, then
the game's own inputs at volume, then a frozen CI replay.

---

## The ordered ladder — stage by stage

Each row: the stage, the rung it earns, the driver, and its automation status.

| # | Stage | Earns | Driver / command | Needs game? | Status |
|---|-------|-------|------------------|:-----------:|--------|
| 0 | **Prioritize** — rank the backlog by real hit counts | (backlog) | `profiler_feed.py` → fun-doc queue | ✅ (to capture counts) | 🔨 NEW (Phase 4) |
| 1 | **Document** — name, prototype, comments, structs in Ghidra | `DOC_*` | `fun_doc.py` (`FUNDOC_DOC_TAGS=1`) | ❌ | ✅ wired |
| 2 | **Port + static vectors** — draft the D2MOO reimpl, prove vs Ghidra-emulated vectors | `CONF_VECTORS` | `port_pipeline.run_harness` (inside `fun_doc.py`) | ❌ | ✅ wired |
| 3 | **Live prove** — call original + reimpl in the running game, compare | `CONF_LIVE` | `port_live_prove.run_live_prove` (`FUNDOC_LIVE_PROVE=1`) | ✅ | ✅ wired |
| 4 | **V1 adversarial re-proof** — independent break-it vectors from the decompile only | `vetted:adversarial` | `adversarial_reproof.py` | ✅ | ⚠️ built, **not auto-chained** |
| 5 | **Stage as shipping shadow dispatcher** — the reimpl now runs INSIDE shipping `D2Common.dll` | (prep for V2) | `shadow_promote` (`FUNDOC_SHADOW_PROMOTE=1`) + deploy | ✅ | ⚠️ Class A/B only |
| 6 | **Battletest** — zero divergence over real gameplay at volume | `CONF_BATTLETESTED` | `battletest_promoter.py --loop` (**you play**) | ✅ | ✅ wired |
| 7 | **Freeze → offline gate** — capture real outputs, emit a doctest into `D2CommonTests` | `CONF_REGRESSION` | `freeze_corpus.py` → `gen_offline_tests.py` → `ctest` | ✅ (to capture) | ⚠️ coord2 ABI only |
| 8 | **Human ship review** — read reimpl vs decompile, coverage ledger | SHIPPING | you | ❌ | manual gate |

Stages 1→4 are the **unattended automatable ceiling** (no gameplay). 5→7 need a
build+deploy and some of your gameplay; 6 and 8 are inherently yours.

---

## What's already wired inside `fun_doc.py`

The port worker (`process_port_candidate` → `run_port_worker_pass`) already chains
1→3→5 behind opt-in env flags, and polls 6:

```sh
# from fun-doc/, with the venv active
export FUNDOC_DOC_TAGS=1        # stage 1: write DOC_* rung in Ghidra from the score
export FUNDOC_LIVE_PROVE=1      # stage 3: live-prove global_leaf/pure fns → CONF_LIVE
export FUNDOC_SHADOW_PROMOTE=1  # stage 5: stage proven fns into shadow_manifest.json
python fun_doc.py               # dashboard; or drive a port batch via the worker
```

`battletest_poll` (stage 6) runs once per continuous-worker iteration, so
already-deployed dispatchers accrue `CONF_BATTLETESTED` hands-off while you play.

**The gaps this workflow closes** (see the orchestrator + new components below):
- Stage 0 (hit-ranked backlog) — the profiler counts real hits live but nothing
  exports them into fun-doc's queue yet.
- Stage 4 (V1) is standalone; it should run automatically after every `CONF_LIVE`.
- Stage 5 shadow generator emits only Class A (int-return) / B (outbuf); the
  register-explicit **Class D** the `DATATBLS_*` family needs isn't generated, so
  those reach `CONF_LIVE`+V1 but can't be shadowed/frozen/shipped yet.
- Stage 7 freeze/emit handles only the coord2 ABI; scalar-in/scalar-ret isn't built.

---

## The orchestrator — one command for the automatable ladder

`conformance/tools/run_conformance_batch.py` drives stages 0→4 (and stages 5/7
where the ABI is supported) for a batch of N functions and prints a per-function
**final-rung report**. It is a thin orchestrator: it shells out to the existing
stage scripts and never reimplements a stage.

```sh
# rank the backlog, take the top 10 un-proven hot functions through the ladder
python conformance/tools/run_conformance_batch.py --top 10

# a specific subsystem, or an explicit list
python conformance/tools/run_conformance_batch.py --subsystem DATATBLS --top 8
python conformance/tools/run_conformance_batch.py --functions FOO,BAR,BAZ

# how far to push (default: as far as the ABI allows, unattended)
#   --stop-at live        stop after CONF_LIVE
#   --stop-at v1          stop after V1 adversarial (the no-gameplay ceiling)
#   --stop-at shipping    also stage+freeze where the ABI is Class A/B/scalar
python conformance/tools/run_conformance_batch.py --top 10 --stop-at v1
```

Output is a table: `name | DOC | CONF | vetted | where it stalled`.

---

## Preconditions & the game-launch handoff

Stages 3, 4, 6, 7 (and hit-count capture in 0) need the **elevated game running**
with the live bridge on `127.0.0.1:8790`. The game runs elevated (one UAC prompt).

Launch the **conformance build** (`build-1.13c`, which carries the shadow
dispatchers + D2Debugger bridge) — NOT `D2MOO-Tool.ps1 -Action Launch`, which
boots `out/build/VS2022` (the wrong tree, no bridge):

```powershell
# elevated; sets the patch dir so the dispatcher D2Common.dll + D2Debugger.dll load
$root = "C:\Users\benam\source\cpp\D2MOO"
$env:DIABLO2_PATCH = "$root\build-1.13c\patch"
$env:D2_DEBUGGER   = "1"
& "$root\build-1.13c\external\D2.Detours\source\Release\D2.DetoursLauncher.exe" `
    "C:\Diablo2\ProjectD2\Game.exe" "--" "-w"
```

After launch, bind the provider and turn on shadow mode:

```
d2dbg_reload_provider          # binds candidates → provider DLL
d2dbg_set_all_modes shadow     # every dispatcher compares original vs reimpl
d2dbg_status                   # confirm bridge up, dispatchers > 0, provider loaded
```

**Deploy gotchas** (learned 2026-07-07, see `graduated-conformance-pipeline` memory):
- The running game holds `patch/D2Common.dll` memory-mapped — a rebuild can't
  overwrite it until the **process fully exits** (`exit_to_menu` alone doesn't
  release it).
- The game is elevated, so a non-elevated `taskkill` is Access-denied; use an
  elevated kill. `exit_to_menu` saves the char first, so the kill is lossless.
- Before running `battletest_promoter.py`, make sure Ghidra's **active program is
  D2Common.dll** (`switch_program D2Common.dll`) — the tag-flip endpoint ignores
  its `program` param and silently no-ops against the wrong active program.

---

## The new components (build order)

Built against this runbook; each is independently testable.

1. **`profiler_feed.py`** (stage 0) — GET `/profiler/subsystems` + `/profiler/functions/<sub>`
   off `:8790`, join with `proven_functions.jsonl` proof status, rank by hits,
   write the top un-proven functions into fun-doc's `priority_queue.json`. Needs a
   ~20-min profiling play session first to populate counters.
2. **V1 auto-chain** — the orchestrator calls `adversarial_reproof.reprove` for
   each row that just reached `CONF_LIVE`. Closes the "run automatically after
   every CONF_LIVE proof" intent.
3. **Scalar freeze** (stage 7) — extend `freeze_corpus.py` to capture scalar-in/
   scalar-ret via the general `/oracle` seam, and add a scalar emitter to
   `gen_offline_tests.py`. Unlocks CONF_REGRESSION for the non-coord functions.
4. **Class D shadow generator** (stage 5) — teach `gen_shadow_dispatch.py` to emit
   a naked register-explicit thunk (the oracle already marshals this via
   `orig_regs`). Unlocks V2/shipping/freeze for the register-explicit `DATATBLS_*`
   majority the automated loop produces.
5. **Auto-stage-to-shipping** — on `vetted:adversarial`, add the reimpl to
   `shadow_manifest.json`, regenerate the dispatcher header, and flag a D2Common
   rebuild+deploy. The shadow dispatcher IS the shipping reimpl (it runs in the
   shipped DLL); freezing its captured vectors gives CONF_REGRESSION.

---

## Honest blockers (don't paper over these)

- **Everything live-gated is down when the game is down.** Stages 3/4/6/7 and hit
  capture return nothing until you launch. The orchestrator degrades gracefully:
  it runs stages 1→2 offline and marks the rest `blocked: oracle unreachable`.
- **Class D not built ⇒ the register-explicit majority stalls at CONF_LIVE+V1.**
  `DATATBLS_*` accessors take their arg in a non-standard register; until the
  Class D thunk lands they cannot be shadowed, battletested, frozen, or shipped.
- **V1 can crash the oracle if it sweeps a dispatch/abort arg out of domain.** The
  generator is safe-by-envelope (see `v1-adversarial-reproof` memory); never
  disable that guard. A crashed `:8790` needs a game relaunch.
- **Auto-merge to shipping removes the human V3 gate for those functions.** Keep
  the coverage-ledger discipline: only functions with V1 + battletest evidence
  across the states that matter for them should auto-stage.

## Hard lessons from the first live batch (2026-07-07)

- **The port auto-selector (`fun_doc.py --port`) does NOT target the hot backlog.**
  `select_port_candidates` only picks functions already Stage-1 documented
  (`score>=80`) and sorts leaf-first by address, so it sweeps low-address
  library/string leaves. To port the hot list, drive `process_port_candidate`
  DIRECTLY by address via `fun-doc/port_targets.py` (`--backlog` / `--names`) --
  which the orchestrator now calls. `profiler_feed.py --pin` feeds the DOC worker's
  queue, a different selector; it does not steer porting.
- **Port live-prove of a raw pointer/buffer leaf can crash the oracle thread.**
  A blind batch hit `STRCHR_FindChar` (a `char*` scanner); the oracle marshalled
  its string arg as a raw int, `strchr` walked unmapped memory, and the fault was
  SEH-uncatchable -> the `:8790` server thread died (game process survived, but no
  revive path -> relaunch). Mitigations in place: `port_targets.py` skips
  `_looks_like_library_or_runtime` names, and the hot backlog is game-logic getters
  (int-index = safe scalar vectors; unit-pointer = classified `stateful` -> skipped,
  no oracle call). Same root cause as the V1 jump-table crash -- see
  `v1-adversarial-reproof` memory. A future oracle hardening should allocate real
  buffers for pointer-typed args instead of passing raw ints.
- **The live counters are volatile.** They live in D2Debugger's memory and are lost
  on any crash/restart. Snapshot them (`profiler_feed.py --dump-all`) before a
  restart -- the hot backlog + heat-map are the objective prioritization signal.
