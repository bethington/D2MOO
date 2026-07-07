<#
  run-phase1-live-dispatch.ps1 -- Phase 1 of ../LIVE_DISPATCH_FRAMEWORK_PLAN.md:
  live NATIVE (D2.Detours) test of the dispatcher on DUNGEON_GameTileToClientCoords,
  against the REAL elevated PD2 Game.exe.

  Unlike Phase 0 (Frida, attach to an already-running game), this launches the
  game FRESH through D2.DetoursLauncher.exe, which uses DetourCreateProcessWithDllExW
  to inject D2.Detours.dll into the new process. D2.Detours then loads whatever
  *.dll it finds in the DIABLO2_PATCH folder (here: build-1.13c/patch/D2Common.dll,
  our build with the Phase 1 dispatcher baked in) and wires the hook via a real
  Detours transaction -- the actual mechanism the whole framework depends on, not
  Frida's Interceptor stand-in.

  Mode is read once at D2Common.dll load from D2MOO_DISPATCH_MODE (original |
  reimpl | shadow) -- see LiveDispatch_GameTileToClientCoords.h. Defaults to
  'original', which is a behavioral no-op (the dispatcher just calls straight
  through to the real function via the Detours-built trampoline).

  The dispatcher writes a throttled heartbeat + any SHADOW divergence to a fixed
  log file (phase1_dispatch_log.txt, next to this script) -- readable after/while
  the game runs, no debugger needed. D2.DetoursLauncher itself BLOCKS until the
  game exits (WaitForSingleObject INFINITE), so this window stays busy for the
  whole play session; check the log from elsewhere while it runs.

  Usage (normal shell):
    powershell -ExecutionPolicy Bypass -File run-phase1-live-dispatch.ps1
    powershell -ExecutionPolicy Bypass -File run-phase1-live-dispatch.ps1 -Mode reimpl
    powershell -ExecutionPolicy Bypass -File run-phase1-live-dispatch.ps1 -Mode shadow
#>
param(
    [ValidateSet('original','reimpl','shadow')]
    [string]$Mode = 'original',
    [string]$GameExe = 'C:\Diablo2\ProjectD2\Game.exe',
    [string]$BuildDir = "$PSScriptRoot\..\..\build-1.13c",
    [string]$Config = 'Debug'
)
$ErrorActionPreference = 'Stop'
$Here = $PSScriptRoot

# ---- self-elevate: PD2 is elevated, so the launcher must be too ----
$principal = New-Object Security.Principal.WindowsPrincipal(
    [Security.Principal.WindowsIdentity]::GetCurrent())
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Host 'Requesting elevation (one UAC) -- PD2 needs it...' -ForegroundColor Yellow
    $a = @('-NoProfile','-ExecutionPolicy','Bypass','-File', "`"$PSCommandPath`"",
           '-Mode', $Mode, '-GameExe', "`"$GameExe`"", '-BuildDir', "`"$BuildDir`"", '-Config', $Config)
    Start-Process powershell.exe -Verb RunAs -ArgumentList $a
    return
}

Write-Host "=== ELEVATED: Phase 1 live dispatch (mode=$Mode) ===" -ForegroundColor Green

$launcher = Join-Path $BuildDir "external\D2.Detours\source\$Config\D2.DetoursLauncher.exe"
$patchDir = Join-Path $BuildDir 'patch'
$patchDll = Join-Path $patchDir 'D2Common.dll'
$logFile  = Join-Path $Here 'phase1_dispatch_log.txt'

if (-not (Test-Path $launcher)) { throw "Launcher not built: $launcher" }
if (-not (Test-Path $patchDll)) { throw "Patch DLL not staged: $patchDll (copy build-1.13c/source/D2Common/$Config/D2Common.dll there first)" }

Write-Host "Launcher : $launcher"
Write-Host "Patch dir: $patchDir  (D2Common.dll dated $((Get-Item $patchDll).LastWriteTime))"
Write-Host "Game     : $GameExe"
Write-Host "Log file : $logFile"
Write-Host ''
Write-Host 'This window BLOCKS until you close the game (D2.DetoursLauncher waits on the' -ForegroundColor Cyan
Write-Host 'child process). Bring the game to the main menu (or play a bit) so the coord' -ForegroundColor Cyan
Write-Host 'function fires on the GAME''S OWN calls -- that is the real Phase 1 signal.' -ForegroundColor Cyan
Write-Host 'Read phase1_dispatch_log.txt from another window/session while this runs.' -ForegroundColor Cyan
Write-Host ''

$env:DIABLO2_PATCH = $patchDir
$env:D2MOO_DISPATCH_MODE = $Mode

& $launcher $GameExe -- -w
$code = $LASTEXITCODE
Write-Host ("D2.DetoursLauncher exit code: {0}" -f $code)
Read-Host 'Game exited. Press Enter to close this window'
