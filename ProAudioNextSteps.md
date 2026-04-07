# Pro Audio UMG Templates - Implementation Guide

## Overview

This document outlines the implementation steps for creating UMG widget templates that provide a virtual sound board interface in the Command Console, synchronized bidirectionally with physical pro audio consoles via OSC.

## Current Implementation Status

✅ **ProAudioController Backend (Complete)**
- OSC client/server implemented
- Bidirectional sync delegates ready:
  - `OnChannelFaderChanged(Channel, Level)`
  - `OnChannelMuteChanged(Channel, bMute)`
  - `OnMasterFaderChanged(Level)`
- Virtual-to-physical channel mapping system (`RegisterChannelForSync`)
- Channel validation (max channels per console type)
- Address parsing for multiple manufacturers (Behringer, Yamaha, Allen & Heath, etc.)
- Channel offset support (0-based vs 1-based indexing)
- "Other" option for unsupported hardware (64 channels, generic paths)
- "Custom" option with XX/YY placeholder patterns

🔄 **UMG Templates (Next Step)**
- Template widgets need to be created
- Delegate bindings need to be implemented
- Two-way sync (UI ↔ Physical) needs to be wired
- Console type dropdown needs to be added
- Custom OSC pattern input fields need to be added

## UMG Template Requirements

### Widget Structure

```
ProAudioMixerPanel (Widget Blueprint)
├── MasterSection
│   ├── MasterLabel: "MASTER"
│   └── MasterFader: ProgressBar or Slider (0-1 range)
├── ChannelsContainer (ScrollBox)
│   └── ChannelRow (Widget Blueprint - reusable instance)
│       ├── ChannelNumberLabel: "CH 1"
│       ├── ChannelFader: ProgressBar or Slider (0-1 range)
│       ├── MuteButton: Toggle Button
│       └── DeleteButton: Button
└── AddChannelButton: Button
```

### Minimum Requirements for Testing

1. **Master Fader**
   - Visual slider/bar (0.0 to 1.0 range)
   - Bind to `ProAudioController->OnMasterFaderChanged`
   - On user interaction: Call `ProAudioController->SetMasterFader(Value)`

2. **Channel Faders (Minimum 2 for testing)**
   - Each channel needs:
     - **Channel Number Display**: Label showing "CH 1", "CH 2", etc.
     - **Physical Channel Input**: Small text input field for entering physical board channel number
     - **Sync/Register Button**: Circular arrow button to register virtual channel to physical channel
     - **Fader Control**: Slider or ProgressBar (0.0 to 1.0 range)
     - **Mute Button**: Toggle button
     - **Delete Button**: Remove channel from panel
   - Each channel widget must:
     - On sync button click: 
       - Read physical channel number from text input
       - Call `bool Success = ProAudioController->RegisterChannelForSync(VirtualChannelNumber, PhysicalChannelNumber)`
       - Display error if `Success == false` (channel out of range, invalid, etc.)
     - Bind to `ProAudioController->OnChannelFaderChanged` with channel-specific handler (check Channel parameter matches this widget's virtual channel)
     - Bind to `ProAudioController->OnChannelMuteChanged` with channel-specific handler (check Channel parameter matches this widget's virtual channel)
     - On fader user interaction: Call `ProAudioController->SetChannelFader(VirtualChannelNumber, Value)`
     - On mute button click: Call `ProAudioController->SetChannelMute(VirtualChannelNumber, bMute)`

3. **Channel Management**
   - **+ Add Channel Button**: Creates new `ChannelRow` widget instance
   - **Delete Button (per channel)**: Removes channel row, calls `UnregisterChannelForSync`

### Two-Way Sync Implementation

#### Physical Board → UMG (Automatic)
- Physical board sends OSC message
- `ProAudioController` receives via bound event handlers
- Appropriate delegate fires (`OnChannelFaderChanged`, `OnChannelMuteChanged`, etc.)
- UMG widget's bound function called
- Update visual state (slider position, mute button state)

#### UMG → Physical Board (User Interaction)
- User moves fader slider in UMG
- OnValueChanged event fires
- Call `ProAudioController->SetChannelFader(ChannelNumber, NewValue)`
- OSC message sent to physical board
- Physical board fader moves

**Important**: Prevent feedback loops! When delegate fires (physical → UI update), temporarily disable the OnValueChanged handler or use a flag to prevent re-sending to board.

## Implementation Steps

### Step 1: Create ChannelRow Widget Blueprint

1. Create new Widget Blueprint: `WBP_ProAudioChannelRow`
2. **Properties to add:**
   - `VirtualChannelNumber` (int32, exposed to designer) - The UMG channel number (1, 2, 3...)
   - `ProAudioController` (ProAudioController reference, exposed to designer)
   - `PhysicalChannelInput` (EditableText, for user to enter physical board channel number)
3. **Visual Elements:**
   - Text Block: `ChannelNumberLabel` (bind to "CH " + VirtualChannelNumber)
   - EditableText: `PhysicalChannelInput` (small text field for entering physical channel number)
   - Button: `SyncButton` (circular arrow icon, labeled "Sync" or "Register")
   - Slider: `ChannelFader` (0.0 to 1.0 range)
   - Toggle Button: `MuteButton` (visual pressed/checked state)
   - Button: `DeleteButton`
4. **Event Graph Setup:**
   - **OnConstruct**: 
     - Set channel label text to "CH " + VirtualChannelNumber
     - Set PhysicalChannelInput placeholder text to "Physical CH..."
     - Bind delegates (but don't register channel yet - user must click Sync button):
       - `ProAudioController->OnChannelFaderChanged.AddDynamic(this, &WBP_ProAudioChannelRow::OnPhysicalFaderChanged)`
       - `ProAudioController->OnChannelMuteChanged.AddDynamic(this, &WBP_ProAudioChannelRow::OnPhysicalMuteChanged)`
   - **OnSyncButtonClicked**:
     - Get text from `PhysicalChannelInput`
     - Convert to int32 (physical channel number)
     - Validate: Check if > 0, check if <= `ProAudioController->GetMaxChannelsForConsole()`
     - Call `bool Success = ProAudioController->RegisterChannelForSync(VirtualChannelNumber, PhysicalChannelNumber)`
     - If Success: Show success indicator, disable input field, enable fader/mute controls
     - If Failed: Show error message (channel out of range, invalid console, etc.)
   - **OnChannelFaderValueChanged** (user moved slider):
     - Get slider value
     - Call `ProAudioController->SetChannelFader(ChannelNumber, Value)`
   - **OnMuteButtonClicked**:
     - Get current mute state (toggle)
     - Call `ProAudioController->SetChannelMute(ChannelNumber, bMute)`
   - **OnPhysicalFaderChanged** (delegate callback):
     - Check if Channel parameter matches this widget's ChannelNumber
     - Update slider value (disable value change event temporarily to prevent loop)
   - **OnPhysicalMuteChanged** (delegate callback):
     - Check if Channel parameter matches this widget's ChannelNumber
     - Update mute button pressed state
   - **OnDeleteButtonClicked**:
     - Call `ProAudioController->UnregisterChannelForSync(ChannelNumber)`
     - Remove self from parent container

### Step 2: Create Master Fader Widget (Optional - can be in main panel)

1. **MasterFaderSlider** (Slider, 0.0 to 1.0 range)
2. **Event Graph:**
   - **OnConstruct**:
     - Bind to `ProAudioController->OnMasterFaderChanged.AddDynamic(this, &WBP_ProAudioMixerPanel::OnPhysicalMasterFaderChanged)`
   - **OnMasterFaderValueChanged** (user moved):
     - Call `ProAudioController->SetMasterFader(Value)`
   - **OnPhysicalMasterFaderChanged** (delegate callback):
     - Update slider value (disable value change event temporarily)

### Step 3: Create Main Mixer Panel Widget

1. Create Widget Blueprint: `WBP_ProAudioMixerPanel`
2. **Properties:**
   - `ProAudioController` (ProAudioController reference, exposed to designer)
   - `Channels` (Array of WBP_ProAudioChannelRow, for tracking)
3. **Visual Layout:**
   - Vertical Box (root)
   - **Console Selection Section (Horizontal Box)**:
     - Label: "Console Type:"
     - ComboBox/Dropdown: `ConsoleTypeDropdown` (bind to ProAudioController->Config.ConsoleType enum)
     - **Custom Pattern Fields (Hidden by default, shown when Custom selected)**:
       - EditableText: `CustomFaderPatternInput` (bind to ProAudioController->Config.CustomFaderPattern)
       - Label: "OSC Fader Pattern: /ch/XX/fader"
       - EditableText: `CustomMutePatternInput` (bind to ProAudioController->Config.CustomMutePattern)
       - Label: "OSC Mute Pattern: /ch/XX/mute"
       - EditableText: `CustomBusPatternInput` (bind to ProAudioController->Config.CustomBusSendPattern)
       - Label: "OSC Bus Pattern: /ch/XX/bus/YY/level"
       - EditableText: `CustomMasterPatternInput` (bind to ProAudioController->Config.CustomMasterPattern)
       - Label: "OSC Master Pattern: /master/fader"
   - Master Section (Horizontal Box)
   - ScrollBox: `ChannelsContainer`
   - Button: `AddChannelButton`
4. **Event Graph:**
   - **OnConstruct**:
     - Populate `ConsoleTypeDropdown` with all console type options (Behringer X32, Yamaha QL, Other, Custom, etc.)
     - Bind `ConsoleTypeDropdown` selection change to update `ProAudioController->Config.ConsoleType`
     - Show/hide Custom pattern fields based on selection (use IsVisible binding or custom event)
   - **OnConsoleTypeChanged**:
     - Update `ProAudioController->Config.ConsoleType`
     - If Custom: Show custom pattern input fields
     - If not Custom: Hide custom pattern input fields
   - **OnAddChannelClicked**:
     - Determine next virtual channel number (highest existing + 1, or start at 1)
     - Create new `WBP_ProAudioChannelRow` widget instance
     - Set `VirtualChannelNumber` property
     - Set `ProAudioController` reference
     - Add to `ChannelsContainer` ScrollBox
     - Add to `Channels` array for tracking

### Step 4: Integration into Command Console

1. Open Command Console Widget (`UHoloCadeServerManagerWidget` Blueprint)
2. Add `ProAudioController` component or reference
3. Add `WBP_ProAudioMixerPanel` widget to UI layout
4. Set `ProAudioController` reference in mixer panel properties
5. Configure `ProAudioController`:
   - Set `ConsoleType` via dropdown in mixer panel UI (or set default)
   - Set `BoardIPAddress`
   - Set `OSCPort` (default 10023 for X32)
   - Enable `bEnableReceive` for bidirectional sync
   - Set `ReceivePort` (default 8000)
   - If using Custom console type:
     - Configure `CustomFaderPattern` (use XX as channel placeholder)
     - Configure `CustomMutePattern` (use XX as channel placeholder)
     - Configure `CustomBusSendPattern` (use XX for channel, YY for bus)
     - Configure `CustomMasterPattern`

## Testing Checklist

### Physical → UI Sync
- [ ] Move fader on physical X32 board
- [ ] UMG channel fader slider moves to match
- [ ] Move different channel fader on X32
- [ ] Correct UMG channel fader updates (not wrong channel)
- [ ] Toggle mute on physical board
- [ ] UMG mute button state toggles correctly
- [ ] Move master fader on physical board
- [ ] UMG master fader updates

### UI → Physical Sync
- [ ] Move UMG channel fader slider
- [ ] Physical board channel fader moves
- [ ] Click UMG mute button
- [ ] Physical board channel mutes/unmutes
- [ ] Move UMG master fader
- [ ] Physical board master fader moves

### Multi-Channel Testing
- [ ] Add multiple channels (at least 2)
- [ ] Each channel operates independently
- [ ] Move channel 1 fader → only channel 1 responds
- [ ] Move channel 2 fader → only channel 2 responds
- [ ] No cross-talk between channels

### Channel Management
- [ ] Add channel button creates new channel row
- [ ] Channel numbers are sequential and correct
- [ ] Delete button removes channel
- [ ] Deleted channel stops receiving updates

## Known Limitations / Future Enhancements

- **Bus Sends**: Not yet implemented in delegates (only logged)
  - Future: Add `OnChannelBusSendChanged` delegate
  - Add bus send controls to channel row widget

- **OSC API Verification**: Actual Unreal OSC plugin API calls may need adjustment
  - `Message.GetFloat(0)`, `Message.GetInt32(0)`, `Message.GetAddress()` may have different signatures
  - Test and adjust in `ProAudioController.cpp` if compilation fails

- **Console-Specific Features**: Some consoles may have additional controls
  - EQ controls
  - Pan/balance
  - Solo buttons
  - These can be added as additional delegates and widgets

## Notes

- **Feedback Loop Prevention**: Critical to prevent infinite loops when updating UI from delegate callbacks
  - Use a boolean flag or disable event handlers temporarily during delegate-driven updates
  - Example: Set `bUpdatingFromDelegate = true`, update widget, set back to `false`

- **Virtual-to-Physical Channel Mapping**:
  - Virtual channels are the UMG widget channel numbers (1, 2, 3...)
  - Physical channels are the hardware board channel numbers (1-32 for X32, 1-64 for Wing, etc.)
  - User must explicitly map each virtual channel to a physical channel via the sync button
  - Mapping is stored in `VirtualToPhysicalChannelMap`: `VirtualChannelNumber -> PhysicalChannelNumber`
  - `RegisterChannelForSync(VirtualChannel, PhysicalChannel)` returns `false` if:
    - PhysicalChannel <= 0
    - PhysicalChannel > GetMaxChannelsForConsole()
  - Each virtual channel must be registered before it will sync

- **Channel Numbering**: Supports both 0-based and 1-based indexing
  - Most consoles use 1-based (Channel 1 = /ch/01/), so `ChannelOffset = 0` (default)
  - Some consoles use 0-based (Channel 1 = /ch/00/), set `ChannelOffset = -1`
  - Offset is applied automatically: `OSCChannel = VirtualChannel + ChannelOffset`
  - Configure via `ProAudioController.Config.ChannelOffset`

- **Console Type Selection**:
  - Dropdown should list all supported console types
  - **Supported Consoles with Channel Limits**:
    - Behringer X32/M32: 32 channels
    - Behringer Wing: 48 channels
    - Yamaha QL/CL/TF: 64 channels (max of series)
    - Yamaha DM7: 96 channels
    - Allen & Heath SQ: 64 channels
    - Allen & Heath dLive: 128 channels
    - Soundcraft Si: 64 channels
    - PreSonus StudioLive: 32 channels
  - **"Other" Option**: Generic OSC paths, assumes 64 channels, no validation
    - Use for unsupported hardware that follows common `/ch/XX/fader` pattern
  - **"Custom" Option**: Full manual OSC path control
    - User provides patterns with `XX` as channel placeholder, `YY` as bus placeholder
    - Example: `/mix/input/XX/volume` becomes `/mix/input/05/volume` for channel 5
    - Patterns shown/hidden automatically when Custom is selected

- **Channel Validation**:
  - `RegisterChannelForSync()` validates physical channel number against console's max channels
  - Error logged and `false` returned if channel exceeds console limit
  - UMG widget should display error message to user if registration fails

- **ProAudioController Reference**: 
  - Can be component on same actor, or passed reference
  - Must be initialized before widget construction
  - Widget should handle null reference gracefully

- **Network Configuration**: 
  - Physical board must be configured to send OSC updates to Command Console's IP address
  - Receive port is set in `ProAudioController.Config.ReceivePort` (default 8000)
  - Board's OSC send settings should target: `<CommandConsoleIP>:<ReceivePort>`

