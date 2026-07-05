<#
  run_loader_harness.ps1 -- run the Milestone 3 data-table loader harness.

  The harness (d2moo_loader_harness.exe, built by the CMake target of the same
  name under source/D2Common/tests) links D2MOO's real D2Common statically but
  calls into the REAL Storm.dll/Fog.dll to parse PD2's real MPQs. Those real
  DLLs are NOT on the exe's search path by default, and PD2's Storm.dll won't
  load standalone (its PD2_EXT.dll bootstrap fails outside Game.exe). This
  script stages the exact runtime deps next to the exe:
    - real Storm.dll / Fog.dll / D2CMP.dll / D2Lang.dll  (from ProjectD2)
    - PD2_EXT.dll  <=  a COPY OF THE WINDOWS version.dll, renamed.
        PD2's Storm.dll imports only 3 harmless version.dll funcs
        (GetFileVersionInfoA / *SizeA / VerQueryValueA) from "PD2_EXT.dll" -- a
        version.dll DLL-hijack the mod uses to bootstrap. A plain version.dll
        satisfies those imports with NO mod bootstrap, so Storm loads & parses.

  Output: prints the Experience table as JSON (compare vs pd2_experience.json,
  the live-game golden capture, with compare_fp.py-style diffing).
#>
param(
    [string]$ProjectD2 = 'C:\Diablo2\ProjectD2',
    [string]$BuildDir  = "$PSScriptRoot\..\..\build-1.13c"
)
$ErrorActionPreference = 'Stop'
$run = Join-Path $BuildDir 'source\D2Common\tests\Debug'
$exe = Join-Path $run 'd2moo_loader_harness.exe'
if (-not (Test-Path $exe)) { throw "Harness not built: $exe (build target d2moo_loader_harness)" }

foreach ($d in 'Storm.dll','Fog.dll','D2CMP.dll','D2Lang.dll') {
    Copy-Item (Join-Path $ProjectD2 $d) (Join-Path $run $d) -Force
}
# version.dll renamed -> PD2_EXT.dll (satisfies Storm's 3 version imports, no PD2 bootstrap)
Copy-Item "$env:WINDIR\SysWOW64\version.dll" (Join-Path $run 'PD2_EXT.dll') -Force

& $exe
