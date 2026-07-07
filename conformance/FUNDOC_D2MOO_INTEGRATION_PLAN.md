# fun-doc → D2MOO integration: document, port, LIVE-prove

Connect the fun-doc worker system (`C:\Users\benam\source\mcp\ghidra-mcp\fun-doc`)
to D2MOO's live dispatch/shadow framework so that an automated worker can:
**document a PD2 function → draft a one-for-one D2MOO equivalent → prove it
bit-exact against the real running game → record the result** — and the proof
*validates the documentation itself*.

## The key insight: D2MOO's live oracle removes fun-doc's two hard limits

fun-doc already has this pipeline for **OpenD2** (`port_pipeline.py`,
`conformance_workbench.py`), but:

1. **It only handles PURE/LEAF functions.** Its prove-stage oracle is the static
   Ghidra `/emulate_function` (no live process, no real state), so stateful
   functions -- pointer/global-state args -- are classified and **skipped**
   (left to manual OpenD2 skills). That's the majority of interesting game logic.
2. **Its proof is offline emulation**, not the real running game.

D2MOO's **live shadow-divergence** oracle (LIVE_DISPATCH_FRAMEWORK_PLAN.md) is
strictly more powerful for the skipped class: it runs the candidate IN the real
PD2 process, on real arguments and real state, every call, and diffs against the
original. So it can prove **stateful** functions fun-doc currently can't touch.
The two oracles are **complementary**:

| Oracle | Covers | Inputs | Needs |
|---|---|---|---|
| Static `/emulate_function` (fun-doc today) | pure/leaf only | chosen + adversarial/edge | offline, fast |
| **Live shadow (D2MOO, new)** | **any fn incl. stateful** | real gameplay distribution | running game |

A function proven by BOTH is maximally trusted (edge cases + real-usage breadth).
And crucially: **if the equivalent -- written from fun-doc's documentation --
matches bit-exact live, the documentation is verified correct.** A divergence
means the doc or the port is wrong → feedback to the worker. Documentation stops
being "plausible" and becomes "proven against the binary."

## Reuse, don't rebuild — fun-doc already provides

- **Worker dispatch**: priority queue + LLM providers (Claude/Codex/Minimax) +
  SQL state store + web dashboard + event bus. Don't rebuild any of this.
- **Marker convention** (`conformance_workbench.py`): OpenD2 source carries
  `// @PD2S12 <Module>!<Symbol> @ 0x<addr> (conformance: <STATE>)`; Ghidra carries
  the reverse marker + PORTED/PROVEN tags; `build_index()` greps for markers so
  coverage is always in sync with committed code. **Adopt the same convention
  for D2MOO** (a second port target).
- **`port_pipeline.py`** = the Stage-2/3 template. Write a D2MOO sibling that
  targets this repo and swaps the prove-oracle from static-emulate to live-shadow.

## The D2MOO pipeline (worker flow)

Per candidate function:

1. **Prioritize — from REAL hit data.** fun-doc ranks by xref count + completeness
   today. Feed it the **dynamic profiler's** live hit counts
   (DYNAMIC_PROFILER_PLAN.md): document/port the functions PD2 *actually executes
   most*, first. Objective, usage-driven backlog.
2. **Document (Stage 1)** — fun-doc's existing auto-doc worker: name, prototype,
   plate/inline comments, struct fields, in Ghidra. Unchanged.
3. **Port (Stage 2)** — worker drafts the D2MOO C++ equivalent from the documented
   decompilation and places it in the **organized location by subsystem**
   (`source/D2Common/src/<Subsystem>/`, matching the existing DataTbls/ Drlg/
   Path/ Units/ layout; subsystem = the taxonomy prefix). Adds the `@PD2S12`
   marker. Registers it: a dispatcher entry + `registry.json` row + reimpl target.
4. **Prove (Stage 3) — LIVE shadow.** The function gets a dispatcher hook (Tier-2).
   In a play session it runs in Shadow mode against the real game; N calls with
   zero divergence → **SHADOW-CLEAN**. Optionally ALSO run static `/emulate_function`
   for pure functions to add edge-case coverage → **PROVEN** (both oracles agree).
5. **Record** — stamp Ghidra with the PROVEN tag + reverse marker; flip
   `registry.json` proof_status; update fun-doc state. The conformance ledger
   (registry) and the RE database (Ghidra) stay bidirectionally in sync.

## Restart-free testing (ties to the hot-reload design)

For a worker to add + prove equivalents WITHOUT restarting PD2 each time:
- **Reloadable reimpl-provider DLL**: ported equivalents export by name from a
  DLL D2Debugger can FreeLibrary/LoadLibrary live; the dispatcher calls the
  reimpl through a swappable pointer table.
- **Runtime hook install**: the profiler already demonstrated installing Detours
  hooks on a live game. New candidates get hooked on demand.
- So: worker ports → builds provider DLL → D2Debugger "reload" → new function
  appears in the cockpit in Shadow → proven in the SAME session. Batch many
  candidates, prove them all in one play-through.

## Prefix / naming convergence (the organizing force)

Converge functions to `SUBSYSTEM_` names **incrementally, driven by
understanding** -- exactly when the worker documents/ports a function and thus
knows its subsystem (never a blind mass-rename). The profiler's hit data sets the
order (name the hot path first). The subsystem name then determines: the Ghidra
name, the D2MOO source folder, the taxonomy bucket, and the registry category --
one decision, propagated everywhere by the markers.

## Phased build

1. **Marker + index for D2MOO.** Adopt the `@PD2S12` marker in D2MOO source; add a
   `conformance_workbench`-style index over this repo (or extend the existing one
   with a D2MOO target). Manually mark the 5 proven coord fns + RNG as the seed.
2. **D2MOO port pipeline module** (sibling of `port_pipeline.py`): targets this
   repo, generates the equivalent + dispatcher + registry wiring, prove-oracle =
   live-shadow. Start with the pure functions (provable by BOTH oracles) to
   validate the loop end-to-end against the existing static path.
3. **Reloadable reimpl-provider + runtime dispatcher install** (restart-free).
4. **Profiler → fun-doc priority feed**: export the profiler's ranked hit list
   into fun-doc's queue.
5. **Scale**: worker runs the queue, batching live-shadow proofs per play session;
   dashboard shows document/port/prove coverage per binary, now including the
   stateful functions the OpenD2 path couldn't reach.

## What this yields

A closed loop where reverse-engineering documentation is continuously **proven
against the live binary**, an automated worker converts PD2's real hot-path
functions into verified D2MOO equivalents in an organized tree, and every proof
strengthens both the D2MOO conformance ledger and the Ghidra RE database. The
profiler says what matters; fun-doc documents and ports it; the shadow oracle
proves it; Ghidra + registry record it.
