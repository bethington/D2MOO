# Full D2Common 1:1 Shadow-Conformance Plan

**Goal (set 2026-07-07):** a 1-for-1 D2MOO reimplementation of *every* function in
`D2Common.dll` (1,172 exports), each proven equivalent by **live shadowing** against
the real PD2-S12 original, and **promoted to a live dispatcher** once it's ready.

This doc is the source of truth for the architecture that makes 1,172 tractable.
It supersedes the ad-hoc, hand-wired approach (5 coord functions) with a
**data-driven generator** so promotion is one manifest line + a rebuild.

---

## The three rungs (CONF_* taxonomy) and what "promote" means

| Rung | Mechanism | Evidence | Cost to add |
|------|-----------|----------|-------------|
| `CONF_VECTORS` | offline vectors vs compiled D2MOO | hand-picked inputs | trivial |
| `CONF_LIVE` | **direct-call oracle** (`/oracle`) — calls orig+reimpl with chosen inputs in the live game, compares | our inputs, real game state | 1 `.spec.json` |
| `CONF_BATTLETESTED` | **shadow dispatcher** — hooked into the game's real call path; every real call runs orig (wins) + reimpl, compares | the game's *own* input distribution, at volume | 1 manifest line + rebuild |

"**Promote**" = move a function from the oracle rung to the shadow-dispatcher rung.
The oracle is the fast **pre-filter** (prove cheaply, no game hook); the dispatcher
is the **destination** the goal asks for (real-distribution proof + the swap
mechanism to eventually *run* on the reimpl).

**Promotion gate:** a function is "ready" to promote when it is already `CONF_LIVE`
(oracle-proven bit-exact) AND falls in a dispatcher class we support (below). It
earns `CONF_BATTLETESTED` after accumulating real shadow calls with **zero
divergence** across genuine gameplay.

---

## Why the coord family was easy and 1,172 is not

The 5 coord functions all share ONE signature — `void __stdcall(int*, int*)` — so a
single macro (`LiveDispatch_CoordFamily.h`) instantiates them and comparison is
trivially the two out-params. Real D2Common functions have **arbitrary signatures**
and most **return a value** rather than writing out-params. Hand-wiring a
macro + bridge entry + `ApplyPatchAction` per function does not scale.

## The generalization: a code generator

A data **manifest** (`conformance/shadow_manifest.json`) lists functions ready to
promote: `{name, offset, callconv, ret, arity, class, reimpl}`. A Python generator
(`conformance/tools/gen_shadow_dispatch.py`) emits
`D2.Detours.patches/1.13c/D2Common_ShadowDispatch.gen.h`:

- one **dispatcher namespace** per entry (atomic mode + trampoline slot + hit/divergence
  counters + swappable reimpl pointer + a typed `Thunk`),
- the **bridge table** `LiveDispatchGen::g_entries[]`,
- the accessor functions the coord header's C exports delegate to, and
- `LiveDispatchGen::Install(HookContext*)` — the `ApplyPatchAction` calls.

Promotion is then: add a manifest line → regenerate → rebuild the patch DLL +
restart PD2. The reimpl itself comes from the existing hot-reload provider DLL
(`conformance/reimpl_provider/`), auto-bound **by name** — D2Debugger already
iterates every dispatcher in the bridge and binds `provider[name]` into its reimpl
slot ([D2Debugger.LiveDispatch.cpp:298](../source/D2Debugger/src/D2Debugger.LiveDispatch.cpp#L298)),
so **no D2Debugger change is needed** for new entries. The whole stack is
data-driven off `D2MOO_LiveDispatch_GetCount/GetName/GetOffset`.

### Shared shadow semantics (all classes)

- **Original wins:** in Shadow mode the game observes the *original's* result; the
  reimpl runs on an independent copy purely to compare.
- **Reentrancy guard** (`tl_inDispatch`): a reimpl that transitively re-enters any
  dispatcher on the same thread forces Original — never nests a Shadow.
- **Drain counter** (`g_reimplInFlight`): a hot-reload quiesces (all Original) then
  waits for 0 before `FreeLibrary`-ing the provider — never unmap code mid-call.
- **Verified offset only:** hooked by `D2Common+off` (corrected_maps), never the
  scrambled `.def` ordinal (see `ORDINAL_RECONCILIATION.md`). This is why it's safe
  even with `DISABLE_ALL_PATCHES` on — the legacy ordinal sweep is the dangerous
  part; `DllPreLoadHook` by verified offset is the proven-safe path.
- **Byte-return quirk:** thunks return the full `uint32_t` EAX unchanged (faithful
  forwarding); only the *comparison* masks to the return width (u8→0xFF, u16→0xFFFF).

---

## Dispatcher classes (in order of difficulty / rollout)

- **A — return-value integer** *(SHIPPED 2026-07-07)*. Scalar int/ptr args, scalar
  int return (≤32-bit), comparison = masked return value, reimpl is pure or
  read-only-through-pointer. Covers getters, table lookups, calculators. First batch:
  `GetSeedHi`, `GetItemRandSeed`, `GetDataTableRowEntryCount`,
  `DUNGEON_GetTownLevelIdFromActNo`, `UNIT_GetMode`.
- **B — void with out-param** *(next)*. `void f(T* out, ...)`, comparison = the
  written buffer. Needs the coord-style pattern: run orig, snapshot its output, run
  reimpl on an independent copy of the *inputs*, compare. (The coord family is B,
  specialized.) Candidates already written: `InitRngSeed`, `SetCoordPair`.
- **C — state-mutating** *(hard)*. Mutates a live game object in place. Shadow's
  double-mutation problem: orig mutates, then reimpl mutates already-mutated state.
  Requires **snapshot/restore** of the touched state between the two calls (or a
  deep-copy of the object). Ties into the game-thread call queue.
- **D — register-explicit ABI** *(naked thunk)*. Original uses a non-standard
  convention (e.g. `SEED_GetRandomNumber`: pSeed in ECX, max in EAX). The installed
  Thunk must match via a naked stub; the reimpl is called through the normal
  marshaller. The oracle already handles this (`orig_regs`); the dispatcher needs the
  naked-thunk generator.
- **E — 64-bit return** (edx:eax). Thunk returns `uint64_t`, compare full width.
- **F — float / struct return** (st0 / hidden-pointer). Later; distinct ABI handling.

Class is a manifest field; the generator emits the right thunk shape per class.
Batches A/B/D/E are pure codegen once designed; C is the research frontier.

---

## The scaling loop (per function, eventually fun-doc-driven)

1. **Reimpl** — fun-doc worker reads the Ghidra decompilation + verified prototype,
   writes an equivalent into `reimpl_provider/candidates/`. (The hard creative work.)
2. **Prove cheap** — oracle `/oracle` with generated vectors → `CONF_VECTORS`/`CONF_LIVE`.
   Fast, no game hook, catches most bugs.
3. **Promote** — add a manifest line (class from the signature) → regenerate → rebuild.
4. **Shadow** — play; the dispatcher compares on real calls. Zero divergence over a
   real session → `CONF_BATTLETESTED`. Any divergence auto-logs a vectors-schema JSON
   line → a permanent offline regression case for free.
5. **Write-back** — CONF_* tags + `proven_functions.jsonl` updated (per the
   write-back-to-source-of-truth principle). Divergences reopen the reimpl.

**Priority order:** the dynamic profiler (`DYNAMIC_PROFILER_PLAN.md`) ranks D2Common
exports by real hit count. Promote hot functions first — they accrue
`CONF_BATTLETESTED` evidence fastest and matter most to determinism.

---

## Batch cadence & safety

- Promote in **batches** (e.g. 10–30), one rebuild + restart per batch — the
  per-function marginal cost is a manifest line.
- A blind mass-hook of all 1,172 at once was historically unsafe from two now-fixed
  bugs (aliased ordinals, data exports — see `DYNAMIC_PROFILER_PLAN.md`); we hook a
  curated, verified-offset set, so that risk does not apply.
- Every shadow call is guarded (reentrancy + original-wins + the reimpl runs last on
  a copy). A bad reimpl diverges (logged) rather than corrupting game state.

## Status

- **Class A generator + first batch of 5**: SHIPPED 2026-07-07 (this doc's commit).
- Bridge unified: coord (5) + generated entries share one contiguous index space;
  D2Debugger sees all of them with no code change.
- Next: live shadow-prove the batch in-world; then Class B (`InitRngSeed`,
  `SetCoordPair`); then wire fun-doc to emit manifest lines automatically.
