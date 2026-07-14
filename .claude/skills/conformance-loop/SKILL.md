---
name: conformance-loop
description: Resume or run the D2MOO/PD2 conformance proving loop — the parallel-set harness that proves D2Common functions bit-exact against the live game oracle and writes proof-backed Ghidra docs. Use when asked to "resume the conformance loop", "keep proving functions", "run a proving set", "continue the harness", or to hand this work off to a fresh Claude Code instance. Requires the PD2 game running with D2_DEBUGGER=1 (this skill relaunches it if down).
---

# Conformance proving loop (parallel-set harness)

Proves undocumented D2Common.dll functions **functionally equivalent** to the
original game (rung CONF_LIVE+) by drafting a 1:1 C++ reimpl, proving it bit-exact
against the live game via the D2Debugger oracle, and writing proof-backed docs to
Ghidra. Goal: cover **every** in-scope function in the triage pool, not just easy
ones. Failures are the work queue — each is either a script-fix or an
intelligence-escalate.

## Where everything lives (persistent)

- **Harness**: `C:\Users\benam\source\mcp\ghidra-mcp\fun-doc\parallel_harness\`
  - `pset.py` — ONE supervised parallel set: draft N fns in parallel (minimax),
    batch-build the provider ONCE, prove each serially, emit `pset_report.json`.
  - `batch_builder.py` — selects the next batch (retry_queue → getter_queue →
    cursor), with library/import-thunk filtering + candidate-dir hygiene.
  - `loop_state.json` — cursor, attempted set, buckets (blocked-reason tallies),
    getter_queue (pre-scanned getter candidates), retry_queue, journal.
  - `LOOP_PLAYBOOK.md` — the full operating procedure + all learned constraints.
- **Pipeline**: `fun-doc/fun_doc.py`, `port_pipeline.py`, `port_live_prove.py`,
  `abi_static.py`, `shadow_promote.py` (all the classification/ABI/prove/guard
  capabilities). Run with the venv: `fun-doc\.venv\Scripts\python.exe`.
- **Proofs (source of truth)**: `D2MOO\conformance\proven_functions.jsonl`
  (match by ADDRESS — the name-audit renames functions).
- **Oracle**: D2Debugger MCP on `http://127.0.0.1:8790` (mcp__d2debugger__* tools).
  Ghidra static DB on `:8089`.

## One iteration

1. **Oracle health**: `curl -s http://127.0.0.1:8790/status` (or d2dbg_status).
   - DOWN? Relaunch (needs one UAC prompt):
     ```
     DIABLO2_PATCH=<root>\build-1.13c\patch D2_DEBUGGER=1 \
       <root>\build-1.13c\external\D2.Detours\source\Release\D2.DetoursLauncher.exe \
       "C:\Diablo2\ProjectD2\Game.exe" -- -w
     ```
     A relaunch may hit a transient "Diablo II Error" startup dialog — dismiss via
     Win32 EnumWindows+WM_CLOSE, relaunch once more (comes up clean).
     Then `d2dbg_reload_provider` + `d2dbg_set_all_modes shadow`.
     **Load a character**: poll `d2dbg_list_characters` until count>0 (the
     `charSelectReady`/`charListLoaded` status flags are UNRELIABLE — ignore them),
     then `d2dbg_load_character summoner-skele` difficulty 0. Wait until status
     captureCount>300 and capturedHandle!=0 (in-world with a live unit handle).
2. **Run one set**: from the harness dir,
   `FUNDOC_ADVERSARIAL_VET=0 ..\.venv\Scripts\python.exe pset.py --count 10`
   (in background; ~5-8 min: parallel draft then fast serial prove). Reads
   `pset_report.json` for the result.
3. **REVIEW each result** (the human-intelligence gate):
   - `proven` → done (writeback is automatic).
   - `FAIL mismatch/prove/marshal_fault` on ONE function → likely a draft-quality
     one-off. Escalate to intelligence: re-draft it yourself (read the decompile
     via Ghidra, the oracle's per-vector mismatch, write a correct reimpl, prove
     with `run_live_prove`/`run_handle_prove`), capped at a few oracle rounds.
   - A FAILURE PATTERN across several fns (same stage/reason) → a SCRIPT bug
     (classifier gap, ABI, crash class). Fix the PIPELINE — that fixes it for all
     future fns too. This is the higher-leverage choice; prefer it when the
     signal is systemic.
   - `SKIP stateful/shadow_leaf_pending/handle_abort` → correctly deferred (needs
     a capability that doesn't exist yet, or shadow-first). Bucket it; don't force.
4. **Update state + next set**: append the result to `loop_state.json` journal,
   mark attempted, then loop to step 2. Refill `getter_queue` when low
   (name-scan getter-dense subsystems — see the July-13 journal entries).

## Standing constraints (learned the hard way — see PLAYBOOK for detail)

- ONE oracle/game = ONE shared provider build + candidates dir. NEVER run two
  proving processes against the same game (build/oracle races corrupt both).
- Abort-class handle-getters and delegate getters that deep-deref a captured
  handle DEFER to shadow-first — direct-proving them crashes the game
  uncatchably (killed it 3× this project).
- Idle-town captures make handle-getter proofs degenerate (weak_proof flags them;
  battletest under real play refutes false-lives).
- Genuinely-mutating fns (constructors/mutators/serializers) are out of scope for
  direct proving — they need state-diff capture (unbuilt).
- Code fixes to the pipeline stay uncommitted unless the user says commit.

## Concurrency: a SECOND game instance (next-project, NOT yet built)

A second instance genuinely multiplies throughput but is a real build, gated on
these (do them in order):

1. **Draft-only hard guard** (concurrency PREREQUISITE): parallel draft workers
   must NEVER touch the build/oracle. Today `FUNDOC_DRAFT_ONLY=1` checkpoints
   only the two LLM prove loops in `fun_doc.py` (`process_global_leaf_live`,
   `process_handle_leaf_live` — search `_draft_only_dump`). The MECHANICAL
   short-circuit (`GLOBAL-TABLE SHORT-CIRCUIT`, ~fun_doc.py:11289) and the SYNTH
   paths (`mech_translated`/`_synth_kind`, ~12333) prove BEFORE the checkpoint
   and leak (a worker proved UNITS_GetUnitStat84 in-worker, 2026-07-14). Add the
   same `_draft_only_dump` checkpoint at those two sites so ALL provable paths
   capture a draftmeta instead of proving. Verify: a draft worker set makes ZERO
   oracle calls (watch captureCount / the dispatchers).
2. **Second game on its own port**: D2Debugger's bridge port is currently fixed
   at 8790 (see `conformance/d2debugger_mcp/server.py` + D2Debugger.LiveDispatch).
   Make it configurable (env), launch a second PD2 Game.exe on :8791 with its own
   character.
3. **Isolated provider per instance**: each game loads its own provider DLL from
   its own candidates dir + build tree, else the two builds clobber each other.
4. **Queue partitioning**: split `getter_queue`/cursor so the two instances take
   disjoint functions (even/odd, or two cursors). Both write proofs to the SAME
   `proven_functions.jsonl` (append-only, safe) but NEVER share a candidates dir.

Until all four exist, run ONE instance only. The parallel-DRAFT harness (pset.py)
already captures most of the throughput win within one game.

## Model tiering (throughput)

Bulk drafting is minimax (fast, cheap) via the pipeline. The LAST MILE — mismatch/
malformed one-offs the script abandons — is where YOU (the agent) add value:
iterate against the live oracle until it proves, capped. That is the whole
script-vs-intelligence balance: automate the 80%, apply intelligence to the 20%.
