# HoloCade Server Manager

**UMG-Based Dedicated Server Management Interface**

## Overview

The HoloCade Server Manager is a graphical application built with Unreal Motion Graphics (UMG) for managing dedicated game servers. It provides a user-friendly interface for venue operators to:

- Start and stop dedicated server instances
- Monitor server status in real-time
- Configure server parameters (experience type, player count, port)
- Integrate with NVIDIA Omniverse Audio2Face
- View live server logs

## Architecture

The Server Manager consists of three main components:

1. **`AHoloCadeServerManagerGameMode`** - Game mode that initializes the UI
2. **`UHoloCadeServerManagerWidget`** - C++ widget base class with server control logic
3. **Blueprint Widget** - UMG visual design (created by developers)

```
AHoloCadeServerManagerGameMode
    └─> Creates & Displays
        └─> UHoloCadeServerManagerWidget (C++ Logic)
            └─> Extended by Blueprint (UMG Design)
```

## Setup Instructions

### Step 1: Build the Dedicated Server

Before using the Server Manager, you need a dedicated server executable:

1. Open `HoloCade_Unreal.sln` in Visual Studio
2. Set build configuration to **Development Server**
3. Build the project
4. Verify `Binaries/Win64/HoloCade_UnrealServer.exe` exists

### Step 2: Create a Server Manager Map

1. Open Unreal Editor
2. Create a new level: `File > New Level > Empty Level`
3. Save as `Maps/ServerManager.umap`
4. Set World Settings > GameMode Override to `HoloCadeServerManagerGameMode`

### Step 3: Create the Server Manager Widget Blueprint

1. **Create Widget Blueprint:**
   - Content Browser > Right-click > `User Interface > Widget Blueprint`
   - Name it `WBP_ServerManager`
   - Open the widget

2. **Set Parent Class:**
   - Click `File > Reparent Blueprint`
   - Select `HoloCadeServerManagerWidget`

3. **Design the UI** using UMG Canvas Panel with these key elements:

#### Configuration Panel
- **Dropdown (ComboBox)** for Experience Type
  - Bind options to `GetAvailableExperienceTypes()`
  - On selection: Set `ServerConfig.ExperienceType`
- **Text Box** for Server Name
  - Bind to `ServerConfig.ServerName`
- **Spinbox** for Max Players
  - Bind to `ServerConfig.MaxPlayers`
  - Range: 1-16
- **Spinbox** for Port
  - Bind to `ServerConfig.Port`
  - Range: 1024-65535

#### Control Panel
- **Button: Start Server**
  - On Clicked: Call `StartServer()`
  - Disabled if `ServerStatus.bIsRunning == true`
- **Button: Stop Server**
  - On Clicked: Call `StopServer()`
  - Disabled if `ServerStatus.bIsRunning == false`

#### Status Display
- **Text Block: Server Status**
  - Bind text to: `ServerStatus.bIsRunning ? "● Running" : "○ Stopped"`
  - Color: Green if running, Gray if stopped
- **Text Block: Player Count**
  - Bind text to: `FString::Printf("Players: %d/%d", ServerStatus.CurrentPlayers, ServerConfig.MaxPlayers)`
- **Text Block: Experience State**
  - Bind text to: `"State: " + ServerStatus.ExperienceState`
- **Text Block: Uptime**
  - Bind text to: Format `ServerStatus.Uptime` as HH:MM:SS

#### Omniverse Panel
- **Text Block: Omniverse Status**
  - Bind text to: `OmniverseStatus.bIsConnected ? "● Connected" : "○ Disconnected"`
- **Text Block: Stream Status**
  - Bind text to: `OmniverseStatus.StreamStatus`
- **Button: Configure Omniverse**
  - On Clicked: Call `OpenOmniverseConfig()`

#### Log Display
- **Scroll Box** with **Text Block** inside
  - Name the Text Block `LogTextBlock`
  - Implement custom event to append log messages
  - In C++, override `AddLogMessage()` to send to this text block

### Step 4: Configure the Game Mode

1. Open `Content/Blueprints` folder
2. Create Blueprint based on `HoloCadeServerManagerGameMode`
   - Or modify the default game mode in Project Settings
3. Set `Server Manager Widget Class` to your `WBP_ServerManager`

### Step 5: Test the Server Manager

1. Open the `ServerManager` map
2. Click **Play** (standalone mode)
3. Your Server Manager UI should appear
4. Try starting a server (requires dedicated server build)

## C++ API Reference

### `UHoloCadeServerManagerWidget` Functions

#### Server Control

```cpp
// Start the dedicated server with current configuration
bool StartServer();

// Stop the running dedicated server
bool StopServer();

// Update server status (called automatically via Tick)
void UpdateServerStatus();
```

#### Configuration

```cpp
// Get list of available experience types
TArray<FString> GetAvailableExperienceTypes() const;

// Current server configuration (editable)
FServerConfiguration ServerConfig;

// Current server status (read-only)
FServerStatus ServerStatus;
```

#### Omniverse Integration

```cpp
// Update Omniverse connection status
void UpdateOmniverseStatus();

// Open Omniverse configuration panel
void OpenOmniverseConfig();

// Omniverse status (read-only)
FOmniverseStatus OmniverseStatus;
```

#### Logging

```cpp
// Add a message to the log display
void AddLogMessage(const FString& Message);
```

## Command-Line Launch (Alternative)

For quick testing without the GUI, use the provided batch script:

**Windows:**
```batch
LaunchDedicatedServer.bat -experience AIFacemask -port 7777 -maxplayers 4
```

**Linux:**
```bash
./LaunchDedicatedServer.sh -experience AIFacemask -port 7777 -maxplayers 4
```

## Extending the Server Manager

### Adding Custom Server Parameters

1. Add properties to `FServerConfiguration` in `HoloCadeServerManagerWidget.h`
2. Update `BuildServerCommandLine()` to include new parameters
3. Add UI controls in your Blueprint widget

### Real-Time Status Updates (✅ Implemented)

The Server Manager now includes **real-time status updates** via the Network Beacon system:

**How it works:**
1. When started, the Server Manager creates a `UHoloCadeServerBeacon` in client mode
2. The beacon listens for UDP broadcasts from the managed server (port 7778)
3. When server broadcasts are received, `OnServerStatusReceived()` updates the UI
4. Player count and experience state update automatically in real-time

**Implementation:**
```cpp
// In UHoloCadeServerManagerWidget::NativeConstruct()
ServerBeacon = NewObject<UHoloCadeServerBeacon>(this, TEXT("ServerStatusBeacon"));
ServerBeacon->OnServerDiscovered.AddDynamic(this, &UHoloCadeServerManagerWidget::OnServerStatusReceived);
ServerBeacon->StartClientDiscovery(7778, 10.0f);

// Automatic callback updates UI
void UHoloCadeServerManagerWidget::OnServerStatusReceived(const FHoloCadeServerInfo& ServerInfo)
{
    ServerStatus.CurrentPlayers = ServerInfo.CurrentPlayers;
    ServerStatus.ExperienceState = ServerInfo.ExperienceState;
    // Logs significant changes automatically
}
```

**What you get:**
- ✅ Real-time player count updates
- ✅ Live experience state changes (Lobby → Act1 → Act2, etc.)
- ✅ Automatic server discovery (no manual IP entry)
- ✅ Automatic log messages for state changes

### Implementing Omniverse Integration

To connect to NVIDIA Omniverse Audio2Face:

1. Install [NVIDIA Omniverse](https://www.nvidia.com/en-us/omniverse/)
2. Use the Omniverse Connector SDK
3. Integrate in `UpdateOmniverseStatus()`:

```cpp
void UHoloCadeServerManagerWidget::UpdateOmniverseStatus()
{
    // TODO: Query Omniverse Nucleus for Audio2Face status
    // Example (pseudo-code):
    // OmniverseClient* Client = GetOmniverseClient();
    // OmniverseStatus.bIsConnected = Client->IsConnected();
    // OmniverseStatus.ActiveFaceStreams = Client->GetActiveFaceStreamCount();
}
```

## Troubleshooting

### Server Executable Not Found

**Error:** `ERROR: Server executable not found at Binaries/Win64/HoloCade_UnrealServer.exe`

**Solution:**
1. Verify you've built the `Development Server` configuration
2. Check the build output for errors
3. Ensure `HoloCade_UnrealServer.Target.cs` exists in `Source/`

### Server Starts But No Status Updates

**Cause:** Status polling is not implemented yet (currently just checks process alive)

**Solution:** Implement real-time status updates using Network Beacon (see "Extending" section above)

### UI Not Appearing

**Cause:** Widget class not set in Game Mode

**Solution:**
1. Open your `HoloCadeServerManagerGameMode` Blueprint
2. Set `Server Manager Widget Class` to your `WBP_ServerManager`
3. Ensure the Game Mode is set in World Settings

## Future Enhancements

- [ ] Real-time log streaming from server process
- [ ] Multiple concurrent server management
- [ ] Performance graphs (CPU, memory, network)
- [ ] Automatic crash recovery
- [ ] Remote server management over LAN
- [ ] Integration with venue hardware (hydraulics status)
- [ ] Experience state visualization
- [ ] Player count history graphs

## License

Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

---

**Need Help?** Open an issue at [github.com/scifiuiguy/HoloCade_Unreal/issues](https://github.com/scifiuiguy/HoloCade_Unreal/issues)

