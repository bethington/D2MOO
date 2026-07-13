# PD2 in-memory patch map (on-disk vs. mapped-image diff)

Snapshot of every in-memory edit Project Diablo 2 (and its bundled mods) applies to
the original Diablo II game binaries at runtime. Produced by reading each mapped
module out of the live `Game.exe` process and diffing it against its on-disk file,
relocation-aware and ignoring the loader-filled import table.

Captured 2026-07-12 against the D2MOO conformance rig (PD2 stack + D2.Detours +
D2Debugger). PD2 loads `D2Common.dll` at its preferred base `0x6FD50000` with zero
relocation, so every `.text` difference is a genuine edit, not a loader artifact.

## Key results

- **1,637 clean-PD2 patch sites across 18 game DLLs** (`Storm.dll`, `D2CMP.dll` untouched).
- Dominant patcher is **ProjectDiablo.dll** (reference redirection + in-place logic edits);
  **BH.dll** (maphack) hits D2Client/D2Multi; **SGD2FreeRes / SGD2FreeDisplayFix** (HD)
  hit the render DLLs; **PD2_EXT.dll** hits Game.exe/Fog.
- The **D2MOO harness (reimpl D2Common + D2Debugger) touches only D2Common** (57 sites),
  and **0 of them overlap a PD2 edit** — so the clean-PD2 set below is complete and
  free of harness masking, without needing a separate clean-client launch.
- Heaviest: D2Game.dll (798), D2Client.dll (597), D2Common.dll (128).

## Files

| File | Contents |
|---|---|
| `clean_pd2_patch_sites.csv` | The 1,637 PD2-only patch sites (dll, rva, ordinal, type, patcher, target). **Start here.** |
| `all_patch_sites.csv` | All 1,708 sites including the D2MOO/D2Debugger harness rows, for comparison. |
| `pd2_d2common_memdiff.csv` | D2Common broken out per-function with names (from the D2Debugger function DB). |
| `diff_rig.json` | Full raw per-DLL detail: module map, per-run bytes, sections, base/reloc. |
| `perdll_rollup.json` | Per-DLL attribution rollup (PD2 / harness / logic / other). |
| `tools/` | The analysis pipeline scripts + their intermediate JSON/logs. |

## Reproducing

The scripts in `tools/` read the live process via `ReadProcessMemory` and need an
**elevated** Python (Game.exe runs at high integrity). They hardcode the PID and
absolute temp paths from the capture session, so they are a record of method rather
than turnkey — update `PID` and paths before re-running. Pipeline order:

1. `gendiff.py <pid> <label>` — read all mapped game DLLs, diff vs on-disk, classify `.text` edits.
2. `reattrib.py` — offline re-attribution of each edit to its patcher module (abs-ptr + rel32).
3. `final_clean.py` — masking-overlap check + clean-PD2 per-DLL tables and CSV.

`memdiff.py` / `mapdiffs.py` / `attribute.py` are the single-DLL (D2Common) first pass;
`modmap.py` dumps the process module map; `vq.py` classifies a target memory region.

## On-disk modifications (static file diff vs. vanilla 1.13c)

Separate from the in-memory diff: comparing PD2's shipped `C:\Diablo2\ProjectD2\*`
against a clean non-PD2 1.13c base (`C:\Diablo2Project` root, verified free of PD2
markers). See `ondisk_vs_vanilla_1.13c.csv`.

**PD2 modifies exactly two binaries on disk — both are load-time bootstraps:**

- **Storm.dll** (20 bytes): its import-table string `VERSION` is overwritten in place
  with `PD2_EXT` (same length), so the loader pulls in `PD2_EXT.dll` instead of
  `VERSION.dll` when `Game.exe` statically imports Storm.
- **D2Win.dll** (247 bytes, 52 in `.text`): a code stub + the string `ProjectDiablo`
  are patched in to `LoadLibrary` the mod core `ProjectDiablo.dll`.

**All 19 other classic binaries — including D2Common, D2Game, D2Client — are
byte-identical to vanilla 1.13c.** PD2 does *no* gameplay patching on disk; every
gameplay change is applied in memory at runtime by `ProjectDiablo.dll` (the 1,637
sites above). The two on-disk edits are purely the injection bootstrap.

Methodology note: an initial pass used `C:\Diablo2\output_v2` as the "vanilla"
reference and wrongly flagged ~15 DLLs as modified. That set is a D2MOO *transplant*
output (it flips the `.reloc` section's `MEM_WRITE` bit and zeroes some `.rdata`), so
it is NOT pristine. A 3-way comparison (PD2 vs output_v2 vs Diablo2Project) plus the
`.reloc` characteristics byte exposed the contamination; `Diablo2Project` root is the
correct clean base.

## Load / injection chain

1. Launch `Game.exe` (unmodified vanilla) — statically imports Storm.dll, D2Win.dll, etc.
2. PD2's patched **Storm.dll** imports `PD2_EXT.dll` (via the hijacked VERSION slot) → loads early.
3. PD2's patched **D2Win.dll** `LoadLibrary`s `ProjectDiablo.dll` (the mod core).
4. `ProjectDiablo.dll` loads `BH.dll` (maphack) + `SGD2FreeRes`/`SGD2FreeDisplayFix` (HD)
   and applies all in-memory patches to D2Common/D2Game/D2Client/etc.

`Game.exe` itself is NOT modified and does not import any PD2 DLL — the mod rides in
on the two patched game DLLs. (This differs from the conformance rig, where
`D2.DetoursLauncher` wraps `Game.exe` to *additionally* inject the D2MOO stack.)

## Files (added)

| File | Contents |
|---|---|
| `ondisk_vs_vanilla_1.13c.csv` | Per-binary static file verdict: identical vs modified, byte-diff count, PD2 markers. |

## Patch types

- `JMP-detour` (`E9`) — whole function redirected into a patcher module.
- `CALL-patch` (`E8`) — a call site redirected.
- `inline-edit` — an operand rewritten (rel32 branch or abs pointer) to point into a patcher.
- `NOP-out` / `partial-NOP` — original code removed.
- `logic-*` — in-place byte edits with no external target (conditional flips, immediates).
