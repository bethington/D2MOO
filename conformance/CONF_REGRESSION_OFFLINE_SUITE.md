# CONF_REGRESSION — the offline, CI-enforced regression suite

**The goal (user's framing):** a suite of offline tests, ideally generated from
**real game data** and backed up by **synthetic** data, that any code change can be
run against to verify it doesn't break a proven function — *before* merge, with **no
running game**. This is the rung that makes a proof *permanent*: `CONF_LIVE` and
`CONF_BATTLETESTED` both evaporate the moment PD2 closes; a frozen corpus in the
offline suite keeps guarding the function forever, in CI.

## Where it sits in the ladder

```
draft reimpl
  → CONF_LIVE          prove bit-exact vs live oracle (chosen vectors)         [needs game]
  → CONF_BATTLETESTED  zero divergences under shadow over real gameplay        [needs game]
  → CONF_REGRESSION    freeze real+synthetic vectors → offline doctest in CI   [NO game]  ← this doc
```

`CONF_REGRESSION` is the *frozen, shipped, CI-guarded* state. Earning it requires
two things the live rungs don't:
1. The reimpl is **promoted into shipping D2MOO** (in `source/D2Common/…`, not only
   `conformance/reimpl_provider/candidates/`). The offline suite links and tests the
   **real compiled D2Common** — the code that ships — so the function must live there.
   (See `SHIPPING_PROMOTION_PLAN.md`.)
2. The ground truth is **frozen** to a corpus file so it needs no oracle to re-check.

## The two data sources — and why both

Per the vetting principle, every function should carry **both** flavors of vector:

| Source | `src` tag | Proves | Blind spot it has |
|---|---|---|---|
| **Real** (captured live / from gameplay) | `real` | matches the game where the game actually goes | rare/edge inputs a session never hit |
| **Synthetic** (generated edge cases) | `synthetic` | matches at boundaries gameplay may never reach | inputs the game never actually passes |

The coord family is the proof: the `DUNGEON_GameToClientCoords` SAR-vs-truncating-
divide bug (commit `26ed87f`) only shows on **negative** inputs — synthetic INT16
extremes catch it; real town-coordinate traffic might never feed a negative. Conversely
only real gameplay found the `GameSubtileToClientCoords` off-by-one at `y≈5021`. Freeze
both; when you believe a reimpl is 1:1, synthetic edges are how you find the divergence
real sampling missed.

## The pipeline (and the tools)

```
  shadow mode (both run, diff)                    conformance/tools/
      │  match → golden {in → real out}
      ▼
  captured_vectors.jsonl  ──capture_to_corpus.py──►  regression/<fn>.captured.cases.json
                                                          │
  live oracle (/call,/oracle) ──freeze_corpus.py──►  regression/<fn>.cases.json
  (+ hand-added synthetic edge cases)                     │
                                                          ▼
                                           gen_offline_tests.py  (corpus → doctest)
                                                          │
                                                          ▼
                          source/D2Common/tests/PD2S12Conformance.gen.cpp
                                                          │  compiled into D2CommonTests
                                                          ▼
                                     ctest  (offline, no game)  ← pre-merge gate
```

- **Capture (opt-in):** launch PD2 with `D2MOO_CAPTURE_VECTORS=1`. The generated shadow
  dispatcher's `LogMatch`/`LogMatchBuf` (see `gen_shadow_dispatch.py`) records **distinct,
  first-seen** samples (capped 64/fn — a 100M-hit path costs a hash-set probe, not 100M
  lines) to `behavioral/captured_vectors.jsonl`. Default OFF → zero change to normal shadow
  proving. Needs a patch-DLL build + game restart to take effect (patches inject at load).
- **`freeze_corpus.py`** — the live-oracle freeze: calls `/call`/`/oracle`, reads the **original
  DLL's** outputs, writes `regression/*.cases.json`. Ground truth is never hand-typed.
- **`capture_to_corpus.py`** — folds `captured_vectors.jsonl` into corpora (dedups, skips
  functions it can't replay offline — see boundary below).
- **`gen_offline_tests.py`** — corpus → `PD2S12Conformance.gen.cpp` (checked in, so the test
  build needs no Python). Re-run after editing any corpus.

## The offline-safe boundary (pure vs. stateful) — READ THIS

A function is replayable by a **trivial value replay** only if its inputs are
self-contained **values**. Two classes need more:

- **Pure** (coord family, RNG math, `DUNGEON_GetTownLevelIdFromActNo`): args are scalars,
  output depends only on them. → freeze `{in → out}`, replay directly. ✅ supported now
  (`coord2`, `scalar_ret` ABIs).
- **Stateful via pointer** (unit/struct getters, `kind:"ptr"` args): the function reads
  memory **behind** the pointer. Capture logs the pointer *value*, meaningless offline.
  → needs **input-STATE capture** (snapshot the pointed-to bytes into the vector) or an
  offline fixture. `capture_to_corpus.py` **skips these with a note** rather than emit a
  bogus test.
- **Stateful via global** (`GetDataTableRowEntryCount` reads loaded `.txt` tables): needs
  the state **loaded** offline. The fixture already exists — `d2moo_loader_harness.cpp`
  opens the real MPQs and drives D2Common's loader — so these get an offline home by
  running against loaded tables, not a bare vector replay.

This boundary is the honest answer to "an offline test for **each** function": pure
functions are free; stateful ones are earned by capturing/reconstructing their state.
Input-state capture for `ptr` args is the next extension.

## Corpus schema

```jsonc
{
  "abi": "coord2" | "scalar_ret",   // dispatch shape; extend EMITTERS in gen_offline_tests.py
  "header": "D2Dungeon.h",          // D2MOO header declaring the fn
  "frozen_from": "…", "frozen_date": "…",
  "functions": [
    { "fn": "DUNGEON_GetTownLevelIdFromActNo",
      "args": ["nAct"],                                  // scalar_ret: ordered param names
      "cases": [ { "in": {"nAct": 0}, "ret": 1, "src": "real" }, … ] }
  ]
}
// coord2 case shape: { "in": {"x","y"}, "out": {"x","y"}, "src": … }
```

Adding an **ABI**: add an emitter to `EMITTERS` in `gen_offline_tests.py`. Adding a
**function**: freeze/capture its corpus, drop the `.cases.json` in `regression/`, re-run
the generator, rebuild `D2CommonTests`.

## Relation to fun-doc

When a fun-doc worker ports a function and the reimpl is proven live, freezing its vectors
into `regression/` makes **the documentation itself CI-verified**: if a later edit makes the
shipped function diverge from the captured game behavior, the offline gate fails. Proof stops
being a one-time event and becomes a standing invariant. (`FUNDOC_D2MOO_INTEGRATION_PLAN.md`.)

## Current state (2026-07-07)

Offline suite = **12 doctest cases / 174 assertions, green, no game**:
- `PD2S12Conformance.cpp` (hand-written, pre-existing): 6 coord/RNG cases.
- `PD2S12Conformance.gen.cpp` (generated from `regression/*.cases.json`): 5 coord functions
  (`coord2`, 54 cases) + `DUNGEON_GetTownLevelIdFromActNo` (`scalar_ret`, 5 cases).
- Regression-catch verified: corrupting one frozen output makes `ctest` exit non-zero.

Next: input-state capture for `ptr`-arg getters; a `d2moo_loader_harness`-backed home for
global-state functions; promote more `CONF_LIVE` functions into shipping D2Common so they
become eligible for the frozen suite.
