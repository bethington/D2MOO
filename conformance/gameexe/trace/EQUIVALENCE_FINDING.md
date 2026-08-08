# Reconstructed Game.exe: where it stands (corrected 2026-08-08)

## The honest result of the direct comparison
Run WITHOUT a debugger, in the real PD2 install (Start-Process, full D2 state):

    ORIGINAL Game.exe    -> EXITED 0x00000000  (clean: launcher ran, returned)
    Game.recon.exe       -> EXITED 0xC00000FD  (STACK OVERFLOW at Fog!InitErrorMgr)

So the reconstruction is NOT yet functionally equivalent: it crashes at
FOG_InitErrorMgr (@10019) via an infinite recursion inside Fog
(10019->10142->10251->10085->10030->10234 recursing), while the original runs
past it and exits cleanly.

## Correction to the earlier claim
An earlier note claimed instruction-level equivalence because BOTH binaries
crashed identically under cdb (same instruction, same registers). That was a
DEBUGGER ARTIFACT: cdb makes Fog's InitErrorMgr recurse for BOTH the original
and the recon (a debug-detection / exception-handling interaction). Without a
debugger the original does NOT crash; the recon does. The cdb runs proved the
two are identical *under a debugger*, not that the recon runs correctly.

## What is ruled OUT (verified, not assumed)
- Import ordinals: correct. Traced from the ORIGINAL's own thunks -
  SetLogPrefix@10021, InitErrorMgr@10019, SStrPrintf@578; the full Fog {12} and
  Storm {6} ordinal sets match the original exactly.
- Call arguments: match the original's bytes (ECX=name, EDX=0, push ver,
  push flag=1 for InitErrorMgr; same for SetLogPrefix / SStrPrintf).
- Stack reserve: identical (0x100000). The recursion consumes the full 1 MB,
  so it is data-driven/infinite in the recon, not a stack-size shortfall.
- Same Fog.dll (PD2's), loaded by identical ordinals.

## The remaining suspect
A CRT / linker / PE-loader state difference makes Fog's error manager recurse
in the recon's process but not the original's -- e.g. a load-config / SEH /
security-cookie / heap-flag / CRT-init difference between our VS2003 SP1 `/MT`
link and D2's original link. Next step: bisect that by matching the original's
link characteristics, or trace Fog's InitErrorMgr internal state (the structure
10234 walks) side by side in both processes with the debugger's Fog-crash
suppressed (clear PEB flags / vector the exception).

## What DID land (unchanged, real)
- 10/24 launcher functions byte-exact (incl. SaveCmdLine 264B first-try).
- Full launcher reconstructed (GameStart +5, GameInit -68), imports match the
  original exactly by ordinal.
- The whole pipeline works: reconstruct -> synth import libs -> link (era CRT)
  -> run -> Frida/cdb behavioural harness. The recon RUNS and reaches Fog
  InitErrorMgr; it just does not survive it yet.

## Update: more causes ruled out (still crashing)
- Version resource: ADDED (linked a VERSIONINFO); recon still EXITED 0xC00000FD.
- CRT startup: RUNS. Entry RVA 0x1f06 is the standard MSVC startup
  (`push 60h; push <table>; call __security_init_cookie...`), not WinMain --
  the missing WinMainCRTStartup frame in the cdb stack was just an FPO unwind
  gap, not an uninitialised CRT.
- Load-config GlobalFlagsSet = 0 (no forced heap-debug flags / NtGlobalFlag).
- Still-different vs the original PE: a DEBUG directory (28B) and a larger
  RESOURCE dir (icons + manifest; ours is version-only), plus whatever CRT
  build D2 linked.

## Definitive next step (not yet done)
Disassemble Fog.dll's InitErrorMgr (@10019) and the path into the recursing
10234, to read the BRANCH CONDITION that sends it down the recursing path.
That names exactly what Fog inspects in the host process (a header field, a
module-list walk, an SEH/handler check, a manifest/version probe), which is the
one thing that separates our binary from D2's original here. Everything cheaper
than reading that branch has now been tried and ruled out.

## Located: Fog's error handler recursing (2026-08-08)
The crash is NOT in the 10234 export -- cdb labelled it by nearest export. The
faulting code is an INTERNAL Fog function at Fog+0x18635 (`sub esp,464h`): a
crash-report/error LOGGER. It rate-limits to once/24h (`cmp eax,0x5265C00`),
reads computer/user name, formats a path, and writes a log file. It is Fog's
installed ERROR HANDLER.

Mechanism: FOG_InitErrorMgr installs this handler, then an operation during
init RAISES AN ERROR in our process (that the original does not), which invokes
the handler, whose own work raises another error -> the handler re-enters
itself -> stack overflow. So the real question is: what does our binary do
during InitErrorMgr that raises a Fog error the original's does not? Everything
cheap is ruled out (ordinals, args, stack, CRT init, GlobalFlags, version
resource). Remaining candidates: the DEBUG directory / full resource set /
manifest the original has and we lack, or the exact CRT build.

## Definitive next step
Run under cdb with Fog's debugger-triggered crash suppressed (clear PEB
BeingDebugged + NtGlobalFlag, or `sxi` the first-chance and single-step), set a
breakpoint at Fog+0x18635 (the handler) with a 1-deep guard, and read the
ERROR CODE / message it is reporting on first entry. That names the failing
operation directly, instead of inferring it.

## MAJOR CORRECTION: the "crash" was an AppCompat filename shim (2026-08-08)
Renaming the ORIGINAL Game.exe to any other name makes IT exit 0xC00000FD at the
SAME Fog fault. So the Fog InitErrorMgr "crash" is a Windows Application
Compatibility SHIM keyed on the filename "Game.exe" (D2 is a known old game):
with the shim, Fog's error path works; without it (any other name), Fog faults.
Our Game.recon.exe crashed for the IDENTICAL reason the renamed original does --
NOT a reconstruction defect. The reconstruction is sound through CRT startup ->
WinMain -> GameInit -> Fog InitErrorMgr.

Run our recon NAMED Game.exe (compat shim applied) + the DLL closure + correct
install root: it RUNS PAST Fog. It then reaches GameStart and hits Fog's
"Halt: Unrecoverable internal error <addr>" dialog after a flood of C++
exceptions (e06d7363) on multiple threads -- a SECOND, separate issue, almost
certainly the still-STUBBED mid-layer (GAME_LoadConfigFromIniFile is a no-op, so
the Config is never populated from the ini/registry, and a downstream D2 DLL
throws on the empty/misconfigured Config in GameStart's async/archive init).

## Revised next step
Finish the stubs -- reconstruct GAME_LoadConfigFromIniFile (the GetPrivateProfile
loop over gaCmdArguments, which also needs the table's dwType/dwIndex filled) and
ApplyProcessSecurityRestrictions -- so GameStart runs on a correctly-populated
Config. Then re-run NAMED Game.exe and diff to the handoff. The two hard artifacts
(Fog crash) are now explained; this is ordinary remaining reconstruction.

## Final localization (2026-08-08): ARCHIVE_LoadArchives precondition
With GAME_LoadConfigFromIniFile reconstructed + gaCmdArguments filled from the
original binary, the recon under the Frida gate tracks the original CALL-FOR-CALL
through ALL of GameInit -- GetPrivateProfile 57=57, RegOpenKey 2=2, OpenSCManager
1=1 (234/248 events) -- and reaches RegCloseKey, the original's next two events
being FindWindowA -> GetProcAddress = the handoff.

The gap is GameStart. The Frida sandbox DOES have the MPQs (make_sandbox copytrees
ProjectD2 + hard-links the base MPQs), so it is not a missing-archive problem.
The recon halts in D2Win!ARCHIVE_LoadArchives (@10037) at +0x1a -- essentially
its entry, i.e. a PRECONDITION assert (Fog "Unrecoverable internal error", the
Halt dialog). The original reaches the handoff in the SAME sandbox under Frida,
so this is a real GameStart-fidelity difference, not an environment/debugger
artifact. In the REAL install the recon exits 0x00000000, identical to the
original (there GameStart returns before the assert path).

Prime suspect: a Fog/archive precondition our GameStart does not establish that
the original's does -- most likely the install path (FOG_GetInstallPath, called
in our CONF_TRACE GAME_MigrateBetaRegistryKeys +55) or the MPQ config from
FOG_MPQSetConfig. Next step: single-step our GameStart's FOG_MPQSetConfig /
FOG_AsyncDataInitialize / the MigrateBeta install-path call against the
original's and compare the Fog globals ARCHIVE_LoadArchives reads at entry.
