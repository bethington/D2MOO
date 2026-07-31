# deploy_module_rva_fix.ps1 -- stage the RUNTIME-BASE (module+rva) fix and relaunch PD2.
#
# WHY THIS EXISTS
#   The conformance stack assumed every module loads at its Ghidra image base.
#   D2Common (0x6fd50000) and D2Game (0x6fc20000) do; D2Client does NOT -- the
#   live process maps it at 0x03600000 and leaves Ghidra's 0x6fab0000 unmapped.
#   So every D2Client /oracle call `call`ed unmapped memory, faulted, and came
#   back as the generic SEH "handler-exception", which fun-doc files as
#   `marshal_fault` ("wrong callconv/slot-count or a bad pointer arg") -- a
#   TERMINAL verdict. 104 D2Client functions were retired that way without their
#   reimpl ever executing once.
#
# WHAT THIS DEPLOYS (both need a relaunch -- they are loaded, not hot-reloadable)
#   D2Debugger.dll  (Release)         /oracle honours "module"+"rva", refuses a
#                                     target that is not mapped executable
#                                     ("bad-target"), adds GET /modules, and
#                                     advertises specModuleRva in /status.
#   D2Common.dll    (RelWithDebInfo)  D2MOO_ResolveGameFn adds the module's
#                                     RUNTIME base. This one is NOT optional:
#                                     provider_runtime.cpp asks the injected
#                                     resolver FIRST, so a stale patch DLL keeps
#                                     shadowing the provider's fixed table for
#                                     every name it knows -- which is all 3,399
#                                     D2Client entries.
#
#   D2MOO_ReimplProvider.dll is deliberately NOT deployed here: every prove with
#   --build rebuilds and stages it (prove_candidate.build_and_stage), so its
#   half of the fix lands on the next prove with no relaunch.
#
# AFTER RELAUNCH, verify in this order:
#   1. curl http://127.0.0.1:8790/status   -> "specModuleRva":true
#   2. curl http://127.0.0.1:8790/modules  -> D2Client's base (expect 0x03600000,
#      i.e. 56623104 -- but read it, do not assume; the whole bug was an assumed base)
#   3. python fun-doc/scripts/requeue_bad_target_failures.py    (no --module needed
#      once /modules answers; already applied for D2Client on 2026-07-30)
#   4. Load a character, then run ONE D2Client live prove and confirm it reaches a
#      real verdict (proven / mismatch) instead of live_prove_failed[marshal_fault].
#
# Self-elevates (one UAC). Backs up whatever it replaces.
$ErrorActionPreference = 'Stop'

$root     = 'C:\Users\benam\source\cpp\D2MOO'
$game     = 'C:\Diablo2\ProjectD2\Game.exe'
$launcher = "$root\build-1.13c\external\D2.Detours\source\Release\D2.DetoursLauncher.exe"
$patchDir = "$root\build-1.13c\patch"
$oracle   = 'http://127.0.0.1:8790'

# (built artifact, deployed name). Configs match what was already deployed:
# D2Debugger from Release, D2Common from RelWithDebInfo.
$artifacts = @(
    @{ Src = "$root\build-1.13c\source\D2Debugger\Release\D2Debugger.dll";        Name = 'D2Debugger.dll' }
    @{ Src = "$root\build-1.13c\source\D2Common\RelWithDebInfo\D2Common.dll";     Name = 'D2Common.dll'   }
)

$principal = New-Object Security.Principal.WindowsPrincipal(
    [Security.Principal.WindowsIdentity]::GetCurrent())
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Start-Process powershell.exe -Verb RunAs -ArgumentList @(
        '-NoProfile','-ExecutionPolicy','Bypass','-File', "`"$PSCommandPath`"")
    return
}

# ---- ELEVATED from here ----
foreach ($a in $artifacts) {
    if (-not (Test-Path $a.Src)) {
        Write-Host "ERROR: missing build artifact: $($a.Src)" -ForegroundColor Red
        Write-Host '  Build first:' -ForegroundColor Yellow
        Write-Host '    cmake --build build-1.13c --config Release        --target D2Debugger D2MOO_ReimplProvider'
        Write-Host '    cmake --build build-1.13c --config RelWithDebInfo --target D2Common'
        exit 1
    }
}

Write-Host '[1/4] asking the game to exit gracefully...'
# Prefer the oracle's own exit over Stop-Process: a forced kill can leave the
# Detours launcher holding the patch DLLs, which then fails the copy below.
try {
    Invoke-WebRequest -Uri "$oracle/action/exit-game" -Method POST -TimeoutSec 5 `
        -Body '{"confirm":true}' -ContentType 'application/json' -UseBasicParsing | Out-Null
    Write-Host '      requested /action/exit-game'
} catch {
    Write-Host '      oracle did not answer (already down, or no exit route) -- continuing'
}
for ($i = 0; $i -lt 30; $i++) {
    if (-not (Get-Process Game -ErrorAction SilentlyContinue)) { break }
    Start-Sleep -Milliseconds 500
}
if (Get-Process Game -ErrorAction SilentlyContinue) {
    Write-Host '      still running after 15s -- stopping it'
    Get-Process Game,'Diablo II',D2.DetoursLauncher -ErrorAction SilentlyContinue |
        Stop-Process -Force -ErrorAction SilentlyContinue
    for ($i = 0; $i -lt 40; $i++) {
        if (-not (Get-Process Game -ErrorAction SilentlyContinue)) { break }
        Start-Sleep -Milliseconds 500
    }
}
Get-Process D2.DetoursLauncher -ErrorAction SilentlyContinue |
    Stop-Process -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 1

$stamp = Get-Date -Format 'yyyyMMdd_HHmmss'
Write-Host '[2/4] deploying (backing up what is replaced)...'
foreach ($a in $artifacts) {
    $dst = Join-Path $patchDir $a.Name
    if (Test-Path $dst) {
        Copy-Item $dst "$dst.bak_$stamp" -Force
        Write-Host "      backup -> $($a.Name).bak_$stamp"
    }
    Copy-Item $a.Src $dst -Force
    Write-Host ("      {0} <- {1} ({2} bytes)" -f $a.Name, $a.Src, (Get-Item $dst).Length)
    $pdb = [IO.Path]::ChangeExtension($a.Src, '.pdb')
    if (Test-Path $pdb) {
        Copy-Item $pdb (Join-Path $patchDir ([IO.Path]::ChangeExtension($a.Name, '.pdb'))) -Force
    }
}

Write-Host '[3/4] launching PD2 (debugger on :8790)...'
$env:DIABLO2_PATCH = $patchDir
$env:D2_DEBUGGER   = '1'
Start-Process -FilePath $launcher -ArgumentList @("`"$game`"", '--', '-w')

Write-Host '[4/4] waiting for the oracle, then checking the new capability...'
$ok = $false
for ($i = 0; $i -lt 20; $i++) {
    Start-Sleep -Seconds 2
    try {
        $st = (Invoke-WebRequest -Uri "$oracle/status" -TimeoutSec 4 -UseBasicParsing).Content | ConvertFrom-Json
        if ($st.ok) {
            $ok = $true
            if ($st.specModuleRva) {
                Write-Host '      /status specModuleRva = true  (module+rva spec support live)' -ForegroundColor Green
            } else {
                Write-Host '      /status has NO specModuleRva -- the OLD D2Debugger is still loaded.' -ForegroundColor Red
                Write-Host '      fun-doc will refuse D2Client live proves until this reads true.' -ForegroundColor Red
            }
            break
        }
    } catch { }
}
if (-not $ok) {
    Write-Host '      oracle not up yet. If a Diablo II startup dialog appeared, close it and rerun.' -ForegroundColor Yellow
    exit 0
}

try {
    $mods = (Invoke-WebRequest -Uri "$oracle/modules" -TimeoutSec 5 -UseBasicParsing).Content | ConvertFrom-Json
    Write-Host ''
    Write-Host '      RUNTIME module bases (Ghidra base in parentheses):'
    $ghidra = @{ 'D2Client.dll' = 0x6FAB0000; 'D2Common.dll' = 0x6FD50000
                 'D2Game.dll'   = 0x6FC20000; 'Storm.dll'    = 0x6FBF0000 }
    foreach ($m in $mods.modules) {
        if (-not $ghidra.ContainsKey($m.name)) { continue }
        $g = $ghidra[$m.name]
        $tag = if ($m.base -eq $g) { 'at its base' } else { 'RELOCATED' }
        Write-Host ("        {0,-14} 0x{1:X8}  (0x{2:X8})  {3}" -f $m.name, $m.base, $g, $tag)
    }
} catch {
    Write-Host '      GET /modules failed -- the OLD D2Debugger is still loaded.' -ForegroundColor Red
}

Write-Host ''
Write-Host 'Next: load a character, then run ONE D2Client live prove and confirm it'
Write-Host 'reaches a real verdict rather than live_prove_failed[marshal_fault].'
