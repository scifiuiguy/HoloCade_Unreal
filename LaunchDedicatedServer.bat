@echo off
REM HoloCade Dedicated Server Launcher
REM Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

echo ========================================
echo HoloCade Dedicated Server Launcher
echo ========================================
echo.

REM Default configuration
set EXPERIENCE_TYPE=AIFacemask
set MAP_NAME=/Game/Maps/HoloCadeMap
set PORT=7777
set MAX_PLAYERS=4

REM Parse command-line arguments
:parse_args
if "%~1"=="" goto :start_server
if /i "%~1"=="-experience" (
    set EXPERIENCE_TYPE=%~2
    shift
    shift
    goto :parse_args
)
if /i "%~1"=="-port" (
    set PORT=%~2
    shift
    shift
    goto :parse_args
)
if /i "%~1"=="-maxplayers" (
    set MAX_PLAYERS=%~2
    shift
    shift
    goto :parse_args
)
shift
goto :parse_args

:start_server
echo Starting HoloCade Dedicated Server...
echo Experience Type: %EXPERIENCE_TYPE%
echo Map: %MAP_NAME%
echo Port: %PORT%
echo Max Players: %MAX_PLAYERS%
echo.

REM Build path to server executable
set SERVER_PATH="%~dp0Binaries\Win64\HoloCade_UnrealServer.exe"

REM Check if server executable exists
if not exist %SERVER_PATH% (
    echo ERROR: Server executable not found at %SERVER_PATH%
    echo.
    echo Please build the dedicated server target first:
    echo 1. Open HoloCade_Unreal.sln in Visual Studio
    echo 2. Set Configuration to "Development Server"
    echo 3. Build the project
    echo.
    pause
    exit /b 1
)

REM Launch the dedicated server
echo Launching server...
start "HoloCade Dedicated Server" %SERVER_PATH% %MAP_NAME% -server -log -port=%PORT% -ExperienceType=%EXPERIENCE_TYPE% -MaxPlayers=%MAX_PLAYERS%

echo.
echo Server launched successfully!
echo.
echo To stop the server, close the server window or use Task Manager.
echo.
pause



