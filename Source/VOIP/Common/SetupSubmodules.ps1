# HoloCade VOIP - Setup Git Submodules Script
# 
# Initializes and sets up Steam Audio and Mumble submodules.
# Run this script after initializing your git repository.
#
# Usage:
#   .\Source\VOIP\Common\SetupSubmodules.ps1
#
# Prerequisites:
#   - Git repository must be initialized
#   - Run from HoloCade_Unreal/Plugins/HoloCade directory

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "HoloCade VOIP - Setup Git Submodules" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Get the script directory (Common folder)
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$PluginRoot = Split-Path -Parent (Split-Path -Parent $ScriptDir)

Write-Host "Plugin root: $PluginRoot" -ForegroundColor Gray
Write-Host ""

# Check if we're in a git repository
$GitRoot = git rev-parse --show-toplevel 2>$null
if (-not $GitRoot) {
    Write-Host "ERROR: Not in a git repository!" -ForegroundColor Red
    Write-Host ""
    Write-Host "Please initialize git repository first:" -ForegroundColor Yellow
    Write-Host "  1. Navigate to your repository root" -ForegroundColor White
    Write-Host "  2. Run: git init" -ForegroundColor White
    Write-Host "  3. Run this script again" -ForegroundColor White
    Write-Host ""
    exit 1
}

Write-Host "Git repository root: $GitRoot" -ForegroundColor Gray
Write-Host ""

# Change to plugin root
Push-Location $PluginRoot

try {
    # Ensure Plugins directory exists
    $PluginsDir = Join-Path $PluginRoot "Plugins"
    if (-not (Test-Path $PluginsDir)) {
        New-Item -ItemType Directory -Path $PluginsDir | Out-Null
        Write-Host "Created Plugins directory" -ForegroundColor Green
    }

    Write-Host "Setting up submodules..." -ForegroundColor Yellow
    Write-Host ""

    # Add Steam Audio submodule
    Write-Host "1. Adding Steam Audio submodule..." -ForegroundColor Yellow
    $SteamAudioPath = Join-Path $PluginsDir "SteamAudio"
    if (Test-Path $SteamAudioPath) {
        Write-Host "   ⚠ Steam Audio already exists at: $SteamAudioPath" -ForegroundColor Yellow
        Write-Host "     Skipping..." -ForegroundColor Gray
    } else {
        # Note: Steam Audio for Unreal may need a specific branch or fork
        # Valve's main repo: https://github.com/ValveSoftware/steam-audio.git
        # Check for Unreal-specific branch or fork
        $SteamAudioRepo = "https://github.com/ValveSoftware/steam-audio.git"
        Write-Host "   Adding submodule from: $SteamAudioRepo" -ForegroundColor Gray
        
        git submodule add $SteamAudioRepo $SteamAudioPath 2>&1 | Out-Null
        if ($LASTEXITCODE -eq 0) {
            Write-Host "   ✓ Steam Audio submodule added" -ForegroundColor Green
        } else {
            Write-Host "   ✗ Failed to add Steam Audio submodule" -ForegroundColor Red
            Write-Host "     You may need to add it manually:" -ForegroundColor Yellow
            Write-Host "     git submodule add $SteamAudioRepo $SteamAudioPath" -ForegroundColor Gray
        }
    }

    Write-Host ""

    # Add MumbleLink submodule
    Write-Host "2. Adding MumbleLink submodule..." -ForegroundColor Yellow
    $MumbleLinkPath = Join-Path $PluginsDir "MumbleLink"
    if (Test-Path $MumbleLinkPath) {
        Write-Host "   ⚠ MumbleLink already exists at: $MumbleLinkPath" -ForegroundColor Yellow
        Write-Host "     Skipping..." -ForegroundColor Gray
    } else {
        Write-Host "   ⚠ MumbleLink repository URL needs to be determined" -ForegroundColor Yellow
        Write-Host "     Options:" -ForegroundColor White
        Write-Host "     - Official Mumble C++: https://github.com/mumble-voip/mumble" -ForegroundColor Gray
        Write-Host "     - Unreal wrapper (may need to be created)" -ForegroundColor Gray
        Write-Host ""
        Write-Host "     For now, you'll need to add this manually:" -ForegroundColor Yellow
        Write-Host "     git submodule add <mumble-repo-url> $MumbleLinkPath" -ForegroundColor Gray
        Write-Host ""
        Write-Host "     Or create a wrapper plugin that interfaces with Mumble C++ library" -ForegroundColor Gray
    }

    Write-Host ""

    # Initialize all submodules
    Write-Host "3. Initializing submodules..." -ForegroundColor Yellow
    git submodule update --init --recursive 2>&1 | Out-Null
    if ($LASTEXITCODE -eq 0) {
        Write-Host "   ✓ Submodules initialized" -ForegroundColor Green
    } else {
        Write-Host "   ⚠ Some submodules may not have initialized" -ForegroundColor Yellow
    }

    Write-Host ""
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host "Setup complete!" -ForegroundColor Green
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "Next steps:" -ForegroundColor Yellow
    Write-Host "  1. Verify submodules are in Plugins/ directory" -ForegroundColor White
    Write-Host "  2. Open Unreal Editor and enable plugins:" -ForegroundColor White
    Write-Host "     - Edit → Plugins → Search 'Steam Audio' → Enable" -ForegroundColor Gray
    Write-Host "     - Edit → Plugins → Search 'MumbleLink' → Enable (if available)" -ForegroundColor Gray
    Write-Host "     - Edit → Plugins → Search 'HoloCade VOIP' → Enable" -ForegroundColor Gray
    Write-Host "  3. Rebuild project (if C++ changes)" -ForegroundColor White
    Write-Host "  4. Configure Steam Audio HRTF file in Project Settings" -ForegroundColor White
    Write-Host "  5. Test VOIP connection" -ForegroundColor White
    Write-Host ""

} finally {
    Pop-Location
}



