@echo off
REM Build the five video-DLL stubs, 32-bit to match Game.exe.
REM
REM   D2DDraw / D2Direct3D / D2Glide : the one NONAME ordinal-10000 export that
REM                                    every real backend has, plus 128K of
REM                                    .text padding for SGD2FreeRes to patch.
REM   ddraw / glide3x                : load-only wrappers, same padding.
REM
REM Output in out\ -- installing into the game dir is install_stubs.bat.
setlocal
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat" >nul 2>&1
cd /d "%~dp0"
if not exist out mkdir out

for %%N in (D2DDraw D2Direct3D D2Glide) do (
  cl /nologo /O1 /W3 /MT /LD /Fo:out\ /Fe:out\%%N.dll stub_backend.c ^
     /link /SUBSYSTEM:WINDOWS /NOENTRY:NO ^
     "/EXPORT:D2StubDriverEntry=_D2StubDriverEntry@0,@10000,NONAME"
  if errorlevel 1 ( echo BUILD FAILED: %%N & exit /b 1 )
)

cl /nologo /O1 /W3 /MT /LD /Fo:out\ /Fe:out\ddraw.dll stub_wrapper.c ^
   /link /SUBSYSTEM:WINDOWS ^
   /EXPORT:DirectDrawCreate=_DirectDrawCreate@12 ^
   /EXPORT:DirectDrawCreateEx=_DirectDrawCreateEx@16 ^
   /EXPORT:DirectDrawEnumerateA=_DirectDrawEnumerateA@8
if errorlevel 1 ( echo BUILD FAILED: ddraw & exit /b 1 )

cl /nologo /O1 /W3 /MT /LD /Fo:out\ /Fe:out\glide3x.dll stub_wrapper.c ^
   /link /SUBSYSTEM:WINDOWS
if errorlevel 1 ( echo BUILD FAILED: glide3x & exit /b 1 )

echo ---- results ----
for %%N in (D2DDraw D2Direct3D D2Glide ddraw glide3x) do (
  for %%S in (out\%%N.dll) do echo   %%N.dll  %%~zS bytes
)
echo ---- D2DDraw exports ----
dumpbin /exports out\D2DDraw.dll | findstr /C:"10000"
echo BUILD_EXIT=0
