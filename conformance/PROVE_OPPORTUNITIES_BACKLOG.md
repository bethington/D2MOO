# Prove Pipeline — Opportunities Backlog

Living list from the 2026-07-14/15 self-improving Prove loop session. Update as items are done or superseded. See also: `D2COMMON_FULL_SHADOW_PLAN.md`, `GRADUATED_CONFORMANCE_PIPELINE_PLAN.md`, `CONF_REGRESSION_OFFLINE_SUITE.md`.

## Pool snapshot (2026-07-15, D2Common.dll, from fun-doc state)

| port_status | count |
|---|---|
| never_attempted | 1808 |
| stateful_skip | 357 |
| shadow_leaf_pending | 108 |
| proven_live_pending_review | 56 |
| malformed_response | 53 |
| live_prove_failed | 43 |
| no_vectors | 30 |
| shadow_unreachable | 23 |
| proven_pending_review | 11 |
| harness_failed | 7 |
| oracle_unavailable | 3 |
| error | 2 |
| unsupported_abi | 2 |
| proven_live | 2 |
| handle_abort_hazard_skip | 2 |

## Ranked opportunities

### Immediate / high-yield
1. **[DONE 2026-07-15] Re-queue pre-fix casualties** — `malformed_response` (53) and `harness_failed` (7) were stamped terminal before today's parser fixes (hex-JSON leniency in `json_loads_lenient`, split-fence reasoning heal, `<cinttypes>` scaffolding). Many are provable functions killed by bugs that no longer exist (proof of concept: `SafeDereferencePointer` failed→fixed→proved 20/20). Reset these to eligible and let the loop re-attempt with the fixed pipeline. `no_vectors` (30) is NOT blanket-reset — most are the genuine stack-arg/pointer-param static-emulator limitation, not a bug we fixed.
2. **[DONE 2026-07-15] Load a character into Single Player** — loaded `summoner-skele` (Necro) Normal via d2dbg MCP; handle-prove now active for the `shadow_leaf_pending` pool.

### Medium effort / structural yield
3. **[DONE 2026-07-15] Safe shadow re-deploy** — see [[shadow-multiarg-thunk-crash]]. REBUILT with 97 safe dispatchers (84 1-arg class-A + 3 B + 10 D; the 12 risky multi-arg/0-arg entries moved to `shadow_batch_pending.json` so they're not even in the build → can't be armed accidentally). Deployed clean (102 total incl. 5 coord family), armed ALL to shadow in 6 batches of 17 with live-checks between each — **game survived every batch** (vs the all-at-once crash on 2026-07-14). 63/102 exercised so far, **ALL 63 zero-divergence**. Now accumulating battletest hits during gameplay; battletest_promoter promotes at 1000+ hits / 0 divergence → CONF_BATTLETESTED.
4. **Review/promotion backlog** — 67 functions (`proven_live_pending_review` 56 + `proven_pending_review` 11) sit awaiting promotion to `Shared/`/the CONF_LIVE ledger. Define what "review" means (spot-check sample? auto-promote strong synth proofs above N vectors?) before this pile grows unmanageable.
5. **Route static `no_vectors` failures to live-prove fallback** — the stack-arg emulator gap keeps recurring in the static harness, but the LIVE oracle supports stack args. A fallback path (`no_vectors` in static lane → retry via live-prove when oracle is up) converts a known dead-end into proofs without touching the emulator itself.

### Loop/pipeline efficiency
6. **[DONE 2026-07-15] Pre-draft unresolvable-global check ordering** — handle lane now gates on (`static_abi.data_globals` present AND `resolvable_globals` empty) → `unresolvable_global_predraft_defer` to shadow-first BEFORE drafting. (Live/global_leaf lane intentionally left alone — its mechanical global-table translator has its own resolution.) Also DONE same day: provider timeouts now REFUND the draft-attempt budget (capped) across all 3 draft lanes, so a slow-provider blip no longer burns a function to `malformed_response` (iter23 lesson).
7. **[DONE 2026-07-15] Raise loop batch size** — bumped 2→5.

### New (found by the loop)
12. **[NEXT] HTTP worker-launch endpoint** — the loop launches workers via socket.io `request_start_worker` (the only trigger path today). Repeated short-lived socketio.Client connect/disconnect cycles degraded the server's socket.io mid-session (2026-07-15: polling 200s but namespace connect hung; fixed by server restart). Add an HTTP POST route (e.g. `/api/worker/start`) that calls `worker_mgr.start_worker(...)` directly, so autonomous launches don't depend on socket.io stability. Small, high-value for unattended running.

11. **Live-lane partial-resolvable-globals waste** — found 2026-07-15 iter26: `DATATBLS_GetLvlTypeField2C` (live/global_leaf lane) has SOME resolvable globals but also needs `g_pLvlPrestData` which isn't in the resolve table. The pre-draft gate (#6) requires ALL globals unresolvable, so it doesn't catch this; the resolve-name gate then retries 3× (model re-invents the name) → `malformed_response`. Fix idea: in the live lane, if the resolve-name gate keeps returning the SAME unknown name across retries AND that name maps to a real `data_globals` address with no resolve-table entry, defer to shadow-first instead of burning the retry budget. Low frequency; nuanced (don't disturb the mechanical global-table translator). Watch-listed, not blocking.

13. **[CRITICAL] Provider-reload vs armed-shadow mutual exclusion** — 2026-07-15: resuming the prove loop (which hot-reloads the provider for live-prove) while 102 dispatchers were armed to shadow DEADLOCKED the D2Debugger oracle (:8790 hung, game stayed alive). The reload drain-to-zero can't complete while shadow reimpls are constantly in-flight. FIX: before `reload_provider`, force all dispatchers to Original + drain, reload, then restore shadow modes — OR make reload_provider refuse while any dispatcher is in shadow. Until fixed, run EITHER the prove loop OR armed-shadow battletest, never both. See [[shadow-provreload-deadlock]] memory.

14. **Lost-candidate proofs (18 fns)** — found 2026-07-15 during the Ghidra write-back pass: addresses whose LATEST registry proof has no reimpl source anywhere in `reimpl_provider/` (candidate written at prove time, never committed, later cleaned). Proof facts survive in the registry + CONFORMANCE bookmarks (reimpl field honestly omitted), but the code can't be shown or re-frozen. Mostly 2026-07-08/09 CONF_LIVE item/overlay getters (full names, `conformance_doctor.py` greps this list to distinguish known casualties from NEW losses): `UNIT_GetPlayerData`, `SKILLS_GetSkillTargetGUID`, `SKILLS_GetBaseManaCost`, `INV_SetInventoryField24`, `ITEMS_GetItemDexBonus`, `ITEMS_GetItemStrBonus`, `ITEMS_GetItemRangeAdder`, `ITEMS_GetItemRecordField13E`, `ITEMS_GetItemDataItemCell`, `ITEMS_GetItemDataInvPage`, `ITEMS_GetItemDataBodyLoc`, `ITEMS_GetItemDataRareSuffix`, `ITEMS_GetItemDataRarePrefix`, `ITEMS_GetItemDataAutoAffix`, `DATATBLS_GetItemTypeQuiver`, `DATATBLS_GetItemTypeThrowable`, `DATATBLS_GetOverlayTrans`, `DATATBLS_GetOverlayPreDraw`. These are simple getters — cheapest fix is re-drafting through today's (much better) pipeline; candidates now get committed.

### Larger builds (scoped, not started)
8. **Class-D-v2 multi-register shadow stubs** — `gen_shadow_dispatch.py` only supports 1-arg register-explicit (class D) and false-safe multi-arg class A (the crash cause). 21 drafted functions wait in `shadow_batch_pending.json`. Also the proper fix for safe bulk shadow arming (fault-rate auto-disarm).
9. **Resolve-table extension with verified internal (non-exported) addresses** — `gen_resolve_table.py` only covers the exports TSV. Extending it to verified internal function addresses would return the ~15+ `unresolvable_delegate` class to direct-prove (no shadow/gameplay needed).
10. **Finish the sectional-draft prototype** — `scripts/scripted_agentic_draft.py` (scratchpad prototype) proved the skeleton→fill→splice→compile mechanics work for functions too large for one-shot MiniMax generation, but fill prompts need the verified-resolve-names list injected (one drafted with an invented `D2MOO_GLOBAL_FILTER_GATE`). Needed for the big-function tail (e.g. `STATS_GetTotalStatWithModifiers`-class).

11. **Offset-suffixed table-tester family: 8+ failures, one shared root cause (2026-07-18 ~01:00–02:05)** — `STATS_TestBitfieldTable_{148,144,FC,DC,D0}` (6× `[mismatch]`) and `MISSILES_TestTblMaskField_{154,14C}` (2× `[build_provider]`) at `6fd83a00–6fd83d80`, while interleaved neighbors proved clean. The MiniMax debug transcript for `_14C` shows the shared confusion explicitly: **the `g_pDataTables + 0x<offset>` single-vs-double dereference convention** (decompiler shows the VALUE of the struct-base pointer plus offset; the drafts waver between treating `g_pDataTables` as the base vs a pointer-to-base). Wrong-but-compiling guesses → [mismatch]; malformed attempts → [build_provider]. Fix: settle the convention once (check one member against D2MOO's `sgptDataTables` access pattern), inject it into the port prompt as a rule, and re-queue the family together (~a dozen functions).

12. **[DONE 2026-07-19] MiniMax 429 misclassified as malformed/no_change (2026-07-18 ~02:52 incident)** — account hit rate limit/quota; every call returned HTTP 429. Root cause found: `_invoke_minimax` SWALLOWED the terminal exception after its 4-attempt retry loop (print + break), so no `provider_error` reached meta and the post-call `detect_quota_wall` never ran — the lanes' existing `quota_paused` routing was fine, it just never got the signal. Fixed in ghidra-mcp `3a3696d` (branch conformance/live-bookmark-scope): terminal error string + `status_code` now land in `meta[provider_error]/[provider_http_status]`, and `_detect_minimax` gives duration-less 429s a 600s fallback pause (safe: that detector only ever sees terminal errors). Incident window was wider than first estimated — 02:44–03:15, 13 PORT functions (not 10; all verified 0-in/0-out on every attempt) reset to eligible in state.db with `port_last_result='requeued: 429 quota-wall casualty'`. The 38 zero-token `no_change` GLOBALS runs need no reset — `no_change` is never clean-cached, so they stay naturally eligible; pollution was runs.jsonl-only. NOTE: 5 of the 13 (`MISSILES_TestTblMaskField_{124,E4,E0,DC}`, `DRLG_CheckBitFlagsOverlapByDataTable`) sit in the #11 offset-suffixed table-tester family — expect them to need #11's deref-convention prompt rule to actually prove.

## Session history
- 2026-07-14/15: parser/salvage/routing fixes (malformed-response near-zero), shadow-batch triage (external-import lesson), shadow deploy→crash→revert, self-improving Prove loop (24+ iterations, 2 pipeline bugs found+fixed, 11+ proofs including 2 live).
