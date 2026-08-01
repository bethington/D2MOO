@echo off
REM Install / uninstall the dsound shim into the game directory.
REM
REM   install.bat          -> install (backs up any existing dsound.dll once)
REM   install.bat remove   -> uninstall, restoring the backup if there was one
REM
REM Uninstalling is just deleting a file: with no local dsound.dll the loader
REM falls back to the system one and everything is exactly as it was.
setlocal
set GAME=C:\Diablo2\ProjectD2
set SRC=%~dp0out\dsound.dll

if /I "%~1"=="remove" goto :remove

if not exist "%SRC%" (
  echo ERROR: %SRC% not found -- run build.bat first.
  exit /b 1
)
if exist "%GAME%\dsound.dll" (
  if not exist "%GAME%\dsound.dll.orig" (
    copy /y "%GAME%\dsound.dll" "%GAME%\dsound.dll.orig" >nul
    echo backed up existing dsound.dll -^> dsound.dll.orig
  )
)
copy /y "%SRC%" "%GAME%\dsound.dll" >nul
if errorlevel 1 ( echo ERROR: copy failed -- is the game running? & exit /b 1 )
echo INSTALLED: %GAME%\dsound.dll
exit /b 0

:remove
if exist "%GAME%\dsound.dll.orig" (
  move /y "%GAME%\dsound.dll.orig" "%GAME%\dsound.dll" >nul
  echo RESTORED original dsound.dll
) else (
  del /q "%GAME%\dsound.dll" 2>nul
  echo REMOVED shim -- loader will use the system dsound.dll
)
exit /b 0
