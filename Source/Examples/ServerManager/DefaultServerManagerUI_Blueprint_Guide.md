# Default Server Manager UI - Blueprint Creation Guide

This guide will walk you through creating a functional default UMG interface for the HoloCade Server Manager.

## Final Result Preview

```
┌──────────────────────────────────────────────────────────────┐
│  HoloCade Server Manager                          [_][□][X]    │
├──────────────────────────────────────────────────────────────┤
│  ┌────────────────────────────────────────────────────────┐  │
│  │ Configuration                                          │  │
│  │                                                        │  │
│  │  Experience Type    [AIFacemask            ▼]         │  │
│  │  Server Name        [HoloCade Server         ]          │  │
│  │  Max Players        [4              ]                 │  │
│  │  Port               [7777           ]                 │  │
│  │  Map                [/Game/Maps/HoloCadeMap  ]          │  │
│  │                                                        │  │
│  │  [ START SERVER ]          [ STOP SERVER ]            │  │
│  └────────────────────────────────────────────────────────┘  │
│                                                              │
│  ┌────────────────────────────────────────────────────────┐  │
│  │ Server Status                                          │  │
│  │                                                        │  │
│  │  Status:  ● Running                                    │  │
│  │  Players: 2/4                                          │  │
│  │  State:   Act1                                         │  │
│  │  Uptime:  00:15:32                                     │  │
│  │  PID:     12345                                        │  │
│  └────────────────────────────────────────────────────────┘  │
│                                                              │
│  ┌────────────────────────────────────────────────────────┐  │
│  │ Omniverse Audio2Face                                   │  │
│  │                                                        │  │
│  │  Status:   ○ Not Connected                             │  │
│  │  Streams:  0 active                                    │  │
│  │                                                        │  │
│  │  [ CONFIGURE OMNIVERSE ]                               │  │
│  └────────────────────────────────────────────────────────┘  │
│                                                              │
│  ┌────────────────────────────────────────────────────────┐  │
│  │ Server Logs                                            │  │
│  │ ┌────────────────────────────────────────────────────┐ │  │
│  │ │ [12:30:15] Server Manager initialized              │ │  │
│  │ │ [12:30:15] Server status beacon initialized        │ │  │
│  │ │ [12:30:20] Server started successfully (PID: 12345)│ │  │
│  │ │ [12:30:22] Server state changed to: Lobby          │ │  │
│  │ │ [12:30:45] Player count changed to: 1/4            │ │  │
│  │ │ [12:31:02] Player count changed to: 2/4            │ │  │
│  │ └────────────────────────────────────────────────────┘ │  │
│  └────────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────────┘
```

---

## Step-by-Step Creation

### Step 1: Create the Widget Blueprint

1. In Content Browser, navigate to `Content/UI/ServerManager/`
2. Right-click → **User Interface** → **Widget Blueprint**
3. Name it `WBP_ServerManager`
4. Open the widget

### Step 2: Reparent to C++ Base Class

1. Click **File** → **Reparent Blueprint**
2. Search for `HoloCadeServerManagerWidget`
3. Click **Select**

### Step 3: Set Canvas Panel Size

1. Select the root **Canvas Panel**
2. In Details panel:
   - **Size**: 800 x 900 (or fullscreen if preferred)
   - **Anchors**: Center
   - **Position**: 0, 0

---

## Widget Hierarchy

Create this exact hierarchy in the **Hierarchy** panel:

```
Canvas Panel (Root)
├─ Background_Image
├─ Title_TextBlock
├─ ConfigPanel_VerticalBox
│  ├─ ExperienceType_HorizontalBox
│  │  ├─ ExperienceLabel_TextBlock
│  │  └─ ExperienceType_ComboBox
│  ├─ ServerName_HorizontalBox
│  │  ├─ ServerNameLabel_TextBlock
│  │  └─ ServerName_EditableTextBox
│  ├─ MaxPlayers_HorizontalBox
│  │  ├─ MaxPlayersLabel_TextBlock
│  │  └─ MaxPlayers_SpinBox
│  ├─ Port_HorizontalBox
│  │  ├─ PortLabel_TextBlock
│  │  └─ Port_SpinBox
│  ├─ MapName_HorizontalBox
│  │  ├─ MapNameLabel_TextBlock
│  │  └─ MapName_EditableTextBox
│  └─ ButtonsRow_HorizontalBox
│     ├─ StartServer_Button
│     │  └─ StartServerText_TextBlock
│     └─ StopServer_Button
│        └─ StopServerText_TextBlock
├─ StatusPanel_VerticalBox
│  ├─ StatusHeader_TextBlock
│  ├─ StatusRunning_HorizontalBox
│  │  ├─ StatusLabel_TextBlock
│  │  └─ StatusValue_TextBlock
│  ├─ StatusPlayers_HorizontalBox
│  │  ├─ PlayersLabel_TextBlock
│  │  └─ PlayersValue_TextBlock
│  ├─ StatusState_HorizontalBox
│  │  ├─ StateLabel_TextBlock
│  │  └─ StateValue_TextBlock
│  ├─ StatusUptime_HorizontalBox
│  │  ├─ UptimeLabel_TextBlock
│  │  └─ UptimeValue_TextBlock
│  └─ StatusPID_HorizontalBox
│     ├─ PIDLabel_TextBlock
│     └─ PIDValue_TextBlock
├─ OmniversePanel_VerticalBox
│  ├─ OmniverseHeader_TextBlock
│  ├─ OmniverseStatus_HorizontalBox
│  │  ├─ OmniverseStatusLabel_TextBlock
│  │  └─ OmniverseStatusValue_TextBlock
│  ├─ OmniverseStreams_HorizontalBox
│  │  ├─ StreamsLabel_TextBlock
│  │  └─ StreamsValue_TextBlock
│  └─ ConfigureOmniverse_Button
│     └─ ConfigureOmniverseText_TextBlock
└─ LogPanel_VerticalBox
   ├─ LogHeader_TextBlock
   └─ LogScroll_ScrollBox
      └─ LogText_TextBlock
```

---

## Detailed Widget Configuration

### Background and Title

#### `Background_Image`
- **Type:** Image
- **Anchors:** Fill
- **Brush Color:** (0.05, 0.05, 0.05, 1.0) - Dark gray
- **Position:** 0, 0
- **Size:** Fill parent

#### `Title_TextBlock`
- **Type:** Text Block
- **Text:** `"HoloCade Server Manager"`
- **Font Size:** 32
- **Color:** (1.0, 1.0, 1.0, 1.0) - White
- **Anchors:** Top Center
- **Position:** 0, 20
- **Alignment:** Center

---

### Configuration Panel

#### `ConfigPanel_VerticalBox`
- **Type:** Vertical Box
- **Anchors:** Top
- **Position:** 50, 80
- **Size:** 700 x 200
- **Padding:** 10
- **Spacing:** 10

#### Experience Type Row (`ExperienceType_HorizontalBox`)
- **Child Fill:** Stretch

**`ExperienceLabel_TextBlock`:**
- Text: `"Experience Type:"`
- Font Size: 16
- Color: White
- Horizontal Alignment: Left
- Size to Content: True

**`ExperienceType_ComboBox`:**
- **Type:** Combo Box (String)
- **Default Options:** Add manually or bind to `GetAvailableExperienceTypes()`
- **On Selection Changed:** Set `ServerConfig.ExperienceType`
- **Widget Style:** Modern dropdown
- **Selected Item Binding:** `ServerConfig.ExperienceType`

#### Server Name Row (`ServerName_HorizontalBox`)

**`ServerNameLabel_TextBlock`:**
- Text: `"Server Name:"`
- Font Size: 16

**`ServerName_EditableTextBox`:**
- **Type:** Editable Text Box
- **Hint Text:** `"Enter server name..."`
- **Text Binding:** Bind to `ServerConfig.ServerName`
- **On Text Changed:** Set `ServerConfig.ServerName`

#### Max Players Row (`MaxPlayers_HorizontalBox`)

**`MaxPlayersLabel_TextBlock`:**
- Text: `"Max Players:"`
- Font Size: 16

**`MaxPlayers_SpinBox`:**
- **Type:** Spin Box
- **Value Binding:** Bind to `ServerConfig.MaxPlayers`
- **Min Value:** 1
- **Max Value:** 16
- **Min Slider Value:** 1
- **Max Slider Value:** 16
- **On Value Changed:** Set `ServerConfig.MaxPlayers`

#### Port Row (`Port_HorizontalBox`)

**`PortLabel_TextBlock`:**
- Text: `"Port:"`
- Font Size: 16

**`Port_SpinBox`:**
- **Type:** Spin Box
- **Value Binding:** Bind to `ServerConfig.Port`
- **Min Value:** 1024
- **Max Value:** 65535
- **On Value Changed:** Set `ServerConfig.Port`

#### Map Name Row (`MapName_HorizontalBox`)

**`MapNameLabel_TextBlock`:**
- Text: `"Map:"`
- Font Size: 16

**`MapName_EditableTextBox`:**
- **Type:** Editable Text Box
- **Text Binding:** Bind to `ServerConfig.MapName`
- **On Text Changed:** Set `ServerConfig.MapName`

#### Buttons Row (`ButtonsRow_HorizontalBox`)

**`StartServer_Button`:**
- **Type:** Button
- **Style:** Green tint when enabled, gray when disabled
- **Size:** 200 x 50
- **On Clicked:** Call `StartServer()`
- **Is Enabled Binding:** `NOT ServerStatus.bIsRunning`

**`StartServerText_TextBlock` (child of button):**
- Text: `"START SERVER"`
- Font Size: 18
- Color: White
- Bold: True
- Alignment: Center

**`StopServer_Button`:**
- **Type:** Button
- **Style:** Red tint when enabled, gray when disabled
- **Size:** 200 x 50
- **On Clicked:** Call `StopServer()`
- **Is Enabled Binding:** `ServerStatus.bIsRunning`

**`StopServerText_TextBlock` (child of button):**
- Text: `"STOP SERVER"`
- Font Size: 18
- Color: White
- Bold: True
- Alignment: Center

---

### Status Panel

#### `StatusPanel_VerticalBox`
- **Type:** Vertical Box
- **Anchors:** Top
- **Position:** 50, 300
- **Size:** 700 x 150
- **Background:** Border or background image with slight tint
- **Padding:** 15
- **Spacing:** 5

#### `StatusHeader_TextBlock`
- Text: `"Server Status"`
- Font Size: 20
- Color: (0.8, 0.9, 1.0, 1.0) - Light blue
- Bold: True

#### Status Rows (All similar structure)

**`StatusRunning_HorizontalBox` (and similar for Players, State, Uptime, PID):**

**Label TextBlock:**
- Text: `"Status:"` (or `"Players:"`, `"State:"`, etc.)
- Font Size: 14
- Color: (0.7, 0.7, 0.7, 1.0) - Gray
- Size to Content: True

**Value TextBlock:**
- **Text Binding Examples:**
  - **StatusValue:** `ServerStatus.bIsRunning ? "● Running" : "○ Stopped"`
  - **PlayersValue:** `FString::Printf("%d/%d", ServerStatus.CurrentPlayers, ServerConfig.MaxPlayers)`
  - **StateValue:** `ServerStatus.ExperienceState`
  - **UptimeValue:** Convert `ServerStatus.Uptime` to HH:MM:SS format using Blueprint function
  - **PIDValue:** `FString::Printf("PID: %d", ServerStatus.ProcessID)`
- Font Size: 14
- **Color Binding (for StatusValue only):**
  - Green `(0.2, 1.0, 0.2, 1.0)` if running
  - Gray `(0.5, 0.5, 0.5, 1.0)` if stopped

---

### Omniverse Panel

#### `OmniversePanel_VerticalBox`
- **Type:** Vertical Box
- **Anchors:** Top
- **Position:** 50, 470
- **Size:** 700 x 120
- **Background:** Border or background with tint
- **Padding:** 15
- **Spacing:** 5

#### `OmniverseHeader_TextBlock`
- Text: `"Omniverse Audio2Face"`
- Font Size: 20
- Color: (0.8, 0.9, 1.0, 1.0) - Light blue
- Bold: True

#### Omniverse Status Rows

**`OmniverseStatus_HorizontalBox`:**

**`OmniverseStatusLabel_TextBlock`:**
- Text: `"Status:"`
- Font Size: 14
- Color: Gray

**`OmniverseStatusValue_TextBlock`:**
- **Text Binding:** `OmniverseStatus.bIsConnected ? "● Connected" : "○ Not Connected"`
- Font Size: 14
- **Color Binding:**
  - Green if connected
  - Gray if disconnected

**`OmniverseStreams_HorizontalBox`:**

**`StreamsLabel_TextBlock`:**
- Text: `"Face Streams:"`
- Font Size: 14

**`StreamsValue_TextBlock`:**
- **Text Binding:** `FString::Printf("%d active", OmniverseStatus.ActiveFaceStreams)`
- Font Size: 14

#### `ConfigureOmniverse_Button`
- **Type:** Button
- **Style:** Blue tint
- **Size:** 250 x 40
- **On Clicked:** Call `OpenOmniverseConfig()`

**`ConfigureOmniverseText_TextBlock`:**
- Text: `"CONFIGURE OMNIVERSE"`
- Font Size: 14
- Color: White
- Alignment: Center

---

### Log Panel

#### `LogPanel_VerticalBox`
- **Type:** Vertical Box
- **Anchors:** Bottom Stretch
- **Position:** 50, 610
- **Size:** 700 x 250
- **Background:** Border with dark tint
- **Padding:** 10

#### `LogHeader_TextBlock`
- Text: `"Server Logs"`
- Font Size: 20
- Color: (0.8, 0.9, 1.0, 1.0) - Light blue
- Bold: True

#### `LogScroll_ScrollBox`
- **Type:** Scroll Box
- **Orientation:** Vertical
- **Size:** Fill parent (700 x 200)
- **Scroll Bar Visibility:** Always visible
- **Background:** Darker background

#### `LogText_TextBlock` (inside ScrollBox)
- **Type:** Text Block
- **Text:** Initially empty
- **Font:** Monospace (Courier or Consolas)
- **Font Size:** 12
- **Color:** (0.8, 1.0, 0.8, 1.0) - Light green (terminal style)
- **Auto Wrap Text:** True
- **Justification:** Left

---

## Blueprint Event Graph Setup

### Event: Add Log Message (Custom)

Create a custom event in the Event Graph:

1. **Right-click** → **Add Custom Event**
2. Name: `BP_AddLogMessage`
3. **Input:** `String` parameter named `Message`

**Blueprint Logic:**
```
BP_AddLogMessage (Message)
    |
    ├─ Get Current Time (Format as "[HH:MM:SS]")
    ├─ Append to String: "[HH:MM:SS] " + Message + "\n"
    ├─ Get LogText_TextBlock
    ├─ Append Text
    └─ Scroll LogScroll_ScrollBox to End
```

### Override: Add Log Message (C++)

Override the C++ `AddLogMessage` function to call your Blueprint event:

1. In **Functions** → **Override** → Select `AddLogMessage`
2. In the override:
   ```
   AddLogMessage (Inherited)
       |
       └─ Call BP_AddLogMessage (Message)
   ```

### Event: Format Uptime

Create a function to format uptime seconds to HH:MM:SS:

1. **Functions** → **Add Function**
2. Name: `FormatUptime`
3. **Input:** `Float` named `Seconds`
4. **Output:** `String` named `Formatted`

**Blueprint Logic:**
```
FormatUptime (Seconds)
    |
    ├─ Hours = Floor(Seconds / 3600)
    ├─ Minutes = Floor((Seconds % 3600) / 60)
    ├─ Secs = Floor(Seconds % 60)
    └─ Return: FString::Printf("%02d:%02d:%02d", Hours, Minutes, Secs)
```

Bind this to the `UptimeValue_TextBlock` text property.

---

## Testing the Widget

1. **Save** the widget blueprint
2. Open your `HoloCadeServerManagerGameMode` Blueprint
3. Set **Server Manager Widget Class** to `WBP_ServerManager`
4. **Play** in standalone mode
5. Test buttons and configuration

---

## Styling Tips

### Color Scheme (Recommended)

- **Background:** #0D0D0D (very dark gray)
- **Panel Backgrounds:** #1A1A1A (dark gray)
- **Primary Text:** #FFFFFF (white)
- **Secondary Text:** #B3B3B3 (light gray)
- **Accent Color:** #4A9EFF (blue)
- **Success Color:** #33FF66 (green)
- **Error Color:** #FF3366 (red)

### Font Recommendations

- **Title:** Roboto Bold, 32pt
- **Headers:** Roboto Bold, 20pt
- **Body Text:** Roboto Regular, 14-16pt
- **Logs:** Courier New, 12pt

---

## Advanced: Real-Time Binding

For smoother updates, you can use property bindings instead of manual updates:

### Example: Status Indicator Binding

1. Select `StatusValue_TextBlock`
2. Click **Bind** next to **Text** property
3. **Create Binding** → Select `Get ServerStatus` → `b Is Running`
4. Use **Select** node to return:
   - `"● Running"` if `true`
   - `"○ Stopped"` if `false`

Do this for all dynamic text fields for automatic updates!

---

## Troubleshooting

### Widget not showing?
- Check that `Server Manager Widget Class` is set in Game Mode
- Verify the map's World Settings uses `HoloCadeServerManagerGameMode`

### Buttons not working?
- Ensure `On Clicked` events are bound
- Check that functions are being called (add print statements)

### Text not updating?
- Use property bindings instead of manual text setting
- Ensure `NativeTick` is being called

---

## Next Steps

Once your UI is functional:
- Add visual polish (backgrounds, borders, icons)
- Implement tooltips for configuration options
- Add confirmation dialogs for critical actions
- Create a mini-map showing server location on network

---

**Need Help?** Refer to the C++ documentation in `ServerManager/README.md` or open an issue on GitHub!



