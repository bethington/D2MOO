# PD2-S12 export maps — ordinal ↔ address ↔ candidate names (all 11 D2MOO modules)

One `.tsv` per DLL: `ordinal · address · ghidra_name · def_name · names_agree`.

## The one thing that is ground truth

**`ordinal → address`** — parsed straight from each DLL's PE export directory
(`tools/gen_corrected_map.py`, run against `C:\Diablo2\ProjectD2\*.dll`). This is
always correct and is what a `D2.Detours` drop-in must resolve targets by.

## The two NAME columns are both fallible — in opposite ways

This started as "generate a *corrected* name↔ordinal map," but verification showed
there is **no single trustworthy name source**. Behavioral spot-checks (decompiling
the real function and reading what it *does/calls*, not its label) found:

| Binary | `.def` ordinals | Ghidra names | Evidence |
|--------|-----------------|--------------|----------|
| **D2Common** | ❌ **scrambled** | ✅ correct | `.def GameTileToClient@10110` is really `ITEMS_CheckItemTypeFilter`; real `GameTileToClient` is `@10375` (8+ verified) |
| **D2Game** | ❌ **scrambled** | ✅ correct | `.def @10016 GetPlayerUnitsCount` is really `InitializePerformanceFrequency`; `@10014/@10020/@10023` are stubs (7/8 verified) |
| **Storm** | ✅ **correct** | ❌ misidentified | `.def SNetCreateGame@101` is right (decompile: `STORM\SNET.CPP`, wraps `SNetCreateLadderGame`); Ghidra mislabels it `NET_InitializeGameStateWrapper` |
| **Fog** | ✅ **correct** (sampled) | ❌ misidentified | `.def FOG_SOCKET_CloseSocket@10000` is right (body calls `closesocket()`); Ghidra mislabels it `ProcessInventoryItemWrapper`. `@10002 WSACleanup` agrees. |
| **D2CMP, D2Gfx, D2Lang, D2MCPClient, D2Net, D2Sound, D2Win** | ⚠️ **unverified** | ⚠️ unverified | mixed; `D2Win@10000` `.def CreateWindow` vs real add-text-to-textbox looks scrambled, but not enough samples to call the module |

**Pattern (hypothesis):** the **game-logic** DLLs (D2Common/D2Game) were re-linked
with a *shuffled* export order, so the `.def` (standard 1.13c ordinals) is wrong there
— but Ghidra's analysts named those well. The **support** DLLs (Storm/Fog, standard
Blizzard/OS-abstraction ordinals) kept their ordinals, so the `.def` is right, but the
analysts mislabeled them. Everything else is unverified.

## `names_agree` is only a naive substring check

`yes`/`no` compares the two labels textually. It is **not** a correctness verdict:
Storm/Fog show `no` on rows where the `.def` is actually right (Ghidra's label just
differs), and it would show `yes` on a coincidental string overlap. Use it to *find
disagreements to investigate*, nothing more. (D2Lang's 35 `yes` rows are mostly where
the `.def` still uses generic `D2LANG_NNNNN` placeholders — not real agreement.)

## How to use these

- **Drop-in wiring:** resolve by `ordinal → address`. Always safe.
- **Need the true name at an ordinal:** take `ghidra_name` for D2Common/D2Game,
  `def_name` for Storm/Fog; for the rest, decompile and confirm before trusting either.
- **Regenerate:** `python tools/gen_corrected_map.py <ghidra_prog> <dll> <def> <out.tsv>`
  (needs the Ghidra plugin on `:8089` with the program open).

## Two follow-ups this enables

1. **Fix Ghidra's names for the support DLLs** — Storm/Fog `.def` names are the
   authoritative standard labels; push them back into the Ghidra DB to correct the
   misidentifications (`NET_InitializeGameStateWrapper` → `SNetCreateGame`, etc.).
2. **Fix the `.def` ordinals for the game DLLs** — regenerate D2Common/D2Game `.def`
   from the real PE ordinals + Ghidra names so the drop-in can trust it.
