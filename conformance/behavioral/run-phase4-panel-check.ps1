<#
  run-phase4-panel-check.ps1 -- Phase 4 of ../LIVE_DISPATCH_FRAMEWORK_PLAN.md:
  live visual check that the new "Live Dispatch Registry" ImGui panel
  (D2Debugger.LiveDispatch.cpp) actually renders inside a running PD2 session.

  Low-risk vs. Phase 1/2: this only ADDS a read-only ImGui window to the
  existing, already-working D2Debugger. The coord-family live dispatcher
  (D2Common.dll in the patch folder) is also active here (Original mode by
  default, D2MOO_DISPATCH_MODE unset) -- already proven safe.

  Launches with -debug so D2Debugger's GAME_UpdateProgress hook activates
  (see D2Debugger.patch.common.cpp IsDebuggerEnabled()).
#>
param(
    [string]$GameExe = 'C:\Diablo2\ProjectD2\Game.exe',
    [string]$BuildDir = "$PSScriptRoot\..\..\build-1.13c",
    [string]$Config = 'Release'
)
$ErrorActionPreference = 'Stop'
$Here = $PSScriptRoot

$principal = New-Object Security.Principal.WindowsPrincipal(
    [Security.Principal.WindowsIdentity]::GetCurrent())
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Host 'Requesting elevation (one UAC) -- PD2 needs it...' -ForegroundColor Yellow
    $a = @('-NoProfile','-ExecutionPolicy','Bypass','-File', "`"$PSCommandPath`"",
           '-GameExe', "`"$GameExe`"", '-BuildDir', "`"$BuildDir`"", '-Config', $Config)
    Start-Process powershell.exe -Verb RunAs -ArgumentList $a
    return
}

Write-Host '=== ELEVATED: Phase 4 registry panel live check ===' -ForegroundColor Green

$launcher = Join-Path $BuildDir "external\D2.Detours\source\$Config\D2.DetoursLauncher.exe"
$patchDir = Join-Path $BuildDir 'patch'

if (-not (Test-Path $launcher)) { throw "Launcher not built: $launcher" }
if (-not (Test-Path (Join-Path $patchDir 'D2Debugger.dll'))) { throw "D2Debugger.dll not staged in $patchDir" }
if (-not (Test-Path (Join-Path $patchDir 'D2Common.dll'))) { throw "D2Common.dll not staged in $patchDir" }

Write-Host "Launcher : $launcher"
Write-Host "Patch dir: $patchDir"
Write-Host "Game     : $GameExe"
Write-Host ''
Write-Host 'Look for a "Game" ImGui window (existing) AND a NEW "Live Dispatch' -ForegroundColor Cyan
Write-Host 'Registry" window listing 8 functions with color-coded proof status.' -ForegroundColor Cyan
Write-Host 'This window BLOCKS until you close the game.' -ForegroundColor Cyan
Write-Host ''

$env:DIABLO2_PATCH = $patchDir
$env:D2_DEBUGGER = '1'

& $launcher $GameExe -- -w -debug
$code = $LASTEXITCODE
Write-Host ("D2.DetoursLauncher exit code: {0}" -f $code)
Read-Host 'Game exited. Press Enter to close this window'
