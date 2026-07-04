<#
  start-frida-oracle.ps1 -- one-UAC launcher for the PD2 Frida oracle.

  PD2's Game.exe runs ELEVATED (it requires admin by design), so Frida must be
  elevated to attach to it. This script self-elevates ONCE, then does everything
  in that one elevated context -- launch the game AND run the capture -- so you
  approve a single UAC and get no per-attach prompts. Still far simpler than the
  old dbgeng path: NO ScyllaHide, NO debugger server, just game + Frida.

    1. Self-elevate (one UAC).
    2. Ensure Game.exe is running (launch -w if not).
    3. With -Capture: provision a local .venv-frida (pip install frida) and run
       pd2_frida_capture.py -> pd2_fp.json. Without -Capture, just leave the game
       up and print the command to run.

  For iterative capture, either re-run with -Capture, or open ONE elevated
  PowerShell and run  .venv-frida\Scripts\python pd2_frida_capture.py  repeatedly
  (no re-prompt within that elevated session).

  Usage (normal shell):
    powershell -ExecutionPolicy Bypass -File start-frida-oracle.ps1 -Capture
#>
param(
    [string]$GameExe = 'C:\Diablo2\ProjectD2\Game.exe',
    [switch]$Capture,
    [string]$Python = ''
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
    if ($Capture) { $a += '-Capture' }
    if ($Python)  { $a += @('-Python', "`"$Python`"") }
    Start-Process powershell.exe -Verb RunAs -ArgumentList $a
    return
}

Write-Host '=== ELEVATED: PD2 Frida oracle ===' -ForegroundColor Green

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
if (-not $game) { throw "Game.exe did not start." }
Write-Host ("Game.exe PID {0}. Bring it to the MAIN MENU (D2Common loads there)." -f $game.Id) -ForegroundColor Cyan

if (-not $Capture) {
    Write-Host 'Ready. Capture with:  .venv-frida\Scripts\python pd2_frida_capture.py' -ForegroundColor Green
    Read-Host 'Press Enter to close (game keeps running)'
    return
}

# 2. provision the Frida venv
if (-not $Python) {
    $venv = Join-Path $Here '.venv-frida'
    $Python = Join-Path $venv 'Scripts\python.exe'
    if (-not (Test-Path $Python)) {
        Write-Host 'Provisioning .venv-frida (pip install frida)...'
        python -m venv $venv
        & $Python -m pip install -q --disable-pip-version-check frida
    }
}

# 3. let the menu finish loading D2Common, then capture
Write-Host 'Waiting ~8s for D2Common to load, then capturing...'
Start-Sleep 8
& $Python (Join-Path $Here 'pd2_frida_capture.py') "$($game.Id)"
Write-Host 'Compare with:  python compare_fp.py' -ForegroundColor Green
Read-Host 'Press Enter to close (game keeps running)'
