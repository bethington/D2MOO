@echo off
:: D2MOO Launcher with Administrator Privileges
:: This script launches D2MOO with the debugger enabled

:: Request admin privileges
>nul 2>&1 "%SYSTEMROOT%\system32\cacls.exe" "%SYSTEMROOT%\system32\config\system"
if '%errorlevel%' NEQ '0' (
    echo Requesting administrative privileges...
    goto UACPrompt
) else ( goto gotAdmin )

:UACPrompt
    echo Set UAC = CreateObject^("Shell.Application"^) > "%temp%\getadmin.vbs"
    echo UAC.ShellExecute "%~s0", "", "", "runas", 1 >> "%temp%\getadmin.vbs"
    "%temp%\getadmin.vbs"
    exit /B

:gotAdmin
    if exist "%temp%\getadmin.vbs" ( del "%temp%\getadmin.vbs" )
    pushd "%CD%"
    CD /D "%~dp0"

:: Configuration
set D2MOO_ROOT=%~dp0
set BUILD_DIR=%D2MOO_ROOT%out\build\VS2022\runtime\Release
set DIABLO2_PATH=C:\Diablo2

:: Set environment variables
set DIABLO2_PATCH=%BUILD_DIR%
set D2_DEBUGGER=1

echo ============================================
echo  D2MOO Launcher (Administrator Mode)
echo ============================================
echo.
echo D2MOO Build:    %BUILD_DIR%
echo Diablo II:      %DIABLO2_PATH%
echo Debugger:       ENABLED
echo.

:: Check if launcher exists
if not exist "%BUILD_DIR%\D2.DetoursLauncher.exe" (
    echo ERROR: D2.DetoursLauncher.exe not found!
    echo Please build D2MOO first using:
    echo   cmake --preset VS2022
    echo   cmake --build --preset VS2022 --config Release
    exit /b 1
)

:: Check if Game.exe exists
if not exist "%DIABLO2_PATH%\Game.exe" (
    echo ERROR: Game.exe not found at %DIABLO2_PATH%
    echo Please update DIABLO2_PATH in this script.
    exit /b 1
)

echo Starting Diablo II with D2MOO...
cd /d "%DIABLO2_PATH%"
"%BUILD_DIR%\D2.DetoursLauncher.exe" "%DIABLO2_PATH%\Game.exe" -- -w
echo.
echo D2MOO launched successfully.
