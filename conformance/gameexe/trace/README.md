# Game.exe behavioural tracer (CONF_TRACE) — status: **not yet working**

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

### 1. Game.exe RE-LAUNCHES ITSELF ELEVATED

This is the important one, and it invalidates the obvious design.

`frida.spawn` starts the binary suspended and un-elevated, hooks are
installed, and the process is resumed — but PD2's `Game.exe` then relaunches
itself with elevation and the original process exits. The instrumented
process is therefore **not** the process that goes on to do the work: the
trace ends almost immediately, `reached_handoff` is false, and an
**orphaned elevated Game.exe is left behind that a non-elevated
`Stop-Process` cannot kill**.

Consequences for the design:

* the whole tracer must run **from an elevated shell**, matching what
  `conformance/behavioral/pd2_frida_capture.py` already warns about
  ("since PD2 runs elevated, likely an elevated shell to inject");
* the driver must follow the elevation relaunch, or spawn already-elevated
  so no relaunch happens;
* cleanup must not assume the spawned pid is the pid to kill. The `finally`
  block added after the first crash kills the spawn — which is now known to
  be the wrong process.

Until that is fixed, **run this only when you can clean up elevated
processes**, and check for strays afterwards:

```powershell
Get-CimInstance Win32_Process -Filter "Name='Game.exe'" |
  Select-Object ProcessId, ExecutablePath, CreationDate
```

Distinguish yours from a real session by `CreationDate` and by
`ExecutablePath` pointing into the sandbox — a live PD2 shows no path
(elevated) and a much older creation time.

### 2. `hooked: 0` — imports enumerate but nothing attaches

`main.enumerateImports()` returns **85** imports for Game.exe, and the
attach loop skips every one via its `if (!imp.address) return` guard. Frida
17 moved import enumeration onto the Module instance (the first version
called `Module.enumerateImports(name)` and died with a bare
`TypeError: not a function`); the returned records evidently do not carry a
usable `address` in this configuration. Next step is to print one record's
keys and switch to hooking the IAT **slot** if that is what is populated —
`probe` in `C:\tmp\probe_imports.js` does exactly that.

Do not "fix" this by removing the guard: attaching to a null address would
either fail loudly or corrupt the process, and a tracer that hooks fewer
imports than it reports would make two different binaries look identical for
the dullest possible reason. The `ready` message deliberately reports
`imports`, `hooked` and `failed` separately so this could not pass silently
— and it did not.

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
