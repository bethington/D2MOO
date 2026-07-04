<#
.SYNOPSIS
    D2MOO Build and Launch Tool

.DESCRIPTION
    This script helps build, install, and launch D2MOO with Diablo II.

.PARAMETER Action
    The action to perform: Build, Launch, BuildAndLaunch, Install, Clean

.PARAMETER Version
    D2 version to target: 1.10f, 1.13c, 1.14d (default: 1.13c)

.PARAMETER Config
    Build configuration: Release, Debug (default: Release)

.PARAMETER DiabloPath
    Path to Diablo II installation

.PARAMETER EnableDebugger
    Enable the D2MOO debugger UI

.PARAMETER GameArgs
    Additional arguments to pass to the game

.EXAMPLE
    .\D2MOO-Tool.ps1 -Action Build -Version 1.13c

.EXAMPLE
    .\D2MOO-Tool.ps1 -Action Launch -EnableDebugger

.EXAMPLE
    .\D2MOO-Tool.ps1 -Action BuildAndLaunch -Version 1.13c -EnableDebugger
#>

param(
    [Parameter(Mandatory=$true)]
    [ValidateSet("Build", "Launch", "BuildAndLaunch", "Install", "Clean", "Status")]
    [string]$Action,

    [ValidateSet("1.00", "1.10f", "1.13c", "1.14d")]
    [string]$Version = "1.13c",

    [ValidateSet("Release", "Debug")]
    [string]$Config = "Release",

    [string]$DiabloPath = "",

    [switch]$EnableDebugger,

    [string]$GameArgs = "-w"
)

# Configuration
$D2MOO_ROOT = $PSScriptRoot
$BUILD_DIR = "$D2MOO_ROOT\out\build\VS2022"
$INSTALL_DIR = "$D2MOO_ROOT\out\install\VS2022"
$PRESET = "VS2022"

# Colors for output
function Write-Header($text) {
    Write-Host ""
    Write-Host "============================================" -ForegroundColor Cyan
    Write-Host " $text" -ForegroundColor Cyan
    Write-Host "============================================" -ForegroundColor Cyan
    Write-Host ""
}

function Write-Success($text) {
    Write-Host "[OK] $text" -ForegroundColor Green
}

function Write-Error($text) {
    Write-Host "[ERROR] $text" -ForegroundColor Red
}

function Write-Info($text) {
    Write-Host "[INFO] $text" -ForegroundColor Yellow
}

# Find Diablo II installation
function Find-DiabloPath {
    $possiblePaths = @(
        $DiabloPath,
        "C:\Program Files (x86)\Diablo II",
        "C:\Program Files\Diablo II",
        "C:\Games\Diablo II",
        "D:\Games\Diablo II",
        "C:\Diablo II"
    )

    foreach ($path in $possiblePaths) {
        if ($path -and (Test-Path "$path\Game.exe")) {
            return $path
        }
    }

    # Try registry
    try {
        $regPath = Get-ItemProperty -Path "HKLM:\SOFTWARE\WOW6432Node\Blizzard Entertainment\Diablo II" -ErrorAction SilentlyContinue
        if ($regPath -and $regPath.InstallPath -and (Test-Path "$($regPath.InstallPath)\Game.exe")) {
            return $regPath.InstallPath
        }
    } catch {}

    return $null
}

# Build D2MOO
function Build-D2MOO {
    Write-Header "Building D2MOO for version $Version ($Config)"

    # Check for CMake
    if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
        Write-Error "CMake not found. Please install CMake and add it to PATH."
        return $false
    }

    # Update submodules
    Write-Info "Updating git submodules..."
    Push-Location $D2MOO_ROOT
    git submodule update --init --recursive
    Pop-Location

    # Configure
    Write-Info "Configuring CMake for $Version..."
    $configResult = cmake --preset $PRESET -DD2MOO_ORDINALS_VERSION="$Version" 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Error "CMake configuration failed"
        Write-Host $configResult
        return $false
    }
    Write-Success "Configuration complete"

    # Build
    Write-Info "Building $Config configuration..."
    $buildResult = cmake --build --preset $PRESET --config $Config 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Build failed"
        Write-Host $buildResult
        return $false
    }

    Write-Success "Build complete!"
    return $true
}

# Install D2MOO
function Install-D2MOO {
    Write-Header "Installing D2MOO"

    $installResult = cmake --build --preset $PRESET --config $Config --target install 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Installation failed"
        Write-Host $installResult
        return $false
    }

    Write-Success "Installation complete to: $INSTALL_DIR"
    return $true
}

# Launch D2MOO
function Launch-D2MOO {
    Write-Header "Launching D2MOO"

    # Find Diablo II
    $d2Path = Find-DiabloPath
    if (-not $d2Path) {
        Write-Error "Diablo II installation not found."
        Write-Host "Please specify the path using -DiabloPath parameter"
        return $false
    }
    Write-Success "Found Diablo II at: $d2Path"

    # Find launcher
    $launcherExe = "$BUILD_DIR\external\D2.Detours\source\$Config\D2.DetoursLauncher.exe"
    if (-not (Test-Path $launcherExe)) {
        Write-Error "D2.DetoursLauncher.exe not found at: $launcherExe"
        Write-Host "Please build D2MOO first using: .\D2MOO-Tool.ps1 -Action Build"
        return $false
    }
    Write-Success "Found launcher: $launcherExe"

    # Set patch directory
    $patchDir = "$BUILD_DIR\D2.Detours.patches\$Config\patch"
    if (-not (Test-Path $patchDir)) {
        Write-Error "Patch directory not found at: $patchDir"
        Write-Host "Please build D2MOO first."
        return $false
    }
    Write-Success "Patch directory: $patchDir"

    # Set environment variables
    $env:DIABLO2_PATCH = $patchDir

    if ($EnableDebugger) {
        $env:D2_DEBUGGER = "1"
        Write-Info "Debugger: ENABLED"
    } else {
        $env:D2_DEBUGGER = "0"
        Write-Info "Debugger: DISABLED"
    }

    Write-Info "Game arguments: $GameArgs"
    Write-Host ""
    Write-Host "Starting Diablo II with D2MOO..." -ForegroundColor Green
    Write-Host ""

    # Launch
    $gameExe = "$d2Path\Game.exe"
    $arguments = @($gameExe, "--") + ($GameArgs -split " ")

    Start-Process -FilePath $launcherExe -ArgumentList $arguments -Wait

    return $true
}

# Show status
function Show-Status {
    Write-Header "D2MOO Status"

    Write-Host "D2MOO Root: $D2MOO_ROOT"
    Write-Host "Build Dir:  $BUILD_DIR"
    Write-Host "Install Dir: $INSTALL_DIR"
    Write-Host ""

    # Check build
    $launcherExe = "$BUILD_DIR\external\D2.Detours\source\Release\D2.DetoursLauncher.exe"
    if (Test-Path $launcherExe) {
        Write-Success "Release build exists"
    } else {
        Write-Info "Release build not found"
    }

    $launcherExeDbg = "$BUILD_DIR\external\D2.Detours\source\Debug\D2.DetoursLauncher.exe"
    if (Test-Path $launcherExeDbg) {
        Write-Success "Debug build exists"
    } else {
        Write-Info "Debug build not found"
    }

    # Check D2 installation
    $d2Path = Find-DiabloPath
    if ($d2Path) {
        Write-Success "Diablo II found at: $d2Path"
    } else {
        Write-Error "Diablo II installation not found"
    }
}

# Clean build
function Clean-Build {
    Write-Header "Cleaning D2MOO Build"

    if (Test-Path $BUILD_DIR) {
        Remove-Item -Path $BUILD_DIR -Recurse -Force
        Write-Success "Removed build directory"
    }

    if (Test-Path $INSTALL_DIR) {
        Remove-Item -Path $INSTALL_DIR -Recurse -Force
        Write-Success "Removed install directory"
    }

    Write-Success "Clean complete"
}

# Main
switch ($Action) {
    "Build" {
        Build-D2MOO
    }
    "Launch" {
        Launch-D2MOO
    }
    "BuildAndLaunch" {
        if (Build-D2MOO) {
            Launch-D2MOO
        }
    }
    "Install" {
        if (Build-D2MOO) {
            Install-D2MOO
        }
    }
    "Clean" {
        Clean-Build
    }
    "Status" {
        Show-Status
    }
}
