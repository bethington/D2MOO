@echo off
REM Build the stripped 32-bit benchmark DLL from the ground-truth core.
REM Ordinal-only (NONAME) exports mirror PD2's D2Common so fresh Ghidra shows Ordinal_NNNNN.
setlocal
set VS=C:\Program Files\Microsoft Visual Studio\2022\Community
call "%VS%\VC\Auxiliary\Build\vcvarsall.bat" x86 >nul
set SRC=%~dp0src
set OUT=%~dp0build
cd /d "%OUT%"
REM /O2 realistic optimization, /LD DLL, no /Zi so no debug info, /DEF ordinal exports.
cl /nologo /O2 /LD /Gz "%SRC%\bench_core.c" /Fe:bench_core.dll ^
   /link /DEF:"%SRC%\bench_core.def" /RELEASE /INCREMENTAL:NO /NOLOGO
echo EXITCODE=%ERRORLEVEL%
endlocal
