# Game.exe behavioural tracer (CONF_TRACE) — **working**

**Do not run this while a real Diablo II is open.** Diablo II permits one
instance at a time; a traced launch trips that check and throws a modal
*"Only one copy of Diablo II may run at a time"* dialog on the operator's
screen. Close the game first — and note that "closed" often is not: a wedged
`Game.exe` survives with **no main window**, still `Responding`, and cannot
be killed normally because of the deny-all DACL below. Check for it, and
clear it through the elevated dashboard, which needs no UAC prompt:

```
POST http://127.0.0.1:5000/api/oracle/kill   {"confirm": true}
```

90 hooks installed, one benign failure (`KERNEL32!GetCurrentProcess` — a
two-instruction pseudo-handle stub Frida cannot intercept; nothing depends
on tracing it).

## What a clean run captures (verified 2026-08-08)

244 calls, with arguments, covering **five** of the eighteen launcher
functions — and every one corroborates the static analysis independently:

| Observed | Confirms |
| --- | --- |
| `OpenSCManagerA` → `0` | `GAME_TryStartAsWindowsService` failing, as WinMain's service check expects |
| `RegOpenKeyA(HKLM, "…\Diablo II Beta")` → `2` | `GAME_MigrateBetaRegistryKeys` taking its not-found path, and `REG_PATH_BETA` is right |
| `LoadLibraryA("advapi32.dll")`, then `GetProcAddress` for `AllocateAndInitializeSid`, `InitializeAcl`, `AddAccessDeniedAce`, `SetSecurityInfo` | `ApplyProcessSecurityRestrictions` @ 0x408120, in exactly the order the decompilation describes |
| **57** `GetPrivateProfileIntA`/`StringA` reads, first `VIDEO/WINDOW`, last `DEBUG/SOUNDBKG` | `ParseCmdLine` walking `gaCmdArguments` — and the table size derived independently from the disassembly as `0xd5c / 0x3c = 57` |
| `RegOpenKeyExA(HKCU, "…\VideoConfig")`, `RegQueryValueExA("Render")`, `RegCloseKey` | the video-config block of `GameInit` |

The 57 ini reads are the strongest signal: the first and last section/key
pairs match the first and last entries of D2MOO's `gaCmdArguments` table
exactly, including its idiosyncratic `"QuEsTs"` casing. Two entirely
independent routes — byte-level reconstruction and live behaviour — agree on
the same table.

## Known gaps

* **The handoff is not reached.** After the video-config read, `GameInit`
  calls `GameStart`, which goes straight into `Fog.dll` for archive loading.
  Those are Fog's imports, not Game.exe's, so the return-address filter
  correctly discards them and the trace simply ends. Reaching
  `GetProcAddress("QueryInterface")` needs archive loading to succeed in the
  sandbox; not yet investigated.
* **Sandbox depth changes behaviour.** `GetD2IniPath` walks up two
  directories from the working directory, so in the sandbox the ini resolves
  to `C:\tmp\D2.ini` rather than the install's own. Harmless for an
  original-vs-ours diff (both run in the same sandbox) but it means a trace
  is not directly comparable to one taken from the real install.
* The `CreateEventA` isolation added to avoid closing the live game
  **does not work** — `0` events isolated, so the single-instance check does
  not reach it on this path. Left in place (harmless, and it logs what it
  does) but it is not a solution.


Only 4 of Game.exe's 18 launcher functions can be proven by byte identity.
The other 14 need behavioural evidence, and because the launcher's code runs
**once** — or, on error paths, never — that evidence has to come from
*forcing* the rare paths and diffing every observable side effect between
the original binary and ours.

| Piece | State |
| --- | --- |
| `agent.js` — Frida agent, hooks Game.exe's imports, detects handoff | enumerates imports; **attaches none** (see below) |
| `run_trace.py` — sandbox, fault injection, spawn/inject/collect | runs end to end; produces a trace file |
| `trace_diff.py` — normalise + diff + verdict | written, not yet exercised on real data |

## Two blockers found by running it, not by reading it

### 1. Game.exe MAKES ITSELF UNKILLABLE (not elevation — a DENY-ALL DACL)

A traced run left a `Game.exe` that `Stop-Process` refused with *Access is
denied* and whose `ExecutablePath` read back empty. The obvious reading —
"it relaunched itself elevated" — was **wrong**, and worth recording as a
wrong turn because every surface symptom pointed at it.

The evidence against elevation: `Game.exe` has **no manifest** at all (no
`requestedExecutionLevel`), there is **no AppCompat `RUNASADMIN` layer** for
the sandbox path, and UAC is at its normal setting. Nothing would elevate it.

The actual cause is one of our own four byte-match targets.
`ApplyProcessSecurityRestrictions` @ **0x00408120** loads `advapi32`, then
`AllocateAndInitializeSid` → `InitializeAcl` → `AddAccessDeniedAce` with
access mask **0xF01FFFFE (DENY_ALL)** → **`SetSecurityInfo` on its own
process handle**. Game.exe deliberately applies a deny-everyone DACL to
itself. Same user, same integrity level — the process simply refuses to be
opened, killed, or queried.

That also explains why the *first* crashed run left a killable process and
the second did not: the first died before `device.resume`, so the DACL was
never applied.

Handled in the agent by `neutraliseSelfProtection` (default on): the
`SetSecurityInfo` call is **logged normally** — making it is part of the
behaviour being verified — and then defused by zeroing its
`SECURITY_INFORMATION` mask, so it applies nothing and returns success. Both
binaries are traced identically, so the comparison stays fair, and the count
of neutralisations is reported with the handoff rather than hidden.

To clear a stray from before this fix you need an **elevated** shell (an
administrator holds `SeDebugPrivilege`, which bypasses the DACL):

```powershell
Stop-Process -Id <pid> -Force     # from an elevated prompt
```

Check for strays with:

```powershell
Get-CimInstance Win32_Process -Filter "Name='Game.exe'" |
  Select-Object ProcessId, ExecutablePath, CreationDate
```

Distinguish yours from a real session by `CreationDate` and by
`ExecutablePath` pointing into the sandbox — a live PD2 shows no path
(elevated) and a much older creation time.

### 2. `hooked: 0` — imports enumerate but nothing attaches — FIXED

`main.enumerateImports()` returns **85** records for Game.exe and the attach
loop skipped every one on its `if (!imp.address)` guard. Probing the record
shape (rather than guessing) showed why: under Frida 17 the records carry
only `{type, name, module, slot}` — **no `address` field at all** — and the
`slot` values repeat, so neither field can be attached to directly.

Fixed by resolving each API **by name in its owning DLL** and keeping only
calls whose **return address lies inside Game.exe**. That is not a
workaround, it is a strictly better instrument: it also catches calls made
through `GetProcAddress`-resolved pointers, which never touch the IAT and
which import hooking would silently miss. Game.exe does exactly that in two
places that matter — its handoff, and the whole self-protection DACL — so an
IAT-only tracer would have been blind to both. Dynamically-resolved APIs are
covered by an explicit `EXTRA` list for the same reason.

The guard stayed. Attaching to a null address would fail or corrupt the
process, and a tracer that hooks fewer imports than it reports would make
two different binaries look identical for the dullest possible reason. The
`ready` message reports `imports`, `candidates`, `hooked`, `failed` and
`skipped` separately precisely so this could not pass silently — and it did
not.

## Design that is settled

* **Import table = observable surface.** Game.exe reaches the world through
  35 import thunks (registry, file, LoadLibrary, CreateProcess, window), so
  hooking imports is complete rather than best-effort.
* **Stop at handoff.** Game.exe's job ends when it resolves the loaded
  module's `QueryInterface` export (`PROC_QUERYINT` in D2MOO's `Main.h`) and
  hands off. Tracing past that diffs D2Client, not Game.exe. Runs stay at
  seconds and need no display.
* **Sandbox = throwaway copy of the PD2 install.** ~2 GB; the bulk `.mpq`
  files are hard-linked rather than copied so the fault matrix stays fast
  enough to actually run. **A directory copy does not isolate the
  REGISTRY** — `HKLM/HKCU\Software\Blizzard Entertainment` is machine-global,
  so registry faults need save/restore with the restore *verified*.
* **Normalisation is visible.** Handles and pointers are canonicalised to
  symbolic ids in first-seen order (preserving "closed the key it opened"),
  addresses to module+rva, timestamps dropped — and every rule that fires is
  counted and printed. Strings are **never** normalised, because a wrong
  registry path or filename is precisely the defect being hunted.
* **An incomplete run is not a clean run.** `trace_diff.py` says so loudly
  when either side failed to reach the handoff.

## Fault matrix

| Fault | Forces |
| --- | --- |
| `none` | baseline |
| `no-ini` | the ini-missing config path |
| `bad-ini` | the parse-failure path |
| `no-renderer` | the renderer load-failure path |
| `readonly-dir` | write failures |

`apply_fault` refuses when a fault matches no files — an "applied" fault
that changed nothing would produce a clean-run trace labelled as a fault
run, which is the worst kind of pass.
