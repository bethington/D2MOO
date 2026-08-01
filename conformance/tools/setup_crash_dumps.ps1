<#
.SYNOPSIS
    Make Windows write a crash dump for Game.exe every time it faults.

.DESCRIPTION
    Everything we had before reported from INSIDE the game: the crash observer's
    JSON records, the EBP walk, the stack scan. That has two weaknesses which
    cost most of a night --

      * it cannot capture a crash that happens before or during our own init;
      * it is itself a suspect, so disabling it to test a theory also disables
        the only evidence being used to test it.

    Windows Error Reporting LocalDumps has neither problem. The dump is written
    by the OS after the process faults, with no code of ours involved, and it
    captures every crash including ones inside D2Debugger itself.

    DumpType 1 (mini) on purpose, not 2 (full). A minidump carries the thread
    stacks -- which is all a backtrace needs -- at a few MB, whereas full dumps
    of this game would be hundreds of MB each and DumpCount of them adds up
    fast. (A 2.8 GB behavioural log already had to be untracked from this repo
    once; large artefacts here are not hypothetical.)

    Idempotent: re-running only rewrites the same three values.

    Read the dumps with:  conformance/tools/analyze_crash_dump.py

.PARAMETER DumpFolder
    Where dumps land. Defaults beside the JSON crash records.

.PARAMETER DumpCount
    How many to keep before Windows recycles the oldest. Default 10.

.PARAMETER Remove
    Tear the configuration back down.
#>
[CmdletBinding()]
param(
    [string]$DumpFolder = 'C:\Users\benam\source\cpp\D2MOO\conformance\behavioral\crashes\dumps',
    [int]$DumpCount = 10,
    [switch]$Remove
)

$ErrorActionPreference = 'Stop'

# HKLM: this needs admin, and there is no per-user equivalent -- WER only reads
# LocalDumps from the machine hive.
function Test-Elevated {
    $id = [Security.Principal.WindowsIdentity]::GetCurrent()
    (New-Object Security.Principal.WindowsPrincipal $id).IsInRole(
        [Security.Principal.WindowsBuiltInRole]::Administrator)
}

if (-not (Test-Elevated)) {
    Write-Host 'Requesting administrator privileges (one-time registry write)...'
    $argList = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', "`"$PSCommandPath`"",
                 '-DumpFolder', "`"$DumpFolder`"", '-DumpCount', $DumpCount)
    if ($Remove) { $argList += '-Remove' }
    Start-Process powershell.exe -Verb RunAs -ArgumentList $argList -Wait
    exit
}

$root = 'HKLM:\SOFTWARE\Microsoft\Windows\Windows Error Reporting\LocalDumps'
$key  = Join-Path $root 'Game.exe'

if ($Remove) {
    if (Test-Path $key) { Remove-Item $key -Recurse -Force; Write-Host 'Game.exe dump config REMOVED.' }
    else { Write-Host 'Nothing to remove.' }
    exit
}

if (-not (Test-Path $root)) { New-Item -Path $root -Force | Out-Null }
if (-not (Test-Path $key))  { New-Item -Path $key  -Force | Out-Null }
if (-not (Test-Path $DumpFolder)) { New-Item -ItemType Directory -Path $DumpFolder -Force | Out-Null }

New-ItemProperty -Path $key -Name 'DumpFolder' -PropertyType ExpandString -Value $DumpFolder -Force | Out-Null
New-ItemProperty -Path $key -Name 'DumpCount'  -PropertyType DWord        -Value $DumpCount  -Force | Out-Null
New-ItemProperty -Path $key -Name 'DumpType'   -PropertyType DWord        -Value 1           -Force | Out-Null

Write-Host ''
Write-Host 'Windows will now write a minidump every time Game.exe crashes.'
Write-Host ("  folder : {0}" -f $DumpFolder)
Write-Host ("  keep   : {0} dumps (oldest recycled)" -f $DumpCount)
Write-Host  '  type   : 1 (mini -- thread stacks, a few MB each)'
Write-Host ''
Write-Host 'IMPORTANT: WER only writes a dump for an UNHANDLED exception. D2'
Write-Host 'installs its own filter and shows the "Diablo II Exception" box, which'
Write-Host 'HANDLES the fault -- so a dump appears only if that filter is bypassed.'
Write-Host 'Set D2DBG_NO_D2FILTER=1 to have D2Debugger suppress it and let the'
Write-Host 'fault reach WER.'
Write-Host ''
Write-Host 'Read a dump with:  python conformance/tools/analyze_crash_dump.py'
