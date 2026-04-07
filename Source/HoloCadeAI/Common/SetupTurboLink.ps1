# HoloCade AI - Setup TurboLink Submodule Script
# 
# Initializes and sets up TurboLink gRPC plugin as a git submodule.
# Run this script after initializing your git repository.
#
# Usage:
#   .\Source\AI\Common\SetupTurboLink.ps1
#
# Prerequisites:
#   - Git repository must be initialized
#   - Run from HoloCade_Unreal root directory

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "HoloCade AI - Setup TurboLink" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Get the script directory (Common folder)
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$PluginRoot = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $ScriptDir))

Write-Host "Project root: $PluginRoot" -ForegroundColor Gray
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

# Change to project root
Push-Location $PluginRoot

try {
    # Ensure Plugins directory exists
    $PluginsDir = Join-Path $PluginRoot "Plugins"
    if (-not (Test-Path $PluginsDir)) {
        New-Item -ItemType Directory -Path $PluginsDir | Out-Null
        Write-Host "Created Plugins directory" -ForegroundColor Green
    }

    Write-Host "Setting up TurboLink submodule..." -ForegroundColor Yellow
    Write-Host ""

    # Add TurboLink submodule
    Write-Host "1. Adding TurboLink submodule..." -ForegroundColor Yellow
    $TurboLinkPath = Join-Path $PluginsDir "TurboLink"
    if (Test-Path $TurboLinkPath) {
        Write-Host "   ⚠ TurboLink already exists at: $TurboLinkPath" -ForegroundColor Yellow
        Write-Host "     Skipping..." -ForegroundColor Gray
    } else {
        $TurboLinkRepo = "https://github.com/thejinchao/turbolink.git"
        Write-Host "   Adding submodule from: $TurboLinkRepo" -ForegroundColor Gray
        
        git submodule add $TurboLinkRepo $TurboLinkPath 2>&1 | Out-Null
        if ($LASTEXITCODE -eq 0) {
            Write-Host "   ✓ TurboLink submodule added" -ForegroundColor Green
        } else {
            Write-Host "   ✗ Failed to add TurboLink submodule" -ForegroundColor Red
            Write-Host "     You may need to add it manually:" -ForegroundColor Yellow
            Write-Host "     git submodule add $TurboLinkRepo $TurboLinkPath" -ForegroundColor Gray
            Pop-Location
            exit 1
        }
    }

    Write-Host ""

    # Initialize submodule
    Write-Host "2. Initializing TurboLink submodule..." -ForegroundColor Yellow
    git submodule update --init --recursive $TurboLinkPath 2>&1 | Out-Null
    if ($LASTEXITCODE -eq 0) {
        Write-Host "   ✓ TurboLink submodule initialized" -ForegroundColor Green
    } else {
        Write-Host "   ⚠ TurboLink submodule may not have initialized" -ForegroundColor Yellow
    }

    Write-Host ""

    # Download ThirdParty libraries (CRITICAL - required for build)
    Write-Host "3. Downloading TurboLink ThirdParty libraries..." -ForegroundColor Yellow
    $ThirdPartyPath = Join-Path $TurboLinkPath "Source\ThirdParty"
    if (Test-Path "$ThirdPartyPath\grpc\lib\win64\Release\grpc.lib") {
        Write-Host "   ⚠ ThirdParty libraries already exist" -ForegroundColor Yellow
        Write-Host "     Skipping download..." -ForegroundColor Gray
    } else {
        Write-Host "   Downloading from GitHub releases..." -ForegroundColor Gray
        $ReleasesUrl = "https://api.github.com/repos/thejinchao/turbolink-libraries/releases"
        try {
            $Releases = Invoke-RestMethod -Uri $ReleasesUrl -ErrorAction Stop
            $Latest = $Releases[0]
            $Asset = $Latest.assets | Where-Object { $_.name -like "*ue54*" -or $_.name -like "*windows*" } | Select-Object -First 1
            
            if ($Asset) {
                Write-Host "   Found: $($Asset.name)" -ForegroundColor Gray
                $ZipPath = Join-Path $ThirdPartyPath "turbolink-libraries.zip"
                
                # Download
                Invoke-WebRequest -Uri $Asset.browser_download_url -OutFile $ZipPath -UseBasicParsing -ErrorAction Stop
                Write-Host "   ✓ Downloaded" -ForegroundColor Green
                
                # Extract
                Write-Host "   Extracting..." -ForegroundColor Gray
                Expand-Archive -Path $ZipPath -DestinationPath $ThirdPartyPath -Force -ErrorAction Stop
                Remove-Item $ZipPath -ErrorAction SilentlyContinue
                Write-Host "   ✓ Extracted" -ForegroundColor Green
            } else {
                Write-Host "   ✗ Could not find Windows release asset" -ForegroundColor Red
                Write-Host "     Please download manually from:" -ForegroundColor Yellow
                Write-Host "     https://github.com/thejinchao/turbolink-libraries/releases" -ForegroundColor Gray
            }
        } catch {
            Write-Host "   ✗ Failed to download ThirdParty libraries" -ForegroundColor Red
            Write-Host "     Error: $($_.Exception.Message)" -ForegroundColor Gray
            Write-Host "     Please download manually from:" -ForegroundColor Yellow
            Write-Host "     https://github.com/thejinchao/turbolink-libraries/releases" -ForegroundColor Gray
            Write-Host "     Extract to: $ThirdPartyPath" -ForegroundColor Gray
        }
    }

    Write-Host ""
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host "Setup complete!" -ForegroundColor Green
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "Next steps:" -ForegroundColor Yellow
    Write-Host "  1. Verify TurboLink is in Plugins/TurboLink/" -ForegroundColor White
    Write-Host "  2. Regenerate Visual Studio project files:" -ForegroundColor White
    Write-Host "     - Right-click .uproject → Generate Visual Studio project files" -ForegroundColor Gray
    Write-Host "  3. Build project (expect 1-2 days of compilation fixes for UE 5.5.4 compatibility)" -ForegroundColor White
    Write-Host "  4. Open Unreal Editor and enable plugin:" -ForegroundColor White
    Write-Host "     - Edit → Plugins → Search 'TurboLink' → Enable" -ForegroundColor Gray
    Write-Host "  5. Implement TurboLink integration in AIGRPCClient.cpp" -ForegroundColor White
    Write-Host "     - See TURBOLINK_GUIDE.md for detailed instructions" -ForegroundColor Gray
    Write-Host ""
    Write-Host "⚠️  IMPORTANT: TurboLink may require compatibility fixes for UE 5.5.4" -ForegroundColor Yellow
    Write-Host "   - Check TurboLink GitHub issues for UE 5.5 compatibility" -ForegroundColor Gray
    Write-Host "   - May need to update TurboLink's Build.cs files" -ForegroundColor Gray
    Write-Host "   - Protocol Buffer code generation may need path fixes" -ForegroundColor Gray
    Write-Host ""
    Write-Host "📖 Documentation: See TURBOLINK_GUIDE.md for complete setup guide" -ForegroundColor Cyan
    Write-Host ""

} finally {
    Pop-Location
}

