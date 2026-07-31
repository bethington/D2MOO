# Conformance taxonomy v2 — proposal (2026-07-29)

Revision of `CONFORMANCE_TAXONOMY.md`, driven by a concrete goal: **produce a
functionally equivalent D2Client.dll**. Supersedes v1's `CONF_*` axis. The
`DOC_*` axis is unchanged.

Decisions locked with Ben 2026-07-29. Status: design agreed, **not yet
implemented** — see "Gaps to build".

---

## Why v1 needed revising

Measured against the live programs, not the doc:

| Finding | Evidence |
|---|---|
| `CONF_DRAFT` and `CONF_VECTORS` have **zero writers anywhere in the codebase** | grep: nothing calls `record_proof`/`_set_rung` with either level. use_count 0 in both D2Common and D2Client. |
| Two disconnected state machines | fun-doc workers write `port_status` (14 values) to SQL; the `CONF_` ladder is written only by `port_live_prove.record_proof()`, reached only from `fast_path.py`. `fun_doc.py:12741` ends the main worker path at `port_status="proven_live_pending_review"` and never tags. |
| The selected binary is entirely untagged | D2Client.dll: 0 `CONF_` tags across 5,739 functions, while SQL holds 35 `proven_pending_review` + 1 `proven_live_pending_review` for it. |
| 1,835 functions are invisible | `stateful_skip` (876 D2Client + 959 D2Common) renders identically to never-attempted. |
| The shipping gate is invisible | `SHIPPING_PROMOTION_PLAN.md` says `CONF_LIVE` is not shipping-grade; `adversarial_reproof.py` exists but records `vetted` as a registry field, not a rung. 7 of 199 proven functions are vetted; all 199 read the same. |
| `CONF_REGRESSION` bundles two unrelated facts | "reimpl is in the shipping binary" and "frozen ctest case guards it" — you can hold either without the other. |
| Legacy tags still present | `ORACLE` / `PORTED` / `PROVEN` (3 each, D2Common), retired pre-taxonomy OpenD2 scheme. |

The taxonomy was not wrong. It was **unwired**.

---

## Axis 2 (revised) — `CONF_*`: equivalence proof

**Rungs are earned, not walked.** A function holds the highest rung it has
earned; which lower rungs it passed through depends on whether it is statically
provable. Mutually exclusive — promoting removes the others.

Untagged = never attempted.

| Rung | Meaning | Earned by |
|---|---|---|
| `CONF_BLOCKED` | **Off-ladder.** Attempted; not provable by the current oracle. Reason (`stateful` / `unsupported_abi` / `no_vectors` / `oracle_unavailable` / `abort_hazard`) in the `Conf` property map. | worker classification |
| `CONF_REFUTED` | **Off-ladder.** A rung was earned and then **falsified** — a shadow divergence, a failed adversarial re-proof, or a red regression case. The counterexample (inputs, expected vs actual, source) is in the `Conf` property map. | demotion (see below) |
| `CONF_DRAFT` | A D2Client/D2MOO equivalent exists, untested. | reimpl written to `source/D2Client` or `candidates/` |
| `CONF_VECTORS` | Passes static/offline vectors — no live game. Pure/leaf only. | `port_pipeline.run_harness` pass |
| `CONF_LIVE` | Bit-exact vs the real function in the running game, on **the drafting model's own vectors**. | `port_live_prove.run_live_prove` pass |
| `CONF_VETTED` | Also survives an **independent adversary's** vectors (branch boundaries, dense sweep, 0/−1/INT_MIN/INT_MAX), with a **discriminating** result — `weak_proof: DEGENERATE` does not qualify. | `adversarial_reproof.py` pass + `prove_doc.is_discriminating` |
| `CONF_BATTLETESTED` | Zero divergences under shadow mode over **real gameplay** — ≥1k calls **AND** a distinct-input floor. | `battletest_promoter` poll |
| `CONF_SHIPPED` | **Terminal.** Dispatched by default in the running game, at ≥10k zero-divergence calls. | promoter + default-dispatch flip |

Orthogonal flag in the `Conf` property map (not a rung):

- `regression_frozen: true` — a frozen offline `ctest` case guards this function
  (`regression/*.cases.json` → `gen_offline_tests.py`). Formerly `CONF_REGRESSION`.

### Changes from v1

- **Added** `CONF_BLOCKED` — makes the real denominator visible: proven + blocked
  + untouched = in-scope total. Doubles as the work queue for the next oracle
  capability.
- **Added** `CONF_VETTED` — the adversarial gate becomes countable. Existing
  `CONF_LIVE` tags keep their meaning; no mass demotion.
- **Added** `CONF_SHIPPED` — the only rung that makes "the binary is equivalent"
  a measurable statement.
- **Removed** `CONF_REGRESSION` as a rung → demoted to a property flag. Retag the
  3 existing D2Common functions.
- **Removed** legacy `ORACLE` / `PORTED` / `PROVEN` tags entirely.
- **Kept** `CONF_DRAFT` and `CONF_VECTORS` — the concepts are right, they were
  simply never written. 81 functions across binaries have already *earned*
  `CONF_VECTORS` via the static harness (`proven_pending_review`) without
  receiving it.

---

## The single state machine

v1's split is closed: **`port_status` transitions drive the `CONF_` rung.**
`port_status` keeps the fine-grained operational detail (`port_failure_stage`,
`port_attempts`, `port_last_result`); `CONF_` is the coarse, queryable,
`.gzf`-durable projection in the source of truth. Ghidra remains authoritative
(writeback-source-of-truth); `proven_functions.jsonl` remains the git mirror.

| `port_status` | `CONF_` rung |
|---|---|
| *(none)* | untagged |
| `shadow_leaf_pending`, draft written | `CONF_DRAFT` |
| `proven_pending_review` (static harness) | `CONF_VECTORS` |
| `proven_live_pending_review`, `proven_live` | `CONF_LIVE` |
| — (adversarial pass) | `CONF_VETTED` |
| — (promoter) | `CONF_BATTLETESTED` |
| — (build) | `CONF_SHIPPED` |
| `stateful_skip`, `unsupported_abi`, `no_vectors`, `oracle_unavailable`, `handle_abort_hazard_skip`, `unknown_skip` | `CONF_BLOCKED` + reason |
| `live_prove_failed`, `harness_failed`, `malformed_response`, `error` | no rung change (candidate is deleted; the failure lives in `port_status`) |

`*_pending_review` stops being a dead end: the rung *is* the review state, and
`CONF_VETTED` is the gate that used to be an unimplemented human review.

### Demotion (new — nothing like this exists today)

A rung is a claim, and claims can be falsified. Any of these **demotes to
`CONF_REFUTED`** and writes the counterexample into the `Conf` property map:

- a shadow divergence recorded after promotion,
- an adversarial re-proof failure on a `CONF_VETTED`+ function,
- a frozen regression case failing in `ctest`.

Without this, a falsified proof keeps its tag forever.

**Why `CONF_REFUTED` and not back to `CONF_DRAFT`** (revised 2026-07-29; the
first draft of this document said `CONF_DRAFT`): a refuted function is not the
same as an untested one. It carries a *known counterexample*, and the reimpl
that produced it is known-wrong in a specific way. Collapsing the two means

- a tag query cannot distinguish "never tested" from "tested and FAILED", and
- the function re-enters the normal queue where a worker can re-draft, re-prove
  against its own vectors, and re-promote the same broken reimpl — a regression
  loop with no memory of the divergence.

`CONF_REFUTED` is off-ladder for the same reason `CONF_BLOCKED` is: it must
never win a rung-strength comparison, and it doubles as a work queue.
Clearing it requires addressing the recorded counterexample, not just producing
a new draft.

---

## D2Client: shadow-first, not vectors-first

D2Common was provable vectors-first because it is mostly pure data-table and
math. D2Client is rendering, input, UI, network and unit state — the static
`/emulate_function` oracle is pure/leaf-only by design, and 876 D2Client
functions already classify `stateful_skip`.

**The order inverts.** Rather than constructing state to call a function, hook
the ORIGINAL in the real call path and run original + reimpl on every real call.
The game supplies the state for free; nothing marshals a `UnitAny` or `Room`.
For D2Client, `CONF_BATTLETESTED` is the *primary* route, not a final polish
step — many functions will go `CONF_DRAFT` → `CONF_BATTLETESTED` directly,
never holding `CONF_VECTORS` or `CONF_LIVE`.

Pure/leaf D2Client functions still take the vectors path where it works.

### Promotion thresholds (volume AND diversity)

`SHIPPING_PROMOTION_PLAN.md` is explicit that raw volume is not coverage — "1M
calls with 3 inputs is not coverage" — and names distinct-input sampling as an
unbuilt gap. That gap is closed here: the shadow thunk samples distinct argument
values, and both thresholds must be met.

| Rung | Calls, zero divergence | Distinct inputs |
|---|---|---|
| `CONF_BATTLETESTED` | ≥ 1,000 | ≥ floor (per-function, defaults to 20) |
| `CONF_SHIPPED` | ≥ 10,000 | ≥ floor |

The two numbers are the plan's own (1k for battletested, 10k for shipping); now
that `CONF_SHIPPED` is a separate rung, there is somewhere to hang the higher
bar. A high-volume function that hammers two code paths now visibly fails to
promote instead of promoting on worthless evidence.

#### Saturation — when the flat floor is unsatisfiable

Some functions cannot reach 20 distinct inputs *at all*: an enum mapper or
lookup has a smaller reachable input space than the floor, so it would stall at
its current rung forever, silently. The promoter therefore treats the input
space as covered when `distinct_inputs` stops increasing despite
`SATURATION_HITS` (500) further zero-divergence calls, and waives the
**diversity** floor only — the volume bar always applies.

**Reachable, not legal, domain.** An earlier attempt declared the domain size in
the manifest (`input_domain`) and was removed: a declaration encodes what the
function *legally accepts*, but promotion depends on what live callers
*actually pass*. `GetItemQualityStringId` legally accepts quality 0–6, so
`input_domain: 7` looked right — yet its reachable paths only ever pass 3
values, so it merely swapped an unsatisfiable floor of 20 for an unsatisfiable
7. Saturation is *measured*, so it cannot be wrong about a domain it never
observes.

**Two sessions required** (`REQUIRED_SATURATED_SESSIONS = 2`). Within-session
saturation proves only that the paths *that session* exercised are exhausted; it
says nothing about a different act, character class or difficulty passing a
value the session never could — precisely the "state diversity" axis
`SHIPPING_PROMOTION_PLAN.md` calls out. A session boundary is detected by the
hit counter *dropping* (the patch DLL's counters are process-static and restart
at 0 on relaunch), and each session counts at most once. Raised from 1 to 2
after `ITEMS_GetUltraOrBaseCode` promoted on 3.46M calls with 4 distinct inputs
from a single session: technically saturated, far too thin for the claim.

Saturation watermarks live in `conformance/shadow_watermarks.json`, deliberately
**separate** from `proven_functions.jsonl` — the registry mirrors the proof
axis and must not be buried under per-poll telemetry for every dispatcher.

A saturation-based promotion records its own basis in the `Conf` property map
(`diversity: saturated@3 across 2 sessions (...)`) so the rung's evidence stays
auditable rather than implied.

**Rungs earned under an older, weaker bar are left standing** (same precedent as
the pre-diversity `CONF_BATTLETESTED` set): their `Conf` records state the
actual basis, and a real divergence still refutes them automatically. The ladder
is corrected forward by evidence, not by mass retagging.

### Delivery: provider DLL, not a drop-in binary (yet)

Reimpls ship through D2MOO's hot-reloadable **reimpl-provider DLL** (WS-1),
with D2.Detours installing a dispatcher on the original — the mechanism already
proven for D2Common. `CONF_SHIPPED` therefore means *the running game routes
this call to our code by default*, which is a stronger practical claim than
"compiled into a DLL nobody loads". Assembling an actual drop-in D2Client.dll
is a later step, once enough functions are proven to make it meaningful.

---

## Scope

**In scope: ~4,408 functions.** D2Client.dll has 5,739; excluded are `LIB_CRT`
(264), `LIB_MSVC_EH` (751), `THUNK` (282), `STUB` (34) — the CRT comes from the
toolchain, thunks are linker-generated, stubs are trivial. Exclusion is
automatic via `library_code_detector.py`, matching how the pipeline already
computes `in_scope` for D2Common (2,547 of 2,712).

**Definition of done:** the binary is functionally equivalent when 100% of
in-scope functions hold `CONF_SHIPPED`.

---

## Gaps to build

Ordered by dependency, not by size.

1. **Wire `port_status` → `CONF_` in `fun_doc.py`.** One map + a `_set_rung`
   call at each `update_function_state` site. Backfill the ~280 already-earned
   rows and the 1,835 `CONF_BLOCKED`.
2. **Register the new tag definitions** (`CONF_BLOCKED`, `CONF_VETTED`,
   `CONF_SHIPPED`) with descriptive comments in D2Client.dll and D2Common.dll,
   matching D2Common's existing tag-comment convention. Delete `ORACLE` /
   `PORTED` / `PROVEN`. Retag the 3 `CONF_REGRESSION` functions.
3. **`CONF_VETTED` writer** in `adversarial_reproof.py`, gated on
   `prove_doc.is_discriminating`.
4. **Class D naked register-explicit thunks** + WS-2 dispatcher-from-config,
   retargeted at D2Client addresses. Named as the known gap in
   `SHIPPING_PROMOTION_PLAN.md`. **This is the long pole** — without it the
   shadow-first path does not exist for D2Client.
5. **`source/D2Client` module in D2MOO** + a D2Client resolve table
   (`gen_resolve_table.py` currently emits 1,105 D2Common entries).
6. **`CONF_SHIPPED` writer** at build/link time.
7. **Demotion path** on divergence / adversarial failure / regression failure.

### Validation needed first

D2Client holds 35 `proven_pending_review` + 1 `proven_live_pending_review` in
SQL with **zero** corresponding Ghidra tags. Before backfilling those rungs,
confirm the proofs are real and not residue of the wrong-binary default fixed
2026-07-27 (which silently wrote every non-D2Common proof's tag to
D2Common.dll, usually to no function at all). Backfilling an unverified proof
would launder a bug into a durable claim.

### Scale, stated plainly

D2Common reached 199 proven of 2,547 in-scope (7.8%) over months of live
proving. D2Client's in-scope set is 1.7× larger and structurally harder. The
shadow-first inversion is what makes it tractable at all — it converts the
stateful majority from unprovable into passively-accruing-evidence — but this
is a long program, not a sprint.
