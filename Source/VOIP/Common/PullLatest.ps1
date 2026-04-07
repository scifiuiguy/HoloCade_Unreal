# HoloCade VOIP - Pull Latest Submodules Script
# 
# Updates Steam Audio and Mumble submodules to latest versions.
# Run this script to pull the latest changes from Valve and Mumble repositories.
#
# Usage:
#   .\Common\PullLatest.ps1
#
# Note: This script assumes submodules are already initialized.
# If submodules are not initialized, run:
#   git submodule update --init --recursive

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "HoloCade VOIP - Pull Latest Submodules" -ForegroundColor Cyan
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
    Write-Host "Please initialize git repository first, then run:" -ForegroundColor Yellow
    Write-Host "  git submodule update --init --recursive" -ForegroundColor Yellow
    exit 1
}

Write-Host "Git repository root: $GitRoot" -ForegroundColor Gray
Write-Host ""

# Change to plugin root
Push-Location $PluginRoot

try {
    # Update Steam Audio submodule
    Write-Host "Updating Steam Audio..." -ForegroundColor Yellow
    $SteamAudioPath = Join-Path $PluginRoot "Plugins" "SteamAudio"
    if (Test-Path $SteamAudioPath) {
        Push-Location $SteamAudioPath
        git pull origin main 2>&1 | Out-Null
        if ($LASTEXITCODE -eq 0) {
            Write-Host "  ✓ Steam Audio updated" -ForegroundColor Green
        } else {
            Write-Host "  ✗ Failed to update Steam Audio" -ForegroundColor Red
        }
        Pop-Location
    } else {
        Write-Host "  ⚠ Steam Audio submodule not found at: $SteamAudioPath" -ForegroundColor Yellow
        Write-Host "    Run: git submodule add <steam-audio-repo> Plugins/SteamAudio" -ForegroundColor Gray
    }

    Write-Host ""

    # Update MumbleLink submodule
    Write-Host "Updating MumbleLink..." -ForegroundColor Yellow
    $MumbleLinkPath = Join-Path $PluginRoot "Plugins" "MumbleLink"
    if (Test-Path $MumbleLinkPath) {
        Push-Location $MumbleLinkPath
        git pull origin main 2>&1 | Out-Null
        if ($LASTEXITCODE -eq 0) {
            Write-Host "  ✓ MumbleLink updated" -ForegroundColor Green
        } else {
            Write-Host "  ✗ Failed to update MumbleLink" -ForegroundColor Red
        }
        Pop-Location
    } else {
        Write-Host "  ⚠ MumbleLink submodule not found at: $MumbleLinkPath" -ForegroundColor Yellow
        Write-Host "    Run: git submodule add <mumble-link-repo> Plugins/MumbleLink" -ForegroundColor Gray
    }

    Write-Host ""

    # Update all submodules (if using git submodule update --remote)
    Write-Host "Updating all submodules..." -ForegroundColor Yellow
    git submodule update --remote 2>&1 | Out-Null
    if ($LASTEXITCODE -eq 0) {
        Write-Host "  ✓ All submodules updated" -ForegroundColor Green
    } else {
        Write-Host "  ⚠ Some submodules may not have updated" -ForegroundColor Yellow
    }

    Write-Host ""
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host "Update complete!" -ForegroundColor Green
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "Next steps:" -ForegroundColor Yellow
    Write-Host "  1. Reopen Unreal Engine project" -ForegroundColor White
    Write-Host "  2. Rebuild project (if C++ changes were pulled)" -ForegroundColor White
    Write-Host "  3. Test VOIP connection" -ForegroundColor White
    Write-Host ""

} finally {
    Pop-Location
}



