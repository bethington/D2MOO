@echo off
REM D2MOO Launcher Script
REM This script launches Diablo II with D2MOO patches and the debugger enabled

setlocal EnableDelayedExpansion

REM ============================================
REM CONFIGURATION - Edit these paths as needed
REM ============================================

REM Path to your Diablo II installation (containing Game.exe)
set "DIABLO2_PATH=C:\Program Files (x86)\Diablo II"

REM Path to D2MOO build output
set "D2MOO_BUILD=C:\Users\benam\source\cpp\D2MOO\out\build\VS2022"

REM Build configuration (Release or Debug)
set "BUILD_CONFIG=Release"

REM Enable debugger? (1=yes, 0=no)
set "ENABLE_DEBUGGER=1"

REM Additional game arguments (e.g., -w for windowed, -direct -txt for modding)
set "GAME_ARGS=-w"

REM ============================================
REM END CONFIGURATION
REM ============================================

echo ============================================
echo D2MOO Launcher
echo ============================================
echo.

REM Check if Game.exe exists
if not exist "%DIABLO2_PATH%\Game.exe" (
    echo ERROR: Game.exe not found at: %DIABLO2_PATH%
    echo Please edit this script and set DIABLO2_PATH correctly.
    pause
    exit /b 1
)

REM Check if D2.DetoursLauncher.exe exists
set "LAUNCHER_EXE=%D2MOO_BUILD%\external\D2.Detours\source\%BUILD_CONFIG%\D2.DetoursLauncher.exe"
if not exist "%LAUNCHER_EXE%" (
    echo ERROR: D2.DetoursLauncher.exe not found at:
    echo %LAUNCHER_EXE%
    echo.
    echo Please build D2MOO first:
    echo   cmake --preset VS2022 -DD2MOO_ORDINALS_VERSION="1.13c"
    echo   cmake --build --preset VS2022 --config Release
    pause
    exit /b 1
)

REM Set the patch directory (where D2MOO DLLs are)
set "DIABLO2_PATCH=%D2MOO_BUILD%\D2.Detours.patches\%BUILD_CONFIG%\patch"

REM Check if patch directory exists
if not exist "%DIABLO2_PATCH%" (
    echo ERROR: Patch directory not found at:
    echo %DIABLO2_PATCH%
    echo.
    echo Please build D2MOO first.
    pause
    exit /b 1
)

echo Diablo II Path: %DIABLO2_PATH%
echo Launcher: %LAUNCHER_EXE%
echo Patch Dir: %DIABLO2_PATCH%
echo.

REM Set environment variables
set "DIABLO2_PATCH=%DIABLO2_PATCH%"

if "%ENABLE_DEBUGGER%"=="1" (
    set "D2_DEBUGGER=1"
    echo Debugger: ENABLED
) else (
    set "D2_DEBUGGER=0"
    echo Debugger: DISABLED
)

echo Game Args: %GAME_ARGS%
echo.
echo ============================================
echo Launching Diablo II with D2MOO...
echo ============================================
echo.

REM Launch the game
"%LAUNCHER_EXE%" "%DIABLO2_PATH%\Game.exe" -- %GAME_ARGS%

if errorlevel 1 (
    echo.
    echo Game exited with error code: %errorlevel%
    pause
)

endlocal
