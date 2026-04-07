#!/bin/bash
# HoloCade Dedicated Server Launcher (Linux)
# Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

echo "========================================"
echo "HoloCade Dedicated Server Launcher"
echo "========================================"
echo ""

# Default configuration
EXPERIENCE_TYPE="AIFacemask"
MAP_NAME="/Game/Maps/HoloCadeMap"
PORT=7777
MAX_PLAYERS=4

# Parse command-line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -experience)
            EXPERIENCE_TYPE="$2"
            shift 2
            ;;
        -port)
            PORT="$2"
            shift 2
            ;;
        -maxplayers)
            MAX_PLAYERS="$2"
            shift 2
            ;;
        *)
            shift
            ;;
    esac
done

echo "Starting HoloCade Dedicated Server..."
echo "Experience Type: $EXPERIENCE_TYPE"
echo "Map: $MAP_NAME"
echo "Port: $PORT"
echo "Max Players: $MAX_PLAYERS"
echo ""

# Build path to server executable
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SERVER_PATH="$SCRIPT_DIR/Binaries/Linux/HoloCade_UnrealServer"

# Check if server executable exists
if [ ! -f "$SERVER_PATH" ]; then
    echo "ERROR: Server executable not found at $SERVER_PATH"
    echo ""
    echo "Please build the dedicated server target first."
    echo ""
    exit 1
fi

# Make executable if needed
chmod +x "$SERVER_PATH"

# Launch the dedicated server
echo "Launching server..."
"$SERVER_PATH" "$MAP_NAME" -server -log -port="$PORT" -ExperienceType="$EXPERIENCE_TYPE" -MaxPlayers="$MAX_PLAYERS"

echo ""
echo "Server stopped."



