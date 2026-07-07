<#
  run-phase0-spike.ps1 -- one-UAC launcher for the Phase 0 inline-hook tolerance
  spike (../LIVE_DISPATCH_FRAMEWORK_PLAN.md Phase 0). Modeled on
  start-frida-oracle.ps1.

  PD2's Game.exe runs ELEVATED, so Frida must be too. This self-elevates ONCE,
  then in that one elevated context: ensures the game is running, provisions a
  local frida venv, and runs pd2_phase0_spike.py. The spike is self-driving, so
  the game only needs to reach the MAIN MENU (where D2Common is loaded) -- no
  gameplay required. (If you ARE in a level, the coord fn also fires from
  rendering, an extra signal.)

  Usage (normal shell):
    powershell -ExecutionPolicy Bypass -File run-phase0-spike.ps1
    powershell -ExecutionPolicy Bypass -File run-phase0-spike.ps1 -Replace
#>
param(
    [string]$GameExe = 'C:\Diablo2\ProjectD2\Game.exe',
    [switch]$Replace
)
$ErrorActionPreference = 'Stop'
$Here = $PSScriptRoot

# ---- self-elevate: PD2 is elevated, so Frida must be too ----
$principal = New-Object Security.Principal.WindowsPrincipal(
    [Security.Principal.WindowsIdentity]::GetCurrent())
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Host 'Requesting elevation (one UAC) -- Frida needs it to attach to PD2...' -ForegroundColor Yellow
    $a = @('-NoProfile','-ExecutionPolicy','Bypass','-File', "`"$PSCommandPath`"",
           '-GameExe', "`"$GameExe`"")
    if ($Replace) { $a += '-Replace' }
    Start-Process powershell.exe -Verb RunAs -ArgumentList $a
    return
}

Write-Host '=== ELEVATED: PD2 Phase 0 inline-hook tolerance spike ===' -ForegroundColor Green

# Transcript + verdict file are how the (non-elevated) caller reads the result of
# this separate elevated window.
$log = Join-Path $Here 'phase0_transcript.log'
$verdict = Join-Path $Here 'phase0_verdict.json'
Remove-Item $verdict -ErrorAction SilentlyContinue
try { Start-Transcript -Path $log -Force | Out-Null } catch {}

function Get-Game { Get-Process -Name 'Game' -ErrorAction SilentlyContinue | Select-Object -First 1 }

# 1. ensure the game is running
$game = Get-Game
if ($game) {
    Write-Host ("Game.exe already running (PID {0})." -f $game.Id)
} else {
    Write-Host ("Launching {0} -w ..." -f $GameExe)
    Start-Process -FilePath $GameExe -ArgumentList '-w' -WorkingDirectory (Split-Path $GameExe)
    for ($i = 0; $i -lt 30 -and -not $game; $i++) { Start-Sleep 1; $game = Get-Game }
}
if (-not $game) { throw 'Game.exe did not start.' }
Write-Host ("Game.exe PID {0}. Bring it to at least the MAIN MENU (D2Common loads there)." -f $game.Id) -ForegroundColor Cyan
Read-Host 'Press Enter once the game is at the main menu (or in a level)'

# 2. provision the Frida venv (reuse the oracle's if present)
$venv = Join-Path $Here '.venv-frida'
$Python = Join-Path $venv 'Scripts\python.exe'
if (-not (Test-Path $Python)) {
    Write-Host 'Provisioning .venv-frida (pip install frida)...'
    python -m venv $venv
    & $Python -m pip install -q --disable-pip-version-check frida
}

# 3. run the spike
$spikeArgs = @((Join-Path $Here 'pd2_phase0_spike.py'), "$($game.Id)")
if ($Replace) { $spikeArgs += '--replace' }
& $Python @spikeArgs
$code = $LASTEXITCODE
Write-Host ("Spike exit code: {0} (0 = PASS)" -f $code) -ForegroundColor ($(if ($code -eq 0) { 'Green' } else { 'Red' }))
try { Stop-Transcript | Out-Null } catch {}
Read-Host 'Press Enter to close (game keeps running)'
