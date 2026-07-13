# PD2 Loading & Anti-Tamper Report

How Project Diablo 2 bootstraps itself when you launch `Game.exe`, how its files
differ from the original retail game, and what anti-debug / anti-tamper protection
the running client actually has — with a strategy for locating tamper-detection
among PD2's memory modifications and the investigation that confirms the findings.

Environment: PD2 install `C:\Diablo2\ProjectD2` (1.13c-based), retail base
`C:\Diablo2` (1.14d), captured live from `Game.exe` (32-bit) via `ReadProcessMemory`.
This is the offline/single-player client as it runs in the D2MOO conformance rig.

---

## TL;DR

- **Launching `ProjectD2\Game.exe` runs the vanilla 1.13c engine.** PD2 modifies only
  **two files on disk** — `Storm.dll` and `D2Win.dll` — and uses them purely to
  bootstrap DLL injection. The other 19 game binaries are byte-identical to vanilla.
- **The mod loads through two hijacked import/loader paths**: `Storm.dll`'s import table
  is repointed from `VERSION.dll` → `PD2_EXT.dll`, and `D2Win.dll` is patched to
  `LoadLibraryA("ProjectDiablo.dll")`. `ProjectDiablo.dll` then pulls in `BH.dll`
  (maphack) and the SGD2 HD mods, and applies ~1,637 in-memory patches.
- **The client has essentially no active anti-tamper.** Live probing shows
  `PEB.BeingDebugged = 0`, `NtGlobalFlag = 0`, and **no anti-debug API is hooked**
  (kernel32 / kernelbase / ntdll prologues are all clean). The `IsDebuggerPresent`
  imports are MSVC CRT security-handler boilerplate, not deliberate checks.
- **The only genuine tamper check is passive**: `PD2_EXT.dll` calls
  `LoadLibrary("ProjectDiablo.dll")` and, if it fails, tells the user to *"check your
  Anti-Virus logs and restore the file."* It detects deletion/quarantine, not patching.
- **Proof by coexistence**: D2MOO's detours are patching `D2Common.dll` in the same
  process right now with no crash or ban → PD2 does no runtime code-integrity
  enforcement on the client. Real anti-cheat is server-side (the online ladder).

---

## Part 1 — How the binaries get loaded

### 1.1 Retail `C:\Diablo2` vs PD2 `C:\Diablo2\ProjectD2`

| | `C:\Diablo2` (retail) | `C:\Diablo2\ProjectD2` (PD2) |
|---|---|---|
| Version | **1.14d** | **1.13c** |
| `Game.exe` | 3,590,120 bytes — *monolith* (in 1.14 Blizzard merged all `D2*.dll` into `Game.exe`) | 61,440 bytes — classic small loader |
| Game DLLs | none (all code inside `Game.exe`) | full split set: `D2Common`, `D2Game`, `D2Client`, … |
| Data (MPQs) | `d2data.mpq`, `d2char.mpq`, … (shared) | uses the parent's MPQs + its own `d2gl.mpq`, `SGD2FreeRes.mpq` |

PD2 is a **separate 1.13c install nested inside the retail folder**. It deliberately
targets 1.13c — the last version with separate, easily-patchable DLLs — rather than the
1.14d monolith. Launching `ProjectD2\Game.exe` runs the 1.13c engine; the retail 1.14d
`Game.exe` is not involved.

### 1.2 On-disk modifications: exactly two files

Comparing PD2's shipped DLLs against a verified-clean 1.13c base (see
`ondisk_vs_vanilla_1.13c.csv`), **only two binaries are modified**, both to bootstrap
injection. Everything else — including `D2Common`, `D2Game`, `D2Client` — is byte-identical.

**`Storm.dll` — 20 bytes, import-table hijack.** In `.rdata` the import DLL-name string
`VERSION` is overwritten in place with `PD2_EXT` (same 7-char length):

```
.rdata @0x4E63A:  56 45 52 53 49 4F 4E  ("VERSION")
              ->  50 44 32 5F 45 58 54  ("PD2_EXT")
```

`Game.exe` statically imports `Storm.dll`; Storm now statically imports `PD2_EXT.dll`
instead of `VERSION.dll`, so the Windows loader maps **PD2_EXT.dll automatically** at
process start. (`PD2_EXT.dll` re-exports the handful of version functions Storm needs.)

**`D2Win.dll` — 247 bytes (52 in `.text`), a LoadLibrary stub.** A small D2Win function
was overwritten with:

```asm
0x6F8FA2A1: push 0x6F8FE378        ; -> "ProjectDiablo.dll"
0x6F8FA2A6: call dword [0x6F8FB208] ; LoadLibraryA
0x6F8FA2AC: mov  eax, 1
0x6F8FA2B1: ret
```

When `Game.exe` loads `D2Win.dll` (for the UI), this stub runs and loads the mod core.

### 1.3 The full load chain

```
launch ProjectD2\Game.exe   (vanilla 1.13c, imports only classic D2 DLLs)
   │  loader resolves static imports:
   ├─▶ Storm.dll (patched)  ──static import──▶ PD2_EXT.dll        [loaded at init]
   │                                             └─ also LoadLibrary("ProjectDiablo.dll");
   │                                                on failure → "check your Anti-Virus logs"
   └─▶ D2Win.dll (patched)  ──LoadLibraryA──▶  ProjectDiablo.dll  [the mod core]
                                                  ├─▶ BH.dll                (maphack / overlay)
                                                  ├─▶ SGD2FreeRes.dll       (HD resolution)
                                                  └─▶ SGD2FreeDisplayFix.dll (HD display)
                                                  └─ applies ~1,637 in-memory patches to
                                                     D2Common / D2Game / D2Client / …
```

`Game.exe` itself is **unmodified vanilla** and imports no PD2 DLL — the mod rides in on
the two patched game DLLs. (The conformance rig differs only in that
`D2.DetoursLauncher` additionally injects the D2MOO reimpl + D2Debugger; a normal PD2
launch does not.) All gameplay changes are applied **in memory**, not baked into the
DLLs on disk — see `clean_pd2_patch_sites.csv` for the 1,637 sites.

---

## Part 2 — Anti-tamper / anti-debug investigation

### 2.1 Strategy: where tamper-detection would live

Anti-tamper on a D2 client of this kind would show up in one of five places. I checked each:

| # | Hypothesis | How to detect | Result |
|---|---|---|---|
| 1 | Debugger-detection APIs called deliberately | Disassemble `IsDebuggerPresent`/`NtQueryInformationProcess` call sites in the injected DLLs | **CRT boilerplate only** |
| 2 | Anti-debug APIs hooked to hide/spoof | Compare live prologues vs clean on-disk (kernel32/kernelbase/ntdll) | **None hooked** |
| 3 | Debugger artifacts checked (PEB) | Read live `BeingDebugged`, `NtGlobalFlag` | **Both 0, unmodified** |
| 4 | Code-integrity / CRC of `.text` to catch third-party patches | Find CRC routines; check if any of the 1,637 patch targets route through them | **CRC present but not wired to game code; 0 gameplay targets near it** |
| 5 | Blizzard's own security checks disabled by PD2 | Diff the "security failure" code in Storm/D2Win | **Untouched (vanilla)** |

### 2.2 Static findings

`IsDebuggerPresent` is imported by `ProjectDiablo.dll`, `PD2_EXT.dll`, `BH.dll`, and
`SGD2FreeRes.dll` — but disassembling the call sites shows they are the **MSVC CRT
security/GS-failure handler** (`__raise_securityfailure`), which calls
`IsDebuggerPresent` before raising a fatal exception:

```asm
mov  dword [ebp-0x58], 0x40000015   ; error-report record
mov  dword [ebp-0x54], 1            ; NumberParameters
call dword [IsDebuggerPresent]      ; CRT boilerplate, not a deliberate check
```

Only 1 such site in ProjectDiablo, 2 in PD2_EXT — consistent with compiler-inserted
error handlers, not a scattered anti-debug regime.

Other signals and their real meaning:
- **`rdtsc` ×20 in ProjectDiablo, ×4 in BH** — timing, but not wired to any debugger
  check that was found; consistent with RNG / frame timing / profiling.
- **CRC32 polynomial `0xEDB88320` (once, at `0x102B5378`)** — a CRC32 routine exists.
  ProjectDiablo links `libcrypto-1_1.dll` (TLS to the PD2 realm) and BH renders PNGs, so
  CRC32 is expected for zlib/PNG/network use. No evidence it hashes module `.text`.
- **`PD2_EXT.dll` integrity string**: *"Failed to load ProjectDiablo.dll. Please check
  your Anti-Virus logs and restore the file."* The code is a plain
  `LoadLibraryA` + null-check:

```asm
push <"ProjectDiablo.dll">
call [LoadLibraryA]
test eax, eax
jne  success
… push <"…Anti-Virus logs and restore the file."> ; MessageBox
```

  This detects the mod core being **deleted/quarantined** by AV, not modified — a
  passive presence check, not a cryptographic integrity verification.

### 2.3 Dynamic findings (live `Game.exe`, PID 37536)

Read from the running process with an elevated helper:

```
PEB.BeingDebugged        = 0x00     (no debugger flag; not spoofed either)
PEB.NtGlobalFlag (+0x68) = 0x00000000 (no heap-debug bits set)

API prologue hook check (in-process vs clean on-disk):
  ntdll!NtQueryInformationProcess   clean
  ntdll!NtSetInformationThread      clean   (no ThreadHideFromDebugger hooking)
  ntdll!NtQueryObject / NtClose     clean
  ntdll!DbgUiRemoteBreakin/DbgBreakPoint  clean
  kernel32!IsDebuggerPresent        clean   (forwarder → KERNELBASE!IsDebuggerPresent,
                                             genuine `mov eax, fs:[30h]` impl, unhooked)
  kernel32!CheckRemoteDebuggerPresent clean
  ProjectDiablo IsDebuggerPresent IAT → 0x7684D980 (real kernel32, not a PD2 hook)
```

Nothing is hooked, hidden, or spoofed. (An initial pass flagged `IsDebuggerPresent`/
`OutputDebugStringA` as "hooked" — a **false positive**: those are `jmp dword ptr [__imp]`
forwarder stubs whose absolute operand differs only because kernel32 is ASLR-relocated
in memory vs its on-disk preferred base. Following the pointers lands in the real
KERNELBASE implementations.)

### 2.4 Correlation: none of the 1,637 patches are anti-tamper

Of the 1,637 in-memory patch sites, 1,118 redirect into `ProjectDiablo.dll`, and they
all cluster in the **game-logic hook region `0x1017E7D0`–`0x102469B0`**. Distance to the
anti-tamper landmarks:

```
targets within 0x2000 of the CRC32 routine (0x102B5378):        0
targets within 0x2000 of the CRT security/IsDbg handler (0x10252BAD): 0
```

The gameplay patches never route through the anti-debug or CRC code. Anti-tamper, such
as it is, is **self-contained in the injected DLLs and disconnected from the game-DLL
modifications**.

### 2.5 The decisive empirical test

The process under analysis is simultaneously running **D2MOO's detours on `D2Common.dll`**
(the exact kind of third-party in-memory modification the user is doing) **and** the full
PD2 stack — with no crash, no termination, no ban. If PD2's client performed runtime
code-integrity checking, this could not persist. It confirms the static/dynamic findings:
**the PD2 client does not enforce code integrity or anti-debug at runtime.**

---

## Part 3 — Implications for D2MOO / third-party modification

- **Debugging is unobstructed.** No PEB tricks, no API hooks, no timing traps that were
  found. A debugger can attach and D2MOO can detour freely on the client.
- **The only thing that "notices" tampering** is `PD2_EXT.dll`'s load-guard, and only if
  `ProjectDiablo.dll` is *missing* — patching it in memory does not trip it.
- **Caveats.** (1) This is the **offline/single-player** client. PD2's real anti-cheat is
  **server-side**: the online ladder validates game state and restricts `BH.dll` features;
  memory edits that change gameplay would be caught by server validation, not the client.
  (2) `libcrypto-1_1.dll` implies an authenticated TLS channel to the PD2 realm — client
  attestation could be added there in future updates. (3) Findings reflect this specific
  PD2 build; re-run the probes after a PD2 update to confirm they still hold.

---

## Appendix — method & files

**Static**: `pefile` + `capstone` over `C:\Diablo2\ProjectD2\*` — import tables, string
scan for anti-debug/anti-cheat terms, byte-pattern scan (`rdtsc`, `int 2Dh`, PEB `fs:[30]`,
CRC32 polynomials), and disassembly of the `IsDebuggerPresent`/loader/CRC sites.

**Dynamic** (elevated, live process): `ProcessWow64Information` → PEB32 →
`BeingDebugged`/`NtGlobalFlag`; per-API prologue compare (in-process bytes vs clean
on-disk SysWOW64), following forwarder stubs to their real implementations.

Scripts in `tools/`: `antidebug_probe.py`, `antidebug_probe2.py` (dynamic);
the static scans are inline in the session. Data: `ondisk_vs_vanilla_1.13c.csv`
(on-disk file diff), `clean_pd2_patch_sites.csv` (the 1,637 in-memory sites),
`diff_rig.json` (raw per-DLL detail).
