# Graduated conformance pipeline -- remaining work

> **STANDING PRINCIPLE -- write-back to the source of truth (2026-07-06).** Every
> discovery is fed back where it won't be lost: RE ground truth -> Ghidra, narrative
> -> markdown. When a proof or investigation reveals something (a corrected calling
> convention/name/type from the DISASSEMBLY, which beats the decompiler annotation;
> a bit-exact proof), record it AT THE SOURCE and `save_program`. Progress is
> marked with a TWO-AXIS tag taxonomy in Ghidra (full spec: CONFORMANCE_TAXONOMY.md):
> `DOC_*` (documentation maturity: DRAFT->REVIEWED->VERIFIED) and orthogonal
> `CONF_*` (equivalence proof: DRAFT->VECTORS->LIVE->BATTLETESTED); mutually
> exclusive per axis, untagged = baseline. Live-proof write-back is AUTOMATIC via
> `port_live_prove.record_proof()` -> sets `CONF_LIVE` (removing lower CONF_ rungs)
> + appends `conformance/proven_functions.jsonl`. First applied: GetDataTableRow
> EntryCount ABI corrected __fastcall->__stdcall in Ghidra; 8 functions now
> DOC_REVIEWED + CONF_LIVE (migrated off the retired single CONFORMANCE_PROVEN
> tag). See the `writeback-source-of-truth` memory.

> **This is the master plan.** It sits on top of the detailed sub-plans/history:
> `LIVE_DISPATCH_FRAMEWORK_PLAN.md` (Tier-2 dispatchers, Phases 0-4, done),
> `DYNAMIC_PROFILER_PLAN.md` (Tier-1 profiler, done + live-proven), and
> `FUNDOC_D2MOO_INTEGRATION_PLAN.md` (the worker vision). Status: **the
> foundation (Tier-1 profiler + Tier-2 5-function dispatch + bridge + UI) is
> built and proven live against real PD2.** What follows is what remains.

The end state (agreed 2026-07-06): a **hybrid** where every function is
whole-engine **profiled** at runtime (Tier-1, exploratory, count-only), and a
function **graduates** to a startup **dispatch** layer (Tier-2) once it has a
D2MOO equivalent worth proving -- Original/Reimpl/Shadow, with the equivalent
supplied by a **hot-reloadable reimpl-provider** so new equivalents can be added
and shadow-proven WITHOUT restarting PD2. A fun-doc worker feeds the pipeline,
prioritized by real profiler hits, and proofs sync back to Ghidra.

## What already exists (the foundation)

- **Tier-1 runtime profiler** -- count-only hooks on all D2Common exports,
  per-subsystem install, dedup + data-export gate + graceful-skip, unified
  browser UI with live per-function hit counts. PROVEN live.
- **Tier-2 startup dispatchers** -- 5 coord functions hooked at startup via
  D2.Detours (`LiveDispatch_CoordFamily.h` / `DllPreLoadHook`), 3 modes,
  divergence detection, offset-keyed bridge (`D2MOO_LiveDispatch_*`) to the UI.
  Data-driven UI matching (new dispatchers auto-appear).
- **registry.json** + `build_registry.py` (ordinal/address/ABI/proof-status).
- **corrected_maps/*.tsv** -- ground-truth ordinal->address->ghidra_name.
- **d2moo_conform** -- offline vector replay against the real compiled functions;
  divergences already emit to `behavioral/live_shadow_divergences.jsonl`.
- **fun-doc** (external) -- mature worker system (document->port->prove for
  OpenD2), marker convention (`@PD2S12`), dashboard, priority queue, SQL state.

## Remaining work, by workstream

> **WS-1 STATUS: DONE + proven live (2026-07-06), including MemoryModule.**
> Swappable reimpl pointer + drain-counted quiesce/reload + hot-swap proven
> (buggy provider hot-reloaded -> divergences climbed live, no restart). Then
> upgraded to **MemoryModule in-memory loading** (vendored `external/MemoryModule`,
> MPL-2.0) with a **custom verified-address import resolver** -- no temp files, no
> renaming, no path-cache bug; provider "mem-loaded, 5/5 bound" confirmed live.
> WS-1.5 verified-address resolver (`D2MOO_ResolveGameFn`, 1105-entry table from
> corrected_maps) is built and is what the resolver routes `D2Common!*` imports
> through, bypassing the scrambled export table.

### WS-1: Hot-reload reimpl-provider (the "add equivalents without restart" core)
1. **Swappable reimpl pointer.** Change each dispatcher's Reimpl/Shadow call from
   a static call to a call through a settable `void* g_reimpl` (default: the
   baked-in D2MOO fn, or null). Small, localized to the dispatcher macro.
2. **Bridge setter.** Add `D2MOO_LiveDispatch_SetReimpl(index, funcptr)` (and a
   name-keyed variant) to the patch bridge.
3. **Reloadable provider DLL.** A separate, fast-building DLL exporting the
   equivalent functions by name. D2Debugger `LoadLibrary`s it, resolves each
   export, calls `SetReimpl`. **Reload protocol:** set all dispatchers to
   Original (quiesce in-flight calls), `FreeLibrary` old, `LoadLibrary` new,
   re-resolve, restore modes. (Guard against unloading a DLL mid-call.)
4. **Prove the mechanism** on the existing 5 coord fns: move their reimpls into
   the provider DLL, hot-reload live, shadow-test -- no game restart. This
   validates WS-1 end-to-end before generalizing.
   *Difficulty: low-medium. This is the highest-leverage next step.*

### WS-2: Generalized dispatcher (beyond the hand-written coord family)
5. **Dispatcher-from-config at startup.** `DllPreLoadHook` currently hardcodes 5
   `ApplyPatchAction`s. Change to: read the Tier-2 registry, and for each
   candidate install a dispatcher by verified address, with the same gates the
   profiler uses (dedup, data-export/executable check, graceful-skip). A bad
   candidate must not stop the game booting -> skip-list + a safe-mode env var.
6. **Dispatcher stub generation for arbitrary ABIs.** The hard technical piece.
   The 5 coord dispatchers are one hand-written signature (`void(int*,int*)`).
   Real functions have arbitrary signatures/calling conventions. Need generated
   stubs: templated thunks for the common ABIs (`__stdcall`/`__fastcall`/
   stack+ECX-EDX), asm trampolines for Blizzard register ABIs (EAX/ESI/EDI --
   reuse the discipline proven in `behavioral/pd2_frida_rng_family.js`).
   *Difficulty: high. Gate this behind WS-1 so hot-reload works first with the
   coord fns while this is built.*

### WS-3: Generic shadow snapshot/compare (correctness for stateful fns)
7. **Arbitrary input/output surfaces.** The coord Thunk hard-codes "snapshot
   *pX/*pY, compare *pX/*pY". Stateful functions read/write structs and globals.
   Shadow must deep-copy the exact read+write set before Original runs, feed the
   copy to the reimpl, and compare the full write-set (return + out-params +
   touched globals). The surface descriptor comes from the registry (sourced from
   Ghidra decompilation). Reentrancy guard already exists.
   *Difficulty: high, per-function. This is what lets us prove the majority of
   game logic that the static `/emulate_function` oracle (fun-doc/OpenD2) can't.*

### WS-4: The graduation registry (the Tier-1 -> Tier-2 bridge)
8. **Unify the registry** as the single source of truth: every function with
   (verified_address, ABI, input_surface, write_set, reimpl_symbol,
   proof_status). Tier-1 = profiled; a row gains a `reimpl_symbol` + surface
   descriptors when it graduates to Tier-2. `build_registry.py` extends to emit
   it; the startup dispatch layer (WS-2.5) and the provider (WS-1.3) read it.
   Profiler hit counts get written back so graduation is prioritized by real use.

### WS-5: fun-doc worker integration
> **WS-6a DONE + proven live (2026-07-06) -- the D2MOO-side proving substrate a
> fun-doc worker needs is complete and demonstrated on a REAL function.**
> - **Generalized drop-in provider:** drop a `.cpp` into
>   conformance/reimpl_provider/candidates/ with a `// D2MOO_REIMPL_EXPORT: <name>`
>   marker; the provider CMake globs it (CONFIGURE_DEPENDS) and auto-generates the
>   module .def -- no CMake/.def hand-edit. Proven: coords still 8/8 through the
>   regenerated def.
> - **Offset-resolved original:** `POST /oracle` accepts `offset`/`addr` for the
>   PD2 identity (from Ghidra) -- required because D2MOO names mostly don't match
>   PD2's scrambled/Ghidra names (only DUNGEON_GameToClientCoords coincides).
> - **CAPSTONE proven:** `DUNGEON_GetTownLevelIdFromActNo` (PD2 offset 0x3b1e0,
>   Ghidra name DATATBLS_GetBoundedArrayValue) -- a real non-coord fn exercising
>   the marshaller's scalar-arg + non-void-return path -- proven live 5/5 allMatch
>   (acts 0..4 -> {1,40,75,103,109}) against the running game. Whole flow is the
>   exact one fun-doc will automate: Ghidra offset -> draft reimpl -> drop-in ->
>   `prove_candidate.py --spec` -> verdict. Candidate: candidates/town_levelid.cpp;
>   spec: vectors/town_levelid.spec.json.
>
> **WS-6b DONE + tested (2026-07-06).** fun-doc live-prove gate built and wired:
> - `fun-doc/port_live_prove.py` (standalone, like port_pipeline.py): translates
>   fun-doc's REGISTER `param_layout` -> the oracle's convention+slot spec
>   (translate_layout_to_spec: stack=stdcall, ECX=thiscall/fastcall-1, ECX+EDX=
>   fastcall; exotic register ABIs raise UnsupportedLiveABI so the caller falls
>   back to static), writes the reimpl into the D2MOO provider candidates/, writes
>   the spec, runs prove_candidate.py, and returns run_harness's SAME
>   {ok,passed,total,output} contract. Self-test: translator unit checks + LIVE
>   prove of the town capstone 5/5 through the subprocess contract -- both green.
> - `fun_doc.py::process_port_candidate`: an OPT-IN (FUNDOC_LIVE_PROVE=1),
>   NON-FATAL live-prove step after the static harness passes -- never downgrades
>   a statically-proven candidate, only stamps `port_live_status` (proven_live /
>   live_prove_failed / unsupported_abi / error / skipped) + a bus event. Uses the
>   already-drafted `header` as the D2MOO reimpl (codebase-agnostic for pure
>   leaves). Both files py_compile clean.
>
> REMAINING to make it routine: a D2MOO drafting MODE in fun-doc (draft bodies +
> markers targeting the provider directly for non-trivial functions whose OpenD2
> header isn't drop-in), and the oracle ABI gaps (float/int64/struct/register-in
> beyond ECX/EDX, and pointer args referencing live game state for stateful fns).

9. **Marker convention in D2MOO** (`@PD2S12 D2Common!<name> @ 0x.. (conformance:
   STATE)`), + a `conformance_workbench`-style index over this repo. Seed with the
   proven coord + RNG fns.
10. **D2MOO port pipeline** (sibling of `port_pipeline.py`): worker drafts the
    equivalent, places it by subsystem (`source/D2Common/src/<Subsystem>/`),
    adds the marker, writes the registry row + provider export. Prove-oracle =
    live shadow (via WS-1/2/3) for stateful fns; static `/emulate_function` for
    pure fns (both -> PROVEN).
11. **Profiler hits -> fun-doc priority queue.** Export the ranked hit list so the
    worker documents/ports the hottest un-proven functions first.

### WS-5: D2Debugger MCP interface (the keystone for AUTONOMOUS proving)
> **STATUS: DONE + proven live (2026-07-06).** In-process localhost HTTP server
> (source/D2Debugger/src/D2Debugger.mcp.cpp, 127.0.0.1:8790) routed by
> D2Mcp_HandleRequest in D2Debugger.LiveDispatch.cpp; Python MCP bridge
> (conformance/d2debugger_mcp/server.py + README.md, 9 FastMCP tools). A
> non-elevated agent reaches the elevated game's loopback socket, so proving
> needs no human at the ImGui panel. Proven live via curl AND via the MCP bridge:
> GET /status, GET /dispatchers, GET/POST /dispatcher/{i}[/mode], POST
> /dispatchers/mode, POST /reimpl/reload (mem-loaded 5/5), GET
> /profiler/subsystems, POST /profiler/instrument (COLLISION 20/20), GET
> /profiler/functions/{sub}.
>
> DIRECT-CALL ORACLE (POST /call/{i}, single {x,y} or {vectors:[...]} batch)
> PROVEN: calls the raw original (new export D2MOO_LiveDispatch_GetTrampoline) +
> the bound reimpl (GetReimpl) with chosen inputs, returns per-vector +
> aggregate match verdict.
>   - positive: correct provider -> 6/6 allMatch (incl. negatives, INT16 extremes);
>   - NEGATIVE CONTROL: hot-reloaded a deliberately +1-buggy provider -> the oracle
>     flagged 0/4 on the buggy fn and 4/4 on an untouched fn (discriminating, not
>     globally failing); reverted -> back to 5/5. The full autonomous loop --
>     reload candidate -> oracle with chosen vectors -> precise verdict -- runs
>     with no human, driven end-to-end through the MCP bridge.
>
> To wire the bridge into an MCP host, see conformance/d2debugger_mcp/README.md.
> ABI note: the oracle marshals the coord family's void __stdcall(int*,int*)
> shape; arbitrary-ABI marshalling is design detail B (deferred, below).

Give D2Debugger an MCP interface so a MODEL can drive the live oracle directly --
mirror the exact pattern that already works for Ghidra (the Ghidra plugin exposes
HTTP on :8089; `ghidra-mcp` bridges it to MCP tools):

15. **In-process HTTP server in D2Debugger** (it already runs its own thread).
    Endpoints: `GET /profiler/functions` (fns + hit counts + subsystem),
    `GET /dispatchers` (modes/divergences/proof), `POST /dispatch/{fn}/mode`,
    `POST /reimpl/reload` (hot-reload provider), `GET /dispatch/{fn}/divergences`,
    `POST /instrument/{subsystem}`.
16. **A `d2debugger-mcp` bridge** wrapping that HTTP as MCP tools (sibling of
    `ghidra-mcp`). Localhost + cross-elevation TCP is fine (same as Ghidra).
17. **Two proof modes exposed to the model:**
    - **Passive shadow:** set mode=shadow, let gameplay exercise the fn, read
      divergences (real-usage distribution). Needs the fn to actually be called.
    - **Active direct-call oracle:** `POST /call/{fn}` invokes the function IN
      the live process with model-CHOSEN inputs, running BOTH the original (via
      trampoline) and the reimpl, and returns both outputs. The native,
      always-available version of the `behavioral/` Frida oracle -- decouples
      proving from gameplay, lets the model test edge/adversarial inputs, and --
      running in the LIVE process with real globals/state -- proves STATEFUL
      functions the static `/emulate_function` oracle cannot. **The single most
      powerful capability here, and it largely sidesteps WS-3** (strategy note
      below).

With WS-5 the model has a COMPLETE autonomous loop -- read the function (Ghidra
MCP) -> draft equivalent -> build provider DLL -> hot-reload + call/shadow via
D2Debugger MCP -> read divergences -> iterate -- with NO human clicking.

### WS-6: fun-doc worker integration
18. **Marker convention in D2MOO** (`@PD2S12 D2Common!<name> @ 0x.. (conformance:
    STATE)`) + a `conformance_workbench`-style index over this repo. Seed with the
    proven coord + RNG fns.
19. **D2MOO port pipeline** (sibling of `port_pipeline.py`): worker drafts the
    equivalent, places it by subsystem, adds the marker, writes the registry row +
    provider export. Prove via the WS-5 MCP oracle (direct-call for controlled/
    edge inputs; passive-shadow for real-usage breadth). Static `/emulate_function`
    stays a complementary check for pure fns.
20. **Profiler hits -> fun-doc priority queue** -- port the hottest un-proven fns first.

### WS-7: Ghidra documentation sync
21. As a function reaches shadow-clean, stamp Ghidra via MCP (`rename_function`,
    `set_plate_comment`, PORTED/PROVEN tags) with the D2MOO symbol + conformance
    status. A small registry<->Ghidra sync tool keeps the RE database and the
    conformance ledger aligned.

## Dependencies & recommended sequence

```
WS-1 (hot-reload, coord fns)  ─► proves the reimpl-provider mechanism
   │
   ├─► WS-5 (D2Debugger MCP + direct-call oracle)  ─► autonomous proving
   │        └─► WS-6 (fun-doc worker) ─► WS-7 (Ghidra sync)
   │
   ├─► WS-4 (graduation registry)  ─► the config both sides read
   ├─► WS-2 (generalized dispatcher + startup-from-config)  [hard: ABI stubs]
   └─► WS-3 (generic passive-shadow compare)  [hard; often OPTIONAL -- see note]
```

**Do WS-1 + WS-5 first, together.** WS-1 (hot-reload) is low-risk plumbing on top
of what works; WS-5 (MCP + direct-call oracle) is the capability that makes the
loop autonomous. That pair alone delivers "a model proves an equivalent on a real
function -- no restart, no human" -- the headline outcome.

## Design detail A -- reload-safety protocol (WS-1)

The reimpl-provider DLL exports functions the dispatchers call in Reimpl/Shadow.
Hot-reload = `FreeLibrary(old)` + `LoadLibrary(new)`. If a game thread is
executing INSIDE a provider function when we free it, the code is unmapped ->
crash. Protocol (a quiesce + drain, reference-counted):

1. **Quiesce.** `SetMode(all dispatchers, Original)` (atomic stores). No NEW call
   takes the reimpl path -- they route to the trampoline (real fn) instead.
2. **Drain.** A global atomic `g_reimplInFlight`. Every dispatcher wraps its
   reimpl invocation as `++g_reimplInFlight; reimpl(...); --g_reimplInFlight;`.
   The increment is UNCONDITIONAL on the reimpl path (never call without
   counting), so after step 1 no new increments start and existing calls drain.
   Reload waits (bounded timeout) until `g_reimplInFlight == 0`.
   - Race analysis: a thread may read mode=Shadow just before step 1, then
     increment + call. We then see the counter >0 and keep waiting -> safe. The
     only unsafe case (call without increment) can't happen because the
     increment precedes the call on that path. Nesting (a reimpl calling another
     hooked reimpl) balances via the counter.
   - Memory order: the mode stores must be visible before the counter read
     (seq_cst on both).
3. **Swap.** `FreeLibrary(old)`; `LoadLibrary(new)`; for each dispatcher
   `SetReimpl(i, GetProcAddress(new, symbol))`.
4. **Restore.** Reapply the prior modes (or leave Original for the model to
   re-enable via MCP).

Orchestration: D2Debugger owns the provider `LoadLibrary`. Bridge adds
`D2MOO_LiveDispatch_QuiesceForReload()` (does 1+2, returns when drained or
times out) and `D2MOO_LiveDispatch_SetReimpl(i, ptr)`; D2Debugger does the
Free/Load/re-resolve between them. If drain times out (a stuck reimpl), abort
the reload and report -- never Free under a live call.

## Design detail A2 -- reimpl dependency resolution (WS-1.5, feeds WS-2)

Once reimpls are non-trivial (stateful game logic), a reimpl CALLS other game
functions and reads globals. Two layers:

- **Layer 1 (already solved, loader-independent):** a call to a function B that
  has a DISPATCHER is intercepted at B's prologue (Detours), so ANY caller -- the
  game, or another reimpl -- routes through B's dispatcher and respects B's mode.
  The reentrancy guard forces nested calls to Original. So "which version of B
  does reimpl A get" is already answered for hooked functions.
- **Layer 2 (the real problem):** how does reimpl A *resolve the address* of a
  dependency it calls? It CANNOT import it the normal way: D2Common's export
  ordinals/names are SCRAMBLED (ORDINAL_RECONCILIATION.md), so a naive
  `import D2Common!Foo` resolves -- via the game's export table -- to the WRONG
  function. Dependencies MUST be resolved by VERIFIED ADDRESS (corrected_maps).

Two mechanisms, both valid:

1. **MemoryModule with a custom import resolver (recommended).**
   `MemoryLoadLibraryEx` accepts your own `getProcAddress`/`loadLibrary`
   callbacks used during the provider's import resolution. MemoryModule does the
   hard parts (map image, apply .reloc base relocations, TLS); OUR callback
   decides where each import points:
   - `D2Common!<name>` -> the **verified address** from corrected_maps (bypasses
     the scrambled export table), or
   - -> the **dispatcher** for that function (call respects its mode -- lets
     reimpls COMPOSE), or -> the **trampoline** (always-original -- ISOLATE).
   This is per-dependency routing policy, AND it eliminates temp files / the
   LoadLibrary path-cache bug in one move. Vendoring: MemoryModule is 2 files
   (`MemoryModule.c/.h`, MPL-2.0, x86-capable) added as a dependency -- NOT
   transcribed by hand (it's a PE loader). Caveat: base MemoryModule is weak on
   C++ exceptions/TLS; if reimpls need those, use MemoryModulePP.
2. **Dependency injection (simpler fallback).** The provider imports nothing from
   the game; the framework passes each reimpl a context struct of pre-resolved
   pointers (declared deps, resolved by verified address). Explicit, no loader,
   but the reimpl author threads the context through.

Both need the SAME foundation: a name -> VERIFIED ADDRESS resolver backed by
corrected_maps, exposed from the patch (`D2MOO_ResolveGameFn(name)`), rebased to
the game module. Build that first; it's what either mechanism calls.

## Design detail B -- direct-call ABI marshalling (WS-5)

> **STATUS: v1 DONE + proven live (2026-07-06).** General marshaller in
> source/D2Debugger/src/D2Debugger.oracle.cpp: `D2Oracle_Call(fn, cc, args[], n)`
> handles the x86 32-bit-slot case (every arg an int/uint/pointer slot) for
> cdecl / stdcall / fastcall / thiscall at arities 0..8, by casting `fn` to the
> exact function-pointer type per (conv, arity) so the COMPILER emits correct
> arg passing + stack cleanup (no hand-asm -> no stack-corruption risk). Endpoint
> `POST /oracle` (D2Debugger.LiveDispatch.cpp) is spec-driven: `{name, callconv,
> ret, args:[{id,kind:i32|buf,bytes}], compare:[ret|bufId...], vectors:[...]}`.
> It resolves the RAW original (trampoline if the fn is hooked, else its verified
> game address) and the provider's reimpl of the same name, marshals each vector
> through both with fresh scratch buffers, and diffs the requested channels.
> Proven: coord family via the general spec path -> 8/8 allMatch; negative
> control (compare a differing channel) -> flagged; unknown-name / bad-callconv
> -> clean errors. Driven from prove_candidate.py `--spec` and the MCP tool
> `d2dbg_prove_spec`. Example spec: conformance/vectors/coord_gametoclient.spec.json.
> **BATCH SHAKEOUT 2026-07-06 -- pipeline validated across varied ABIs, no bugs
> in the happy path; 2 genuine coverage gaps surfaced.** Proved live (no game
> restart -- provider-only rebuild + hot-reload): GetSeedHi (fastcall ptr->u32)
> 5/5, GetItemRandSeed (fastcall ptr->u32) 4/4, InitRngSeed (fastcall arity-2,
> ptr+scalar->void, buf-out) 3/3, SetCoordPair (fastcall arity-3, ptr+2 scalars
> ->void, buf-out + STACK arg) 4/4 -- plus town (stdcall scalar->int) and coords
> (stdcall+buf) already proven. Cross-checked meaningful (GetSeedHi=high vs
> GetItemRandSeed=low on the same seeds; SetCoordPair 0xFFFFFFFF|1<<32). Findings:
>   (1) NON-STANDARD REGISTER ABI: SEED_GetRandomNumber (0x510b0) takes `max` in
>       EAX (not the fastcall EDX) -> the marshaller can't express it. This is the
>       real motivation for an explicit-register-input call path (the RNG limited-
>       random family). Highest-value pure-ABI gap to close.
>   (2) int64 RETURN built but not yet live-validated: no clean STANDALONE
>       u64-returning target in this batch (the pure roll is inlined; the standalone
>       SEED_GetRandomNumber returns u32). Need a u64 target to close the loop.
> Process win: the whole batch iterated with NO UAC/restart -- add candidate ->
> prove_candidate.py --spec -> verdict, all against the running game.
>
> int64 RETURN (edx:eax) added 2026-07-06 via D2Oracle_Call64 (ret:"i64"/"u64" ->
> compiler captures edx:eax, no asm); built clean; still needs a STANDALONE
> u64-returning target to live-validate (the pure roll is inlined).
>
> REGISTER-EXPLICIT INPUT (custom-ABI) added + PROVEN LIVE 2026-07-06. D2Oracle_
> CallRegs (naked-asm stub, D2Debugger.oracle.cpp) sets all 6 GP regs from io[]
> then captures them; the /oracle spec's `orig_regs` maps register->arg-id for
> the ORIGINAL, while the reimpl stays a normal-convention C++ fn (no asm shim ->
> fun-doc can auto-generate it). port_live_prove.translate_layout_to_spec now
> emits orig_regs for non-standard register-only layouts. PROVEN: SEED_GetRandom
> Number (0x510b0, takes `max` in EAX) 6/6 live across all branches (max<1, pow2
> &, non-pow2 %) + in-place seed mutation; game unharmed. Closes shakeout finding
> #1. Candidate: candidates/seed_getrandom.cpp; spec: vectors/seed_getrandom.spec.json.
>
> STATEFUL rung 1 (REAL LOADED DATA) PROVEN LIVE 2026-07-06 -- the live oracle's
> UNIQUE value over static /emulate_function. GetDataTableRowEntryCount (0xa0b0)
> scans the real global g_dwDataTableBase; the reimpl reads the SAME live global
> by verified address (0x6fdee2cc; fixed base, no ASLR) so both see loaded data.
> 15/15 across rows, VARIED counts [113,135,116,66,150,1,5,1,5,0,0,0,0,0,150] --
> genuine data dependence static emulation can't reproduce. Candidate:
> candidates/datatable_rowcount.cpp; spec: vectors/datatable_rowcount.spec.json.
> TWO findings: (a) Ghidra's decompiler MISLABELED the ABI __fastcall, but the
> disassembly (MOVZX [ESP+4], RET 0x4) is __stdcall -- ASM is ground truth; the
> pipeline must take ABI from disassembly, not the decompiler annotation. (b) the
> wrong-ABI first attempt did a wild read -> the oracle's SEH guard caught it with
> ZERO game impact (robustness validated). A production reimpl would read its own
> g_dwDataTableBase via a DATA-import resolver (successor to WS-1.5) instead of a
> hardcoded address.
>
> STATEFUL rung 2 (DATA-IMPORT RESOLVER) DONE + proven live 2026-07-07 -- reimpls
> now read REAL game globals BY NAME, no hardcoded addresses. Mechanism =
> DEPENDENCY INJECTION (avoids PE import-table/.def complexity): D2Debugger calls
> the provider's D2MOO_Provider_Init(resolve) after MemoryLoadLibraryEx, passing
> D2MOO_ResolveGameFn; reimpls call D2MOO_Resolve("name") (provider_runtime.h).
> Data globals added to the resolve table via gen_resolve_table.py DATA_GLOBALS
> (g_dwDataTableBase @0x6fdee2cc, g_anTownLevelIds @0x6fde4084). Proven:
> GetDataTableRowEntryCount reimpl reads g_dwDataTableBase BY NAME -> 15/15 live
> (real counts, not the failure sentinel). FIRST real exercise of the WS-1.5
> verified-address resolver by a reimpl. Files: provider_runtime.{h,cpp},
> candidates/datatable_rowcount.cpp.
>
> STATEFUL rung 3 (LIVE GAME OBJECT pointer args) -- MECHANISM BUILT 2026-07-07,
> capture-target discovery PENDING. Built + running: a live-handle CAPTURE hook
> (D2Debugger.capture.cpp, Detours grab of [esp+4] into g_capturedUnit), an oracle
> arg kind "handle" (substitutes the captured pointer, guarded if none captured),
> sub-dword return masking (ret "u8"/"i8"/"u16" -> RetMask, since byte/short
> returns leave stale upper EAX bits), /status capturedHandle+captureCount, and a
> UNIT_GetMode reimpl (candidates/unit_getmode.cpp; __stdcall(UnitAny*)->byte@+0x90).
> KEY INSIGHT: orig+reimpl read the SAME captured pointer back-to-back, so they
> agree regardless of which unit/value -> a valid readable handle is all the proof
> needs; SEH guards a stale pointer (no game crash).
> SNAG: the capture target UNIT_GetMode (0x34870) is COLD -- D2 inlines that
> accessor, so the standalone export ~never runs (captureCount stayed 0 in-game +
> moving). Need a HOT unit function. Clean fix = INTEGRATE capture into the
> profiler stub path (one Detours hook that both counts AND stashes arg0), so we
> instrument a subsystem, read the hottest unit-taking fn, and capture from it with
> no double-hook. Then prove UNIT_GetMode (or any getter) with the live handle.
>
> Still deferred: float/double/int64/struct-by-value ARGS, >8 stack args,
> register-explicit WITH stack args (v1 register-only), and the FINAL prize -- the
> game-thread-affinity call for functions that MUTATE shared state (game-thread
> call queue), building on the live-handle capture above.

`POST /call/{fn}` must invoke an arbitrary-signature function twice (original via
the dispatcher trampoline; reimpl via the provider pointer) with model-chosen
inputs, then diff outputs. Generic outbound calling across D2's ABIs is the same
core problem as WS-2's inbound stubs -- share ONE signature descriptor:

- **Signature descriptor** (in the registry, from Ghidra decompilation): return
  type, ordered args (type + ABI slot: stack / ECX / EDX / EAX / ESI / EDI), and
  which args are out-pointers (the write-set to read back).
- **Marshaller v1 = a per-SHAPE library, not per-function.** Start with the ABI
  shapes the first targets use (coord: `void __stdcall(int*,int*)`; RNG: the
  three register ABIs already proven in `behavioral/pd2_frida_rng_family.js`).
  Each shape: parse JSON inputs -> typed args -> set stack/registers (asm stub
  for register ABIs) -> call -> read outputs -> JSON. New shapes are added as the
  worker meets new signatures; this is the same growing ABI-core WS-2 needs.
- **Inputs incl. pointers:** value args come from JSON; pointer args either (a)
  point at a caller-provided buffer built from JSON, or (b) reference REAL live
  game state (a game-object handle the model passes) -- (b) is what makes the
  in-process oracle able to prove stateful fns with authentic state.
- **Both sides, same inputs:** call original(trampoline) and reimpl(provider)
  with independent copies of the inputs (so out-params don't cross-contaminate),
  compare return + all out-pointer write-sets. This IS the shadow comparison,
  but model-driven and input-controlled instead of gameplay-driven.

WS-2 (inbound stubs) and WS-5's marshaller (outbound calls) should be built on
the shared descriptor + a shared per-shape ABI core, so ABI work is done once.

## The honest hard parts -- and how the direct-call oracle shrinks them

- **Generic ABI dispatch stubs (WS-2)** -- still real work: to INTERCEPT arbitrary
  live calls you need generated stubs per calling convention. Needed for
  passive-shadow at scale and for many-function graduation.
- **Generic passive-shadow snapshot/compare (WS-3)** -- **the direct-call oracle
  (WS-5) makes this largely OPTIONAL.** Instead of snapshotting arbitrary live
  read/write sets mid-gameplay and diffing them, the model calls the function with
  controlled inputs and compares the returned outputs -- far more tractable per
  function, and still in-process so real globals/state are present. Passive-shadow
  stays valuable for *real-usage breadth* (inputs the model wouldn't think to try),
  but it's no longer the *blocking* correctness mechanism it looked like. **This is
  the key strategic shift:** proving moves from "instrument + snapshot arbitrary
  state" to "call both sides with chosen inputs and compare" -- exactly what the
  MCP oracle enables.
