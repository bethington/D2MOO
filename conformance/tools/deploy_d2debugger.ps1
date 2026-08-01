# deploy_d2debugger.ps1 -- stage a freshly built D2Debugger.dll and relaunch PD2.
#
# The narrow sibling of deploy_module_rva_fix.ps1: that one exists for a
# specific two-DLL fix, this one is for the ordinary case of "I changed
# D2Debugger and want to see it". D2Debugger.dll is LOADED, not hot-reloadable,
# so the game has to go down and come back up.
#
# GRACEFUL FIRST. The oracle's own /action/exit-game is preferred over
# Stop-Process: a forced kill can leave the Detours launcher holding the patch
# DLLs, and the copy below then fails with a sharing violation that looks like a
# permissions problem.
#
# -SeedSnap <anchor> writes a starting anchor for the Game panel into imgui.ini
# while the game is DOWN -- it has to be down, because ImGui rewrites that file
# on exit and would otherwise clobber the edit on its way out.
#
# Self-elevates (one UAC prompt): PD2 runs elevated, so both stopping it and
# launching it need admin.
param(
    [string]$Config   = 'Release',
    [string]$SeedSnap = '',
    [switch]$NoLaunch
)
$ErrorActionPreference = 'Stop'

$root     = 'C:\Users\benam\source\cpp\D2MOO'
$game     = 'C:\Diablo2\ProjectD2\Game.exe'
$launcher = "$root\build-1.13c\external\D2.Detours\source\Release\D2.DetoursLauncher.exe"
$patchDir = "$root\build-1.13c\patch"
$ini      = 'C:\Diablo2\ProjectD2\imgui.ini'
$oracle   = 'http://127.0.0.1:8790'
$src      = "$root\build-1.13c\source\D2Debugger\$Config\D2Debugger.dll"

$principal = New-Object Security.Principal.WindowsPrincipal(
    [Security.Principal.WindowsIdentity]::GetCurrent())
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    $argv = @('-NoProfile','-ExecutionPolicy','Bypass','-File', "`"$PSCommandPath`"",
              '-Config', $Config)
    if ($SeedSnap) { $argv += @('-SeedSnap', $SeedSnap) }
    if ($NoLaunch) { $argv += '-NoLaunch' }
    Start-Process powershell.exe -Verb RunAs -ArgumentList $argv
    return
}

# ---- ELEVATED from here ----
if (-not (Test-Path $src)) {
    Write-Host "ERROR: missing build artifact: $src" -ForegroundColor Red
    Write-Host '  Build first:' -ForegroundColor Yellow
    Write-Host "    cmake --build build-1.13c --config $Config --target D2Debugger"
    exit 1
}

Write-Host '[1/4] asking the game to exit gracefully...'
try {
    Invoke-WebRequest -Uri "$oracle/action/exit-game" -Method POST -TimeoutSec 5 `
        -Body '{"confirm":true}' -ContentType 'application/json' -UseBasicParsing | Out-Null
    Write-Host '      requested /action/exit-game'
} catch {
    Write-Host '      oracle did not answer (already down) -- continuing'
}
for ($i = 0; $i -lt 30; $i++) {
    if (-not (Get-Process Game -ErrorAction SilentlyContinue)) { break }
    Start-Sleep -Milliseconds 500
}
if (Get-Process Game -ErrorAction SilentlyContinue) {
    Write-Host '      still running after 15s -- stopping it'
    Get-Process Game,'Diablo II' -ErrorAction SilentlyContinue |
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
$dst = Join-Path $patchDir 'D2Debugger.dll'
if (Test-Path $dst) {
    Copy-Item $dst "$dst.bak_$stamp" -Force
    Write-Host "      backup -> D2Debugger.dll.bak_$stamp"
}
Copy-Item $src $dst -Force
Write-Host ("      D2Debugger.dll <- {0} ({1} bytes)" -f $src, (Get-Item $dst).Length)
$pdb = [IO.Path]::ChangeExtension($src, '.pdb')
if (Test-Path $pdb) { Copy-Item $pdb (Join-Path $patchDir 'D2Debugger.pdb') -Force }

# ---- optional: seed the Game panel's snap anchor ---------------------------
# Only while the game is DOWN. ImGui writes imgui.ini on exit, so an edit made
# with it running is overwritten by the version held in memory.
if ($SeedSnap) {
    if (-not (Test-Path $ini)) {
        Write-Host "      note: $ini does not exist yet -- creating it"
        New-Item -ItemType File -Path $ini -Force | Out-Null
    }
    Copy-Item $ini "$ini.bak_$stamp" -Force
    $lines = @(Get-Content $ini)
    # Drop any existing [D2PanelSnap][Anchors] block, then append a fresh one.
    $out = New-Object System.Collections.Generic.List[string]
    $skip = $false
    foreach ($l in $lines) {
        if ($l -match '^\[') { $skip = ($l -eq '[D2PanelSnap][Anchors]') }
        if (-not $skip) { $out.Add($l) }
    }
    while ($out.Count -gt 0 -and $out[$out.Count-1] -eq '') { $out.RemoveAt($out.Count-1) }
    $out.Add('')
    $out.Add('[D2PanelSnap][Anchors]')
    $out.Add("Game=$SeedSnap")
    $out.Add('')
    Set-Content -Path $ini -Value $out -Encoding ASCII
    Write-Host "      seeded imgui.ini: Game=$SeedSnap"
}

if ($NoLaunch) {
    Write-Host '[3/4] -NoLaunch: stopping here.'
    exit 0
}

Write-Host '[3/4] launching PD2 (debugger on :8790)...'
$env:DIABLO2_PATCH = $patchDir
$env:D2_DEBUGGER   = '1'
Start-Process -FilePath $launcher -ArgumentList @("`"$game`"", '--', '-w')

Write-Host '[4/4] waiting for the oracle...'
$ok = $false
for ($i = 0; $i -lt 20; $i++) {
    Start-Sleep -Seconds 2
    try {
        $st = (Invoke-WebRequest -Uri "$oracle/status" -TimeoutSec 4 -UseBasicParsing).Content | ConvertFrom-Json
        if ($st.ok) { $ok = $true; break }
    } catch { }
}
if ($ok) {
    Write-Host '      oracle is up.' -ForegroundColor Green
} else {
    Write-Host '      oracle did not come up within 40s -- check the game window.' -ForegroundColor Yellow
}
Start-Sleep -Seconds 3
