@echo off
setlocal EnableDelayedExpansion
REM ============================================================
REM  Launch Project Diablo 2 (Game.exe) + D2Debugger oracle
REM  ----------------------------------------------------------
REM  D2_DEBUGGER=1 embeds the oracle in the game, so this single
REM  launch starts BOTH the game and the oracle on :8790.
REM  (The MCP bridge conformance/d2debugger_mcp/server.py talks
REM   to that :8790 endpoint; nothing else to start here.)
REM ============================================================

REM ---- self-elevate: D2.Detours needs admin to hook the game ----
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo Requesting administrator privileges ^(UAC^)...
    powershell -NoProfile -Command "Start-Process -FilePath '%~f0' -Verb RunAs"
    exit /b
)

REM ============================================================
REM  CONFIG - edit these paths if your install moves
REM ============================================================
set "D2MOO_ROOT=C:\Users\benam\source\cpp\D2MOO"
set "GAME_EXE=C:\Diablo2\ProjectD2\Game.exe"
set "DIABLO2_PATCH=%D2MOO_ROOT%\build-1.13c\patch"
set "D2_DEBUGGER=1"
set "LAUNCHER=%D2MOO_ROOT%\build-1.13c\external\D2.Detours\source\Release\D2.DetoursLauncher.exe"
set "GAME_ARGS=-w"
REM ============================================================

echo(
echo ==== Launch PD2 + Oracle ====
if not exist "%LAUNCHER%" ( echo ERROR: launcher not found: & echo   %LAUNCHER% & pause & exit /b 1 )
if not exist "%GAME_EXE%" ( echo ERROR: Game.exe not found: & echo   %GAME_EXE% & pause & exit /b 1 )
if not exist "%DIABLO2_PATCH%" ( echo ERROR: patch dir not found: & echo   %DIABLO2_PATCH% & pause & exit /b 1 )

REM ---- refuse to launch a second instance (two games vs one oracle corrupt state) ----
tasklist /FI "IMAGENAME eq Game.exe" 2>nul | find /I "Game.exe" >nul
if %errorlevel% equ 0 (
    echo Game.exe is ALREADY running - not launching a second instance.
    echo   Two games against one oracle corrupt the build/oracle state.
    echo(
    timeout /t 7 /nobreak >nul
    exit /b 0
)

echo Patch : %DIABLO2_PATCH%
echo Game  : %GAME_EXE%
echo Oracle: http://127.0.0.1:8790  ^(D2_DEBUGGER=1^)
echo(
echo Launching...
start "" "%LAUNCHER%" "%GAME_EXE%" -- %GAME_ARGS%

REM ---- give the game a moment, then check the oracle came up ----
timeout /t 14 /nobreak >nul
powershell -NoProfile -Command "try { $r = Invoke-WebRequest -Uri 'http://127.0.0.1:8790/status' -TimeoutSec 4 -UseBasicParsing; Write-Host ('  Oracle :8790 -> HTTP ' + $r.StatusCode + ' (up)') -ForegroundColor Green } catch { Write-Host '  Oracle :8790 not responding yet.' -ForegroundColor Yellow; Write-Host '  If a Diablo II startup error dialog appeared, close it and run this again (it comes up clean on the 2nd try).' }"

echo(
echo Game window up = oracle live on http://127.0.0.1:8790
echo For the Prove worker, load a character next (d2dbg_load_character summoner-skele).
echo(
timeout /t 8 /nobreak >nul
endlocal
