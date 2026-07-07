# Promoting a reimpl to "shipping" — the vetting ladder

A function proven by the automated live path is `proven_live_pending_review`
(`CONF_LIVE`): bit-exact vs the original across **the vectors the drafting model
chose**, in **one game state**. That is real evidence, but not yet shipping-grade,
because of three residual risks:

1. **Self-consistency bias** — the same model wrote BOTH the reimpl and its test
   vectors, so it tends to test what it implemented, not what the original does. A
   subtle branch the model misread would be missed by the model's own vectors.
2. **Input-coverage gaps** — 12–15 vectors can miss an exact boundary
   (count−1 / count / count+1), an overflow, or a value with special handling.
3. **State-dependence** — the return can depend on loaded/mutable game state.
   (Low for pure data-table readers: PD2's `.txt` tables load once and are identical
   for every player and act. Higher for functions that read unit/room/mutable state.)

The vetting ladder closes these, in increasing strength:

---

## Rung V1 — Independent adversarial re-proof  *(automatable, no user, no rebuild)*

A SEPARATE model call, framed as an adversary and shown ONLY the decompile (never
the reimpl), generates a comprehensive vector set aimed at breaking the reimpl:
every branch boundary the decompile tests, a dense sweep of the input range, and
extremes (0, −1, INT_MIN, INT_MAX, powers of two). Re-run the oracle (the reimpl is
already staged) against these. Kills risks #1 and #2. A failure REOPENS the reimpl.

Status tag: `CONF_LIVE` stays; add an independent-verification marker (registry
`vetted: adversarial`). Cheap; run automatically after every CONF_LIVE proof.

## Rung V2 — Shadow over real gameplay  →  `CONF_BATTLETESTED`  *(the shipping bar)*

The gold standard (this is the mechanism you described). Hook the ORIGINAL in the
game's real call path; on EVERY real call, run original + reimpl and compare. Zero
divergence over a volume threshold with **diverse inputs** = proven against the
game's own input distribution, which no synthetic vector set can fully replicate.

"Large enough" is **volume × input-diversity × state-diversity**, not raw count:
- **volume**: ≥ N real calls, 0 divergence (start 10k for shipping; the current
  `battletest_promoter` bar is 1k for CONF_BATTLETESTED).
- **input diversity**: count DISTINCT inputs seen — 1M calls with 3 inputs is not
  coverage. (Enhancement: the shadow thunk should sample distinct arg values.)
- **state diversity**: which acts/difficulties/char-classes/game-modes were played
  while shadowing (matters for state-readers; negligible for pure table-readers).

**What's automatic vs needs you:** the game calls many functions on its own during
normal play (data-table lookups fire constantly — loading a char, casting skills,
generating items, entering areas), so those accrue shadow evidence hands-off. Rarely
called ones need you to trigger the specific gameplay. Either way the
`battletest_promoter` polls the counters and promotes automatically once the bar is
met — you just play; it watches.

**Gap to close first:** these live-proven data-table functions take their arg in a
NON-STANDARD register (EAX-input, register-explicit ABI). The shadow-dispatcher
generator only emits Class A/B thunks today. Shadowing them needs **Class D** — a
naked register-explicit thunk (the oracle already handles this ABI via `orig_regs`;
the generator needs the matching thunk). Until Class D lands, these can reach V1 but
not V2.

## Rung V3 — Human review + coverage ledger  →  SHIPPING

Final gate before a reimpl replaces the original in a shipped D2MOO build:
- A human reads the reimpl next to the decompile (it's small, mechanical code).
- A **coverage ledger** per function records the game states V2 evidence was
  gathered in, so "shipping" means "battle-tested across the states that matter for
  this function," not "battle-tested in whatever state happened to be loaded."
- Only then does it move from `candidates/` (proven, unmerged) into the shipping
  reimpl set.

---

## Build order (proposal)

1. **V1 adversarial re-proof** — automatable, high value, no blockers. Build now.
2. **Class D shadow generator** — unlocks V2 for the register-explicit majority the
   automated loop produces. The gold standard; needs some of your gameplay to accrue
   evidence, but the promotion itself is automatic (`battletest_promoter`).
3. **Shadow input-diversity + coverage ledger** — makes "large enough" measurable
   and makes V3 a real gate rather than a judgment call.

The through-line: nothing is ever trusted on a model's word. Every rung is a
comparison against the REAL original — V1 broadens the inputs, V2 uses the game's
own inputs at volume, V3 confirms the states covered. A reimpl only ships when it
has out-argued the original everywhere it's been asked.
