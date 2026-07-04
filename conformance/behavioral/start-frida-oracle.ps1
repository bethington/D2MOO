<#
  start-frida-oracle.ps1 -- NON-ELEVATED launcher for the PD2 Frida oracle.

  The Frida oracle runs IN-PROCESS, so unlike the old dbgeng path it needs NO
  admin, NO ScyllaHide, and NO debugger server -- just the game running as a
  normal user with Frida attached. Steps:
    1. Ensure Game.exe is running (launch -w as a NORMAL user if not).
    2. Wait until D2Common.dll is loaded (game reached the main menu; the coord
       functions live there and are pure, so we don't need to be in a game).
    3. With -Capture, run the Frida capture (pd2_frida_capture.py) and write
       pd2_fp.json. Auto-provisions a local .venv-frida (pip install frida) if
       no -Python is given.

  IMPORTANT: run this as a NORMAL (non-elevated) user. If the game is launched
  elevated, non-elevated Frida cannot attach to it.

  Usage:
    powershell -ExecutionPolicy Bypass -File start-frida-oracle.ps1            # just launch
    powershell -ExecutionPolicy Bypass -File start-frida-oracle.ps1 -Capture   # launch + capture
#>
param(
    [string]$GameExe = 'C:\Diablo2\ProjectD2\Game.exe',
    [switch]$Capture,
    [string]$Python = ''
)
$ErrorActionPreference = 'Stop'
$Here = $PSScriptRoot

# Guard: we must NOT be elevated (else the game we launch inherits elevation and
# non-elevated Frida can't attach).
$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()
          ).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if ($isAdmin) {
    Write-Host 'WARNING: running elevated. The game will inherit elevation and Frida (non-elevated) will not attach. Run this in a NORMAL shell.' -ForegroundColor Red
}

function Get-Game { Get-Process -Name 'Game' -ErrorAction SilentlyContinue | Select-Object -First 1 }

# 1. ensure the game is running (non-elevated)
$game = Get-Game
if ($game) {
    Write-Host ("Game.exe already running (PID {0})." -f $game.Id)
} else {
    Write-Host ("Launching {0} -w (normal user)..." -f $GameExe)
    Start-Process -FilePath $GameExe -ArgumentList '-w' -WorkingDirectory (Split-Path $GameExe)
    for ($i = 0; $i -lt 30 -and -not $game; $i++) { Start-Sleep 1; $game = Get-Game }
}
if (-not $game) { throw "Game.exe did not start." }
Write-Host ("Game.exe PID {0}. Bring it to the MAIN MENU (D2Common loads there)." -f $game.Id) -ForegroundColor Cyan

if (-not $Capture) {
    Write-Host 'Ready. Run the capture with:  python pd2_frida_capture.py' -ForegroundColor Green
    return
}

# 2. provision a Frida venv if no -Python given
if (-not $Python) {
    $venv = Join-Path $Here '.venv-frida'
    $Python = Join-Path $venv 'Scripts\python.exe'
    if (-not (Test-Path $Python)) {
        Write-Host 'Provisioning .venv-frida (pip install frida)...'
        python -m venv $venv
        & $Python -m pip install -q --disable-pip-version-check frida
    }
}

# 3. give the menu a moment to finish loading D2Common, then capture
Write-Host 'Waiting ~8s for D2Common to load, then capturing...'
Start-Sleep 8
& $Python (Join-Path $Here 'pd2_frida_capture.py') "$($game.Id)"
