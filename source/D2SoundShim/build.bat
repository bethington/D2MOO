@echo off
REM 32-bit, to match Game.exe. Output goes to the build dir, NOT straight into
REM the game folder -- installing is a separate, deliberate step (install.bat).
setlocal
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat" >nul 2>&1
cd /d "%~dp0"
if not exist out mkdir out
cl /nologo /O2 /EHsc /W3 /MT /LD /Fo:out\ /Fe:out\dsound.dll dsound_shim.cpp ^
   /link /SUBSYSTEM:WINDOWS dxguid.lib ole32.lib user32.lib kernel32.lib
echo BUILD_EXIT=%ERRORLEVEL%
if exist out\dsound.dll (
  echo ---- exports ----
  dumpbin /exports out\dsound.dll | findstr /R "[0-9] .*Direct [0-9] .*Dll [0-9] .*GetDevice"
)
