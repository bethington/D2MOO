<#
  run-capture-session.ps1 -- launch PD2 with LIVE VECTOR CAPTURE enabled, for the
  CONF_REGRESSION offline suite (conformance/CONF_REGRESSION_OFFLINE_SUITE.md).

  Same fresh D2.Detours launch as run-phase1-live-dispatch.ps1, but with:
    - D2_DEBUGGER=1            -> the D2Debugger + MCP control surface (:8790) come up
    - D2MOO_CAPTURE_VECTORS=1  -> matching SHADOW calls append distinct {in->out}
                                 golden samples to behavioral/captured_vectors.jsonl
  The capture flag is read ONCE at D2Common load, so it MUST be in the environment
  before the game starts -- which is why this launcher sets it (you can't flip it on
  a running game).

  Loads the Release patch DLLs from build-1.13c/patch (D2Common.dll must be the
  freshly-staged capture build). After the game is up, set dispatchers to shadow
  (d2dbg_set_all_modes shadow) and play ~1 min; then run capture_to_corpus.py.

  Usage (a normal shell; it self-elevates with one UAC, since PD2 is elevated):
    powershell -ExecutionPolicy Bypass -File run-capture-session.ps1
#>
param(
    [string]$GameExe  = 'C:\Diablo2\ProjectD2\Game.exe',
    [string]$BuildDir = "$PSScriptRoot\..\..\build-1.13c",
    [string]$Config   = 'Release'
)
$ErrorActionPreference = 'Stop'
$Here = $PSScriptRoot

# ---- self-elevate: PD2 is elevated, so the launcher must be too ----
$principal = New-Object Security.Principal.WindowsPrincipal(
    [Security.Principal.WindowsIdentity]::GetCurrent())
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Host 'Requesting elevation (one UAC) -- PD2 needs it...' -ForegroundColor Yellow
    $a = @('-NoProfile','-ExecutionPolicy','Bypass','-File', "`"$PSCommandPath`"",
           '-GameExe', "`"$GameExe`"", '-BuildDir', "`"$BuildDir`"", '-Config', $Config)
    Start-Process powershell.exe -Verb RunAs -ArgumentList $a
    return
}

Write-Host "=== ELEVATED: capture session (Config=$Config) ===" -ForegroundColor Green

$launcher = Join-Path $BuildDir "external\D2.Detours\source\$Config\D2.DetoursLauncher.exe"
$patchDir = Join-Path $BuildDir 'patch'
$patchDll = Join-Path $patchDir 'D2Common.dll'
$builtDll = Join-Path $BuildDir "source\D2Common\$Config\D2Common.dll"

if (-not (Test-Path $launcher)) { throw "Launcher not built: $launcher" }
if (-not (Test-Path $builtDll)) { throw "Capture build not found: $builtDll (build target D2Common $Config first)" }

# Stage the freshly-built capture D2Common.dll into the patch dir. This is why PD2
# must be CLOSED before running: a loaded DLL is locked and the copy fails ("busy").
Write-Host "Staging capture DLL: $builtDll -> $patchDll"
Copy-Item -Force $builtDll $patchDll
if (-not (Test-Path $patchDll)) { throw "Patch DLL not staged: $patchDll" }

Write-Host "Game     : $GameExe"
Write-Host "Patch dir: $patchDir"
Write-Host "Launcher : $launcher"
Write-Host 'CAPTURE  : ON (D2MOO_CAPTURE_VECTORS=1) -> behavioral/captured_vectors.jsonl' -ForegroundColor Cyan
Write-Host 'This window BLOCKS until you close the game (the launcher waits on it).' -ForegroundColor Cyan

$env:DIABLO2_PATCH        = $patchDir
$env:D2_DEBUGGER          = '1'   # bring up the D2Debugger MCP on :8790
$env:D2MOO_CAPTURE_VECTORS = '1'  # <-- record golden vectors on matching shadow calls
$env:D2MOO_DISPATCH_MODE  = 'shadow'

& $launcher $GameExe -- -w
$code = $LASTEXITCODE
Write-Host ("D2.DetoursLauncher exit code: {0}" -f $code)
