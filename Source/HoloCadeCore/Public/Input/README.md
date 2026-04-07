# HoloCade Input Adapter

## Overview

`UHoloCadeInputAdapter` is a universal input component that handles all input sources for HoloCade experiences, with automatic authority checks, replication, and RPC routing.

**Works with:**
- ✅ **All Server Types** - Dedicated servers and listen servers
- ✅ **All Input Sources** - Embedded systems (ESP32), VR controllers, keyboard, gamepad, AI, custom
- ✅ **All Templates** - AIFacemask, FlightSim, MovingPlatform, Gunship, CarSim

**Key Features:**
- **Automatic Authority Checks** - No manual `HasAuthority()` needed
- **Automatic Replication** - State syncs to all clients
- **Automatic RPC Routing** - Client input sent to server seamlessly
- **Edge Detection** - Button press/release events
- **Event-Driven** - Delegate-based, no polling required
- **Separation of Concerns** - Input logic separate from experience logic

---

## Architecture

```
┌────────────────────────────────────────────────────┐
│  Input Sources (Any/All Can Be Active)            │
│  ─────────────────────────────────                 │
│  • Embedded Systems (ESP32, Arduino, etc.)         │
│  • VR Controllers (Meta, Vive, Index)              │
│  • Keyboard / Gamepad                              │
│  • AI / Scripted Events                            │
│  • Custom Blueprint / C++ Logic                    │
└──────────────────┬─────────────────────────────────┘
                   │
                   ▼
┌────────────────────────────────────────────────────┐
│  UHoloCadeInputAdapter (Reusable Component)         │
│  ───────────────────────────────────               │
│  • Authority check (HasAuthority?)                 │
│  • RPC routing (Client → Server)                   │
│  • State replication (Server → Clients)            │
│  • Edge detection (press/release)                  │
└──────────────────┬─────────────────────────────────┘
                   │
                   ▼  Replicates Automatically
┌────────────────────────────────────────────────────┐
│  Delegates Fire on ALL Machines                    │
│  ──────────────────────────────                    │
│  • OnButtonPressed(int32 ButtonIndex)              │
│  • OnButtonReleased(int32 ButtonIndex)             │
│  • OnAxisChanged(int32 AxisIndex, float Value)     │
└──────────────────┬─────────────────────────────────┘
                   │
                   ▼
┌────────────────────────────────────────────────────┐
│  Your Experience Template                          │
│  ────────────────────────                          │
│  void OnInputButtonPressed(int32 ButtonIndex)      │
│  {                                                  │
│      // Fires on server AND clients automatically  │
│      // No authority checks needed!                │
│      ExperienceLoop->AdvanceState();               │
│  }                                                  │
└────────────────────────────────────────────────────┘
```

### Design Principles

**Separation of Concerns:**
- **Input Adapter:** "Button 0 was pressed"
- **Experience Template:** "Button 0 means advance to next state"

**Dependency Inversion:**
- Experience depends on abstraction (InputAdapter), not implementation (specific input source)

**Open/Closed:**
- Open for extension (add new input sources by calling `InjectButtonPress()`)
- Closed for modification (no adapter changes needed)

**Single Responsibility:**
- InputAdapter: Reads input, handles networking, manages replication
- Experience: State management, game logic

---

## Quick Start

### Step 1: Add Component

```cpp
// In your experience header (e.g., AAIFacemaskExperience.h)
UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HoloCade|Components")
TObjectPtr<UHoloCadeInputAdapter> InputAdapter;

// In constructor
AAIFacemaskExperience::AAIFacemaskExperience()
{
    InputAdapter = CreateDefaultSubobject<UHoloCadeInputAdapter>(TEXT("InputAdapter"));
}
```

### Step 2: Configure

```cpp
bool AAIFacemaskExperience::InitializeExperienceImpl()
{
    if (!Super::InitializeExperienceImpl())
        return false;

    if (InputAdapter)
    {
        // Connect embedded device controller (ESP32, Arduino, etc.)
        InputAdapter->EmbeddedDeviceController = CostumeController;
        
        // Enable input sources
        InputAdapter->bEnableEmbeddedSystemInput = true;  // ESP32 buttons
        InputAdapter->bEnableVRControllerInput = true;    // VR (Blueprint override)
        
        // Configure channels
        InputAdapter->ButtonCount = 4;  // 4 buttons
        InputAdapter->AxisCount = 0;    // No analog axes
        
        // Bind to events
        InputAdapter->OnButtonPressed.AddDynamic(this, &AAIFacemaskExperience::OnInputButtonPressed);
    }

    return true;
}
```

### Step 3: Respond to Input

```cpp
void AAIFacemaskExperience::OnInputButtonPressed(int32 ButtonIndex)
{
    // ✅ Fires on server AND clients automatically
    // ✅ No authority checks needed
    // ✅ Already replicated by InputAdapter
    
    UE_LOG(LogTemp, Log, TEXT("Button %d pressed"), ButtonIndex);
    
    if (ButtonIndex == 0 || ButtonIndex == 2)  // Forward buttons
        ExperienceLoop->AdvanceState();
    
    if (ButtonIndex == 1 || ButtonIndex == 3)  // Backward buttons
        ExperienceLoop->RetreatState();
}
```

**That's it!** ~7 lines of code for full multiplayer input support.

---

## Input Sources

### 1. Embedded Systems (ESP32, Arduino, STM32)

**Automatic** - Just connect the controller:

```cpp
InputAdapter->EmbeddedDeviceController = CostumeController;
InputAdapter->bEnableEmbeddedSystemInput = true;
```

The adapter will:
- Read button/axis states every frame
- Detect edges (press/release events)
- Replicate to all clients
- Fire delegates on all machines

**Supported:**
- Digital inputs (`GetDigitalInput()` → button press/release)
- Analog inputs (`GetAnalogInput()` → axis value changes)
- Any protocol (WiFi, Serial, Bluetooth, Ethernet)

---

### 2. Enhanced Input (Gamepad, Keyboard, Mouse)

**Use the HoloCadePlayerController helper class** for automatic Enhanced Input integration:

**Setup in Editor:**
1. Create Input Actions (e.g., `IA_Button0`, `IA_Button1`, `IA_Axis0`)
2. Create an Input Mapping Context and assign gamepad/keyboard bindings
3. Use `AHoloCadePlayerController` as your PlayerController class (or Blueprint child)
4. Assign Input Actions and Mapping Context to controller properties

**Example Input Mappings:**
```
IA_Button0 → Gamepad Face Button Bottom (A/Cross)
IA_Button1 → Gamepad Face Button Right (B/Circle)
IA_Button2 → Keyboard: 1
IA_Button3 → Keyboard: 2
IA_Axis0   → Gamepad Left Thumbstick X-Axis
IA_Axis1   → Gamepad Left Thumbstick Y-Axis
```

**Automatic Routing:**
The `HoloCadePlayerController` automatically:
- Finds the current experience in the world
- Routes all Enhanced Input actions to `InputAdapter->InjectButtonPress()` / `InjectAxisValue()`
- Handles button press/release edge detection
- Supports 8 digital buttons and 4 analog axes out of the box

**Use Cases:**
- ✅ Development testing with gamepad before ESP32 hardware is available
- ✅ Listen server hosts using keyboard/mouse instead of VR controllers
- ✅ Rapid prototyping without physical hardware setup

**Note:** In production LBE venues, dedicated servers read directly from ESP32. This is optional for development only.

---

### 3. VR Controllers (Listen Server Hosts)

**Blueprint Override** - Override `ProcessVRControllerInput`:

**Example Blueprint:**
```
Event ProcessVRControllerInput (from InputAdapter)
  ├─ Get Motion Controller (Right)
  ├─ Get Trigger Value
  ├─ Branch: Value > 0.5
  │   └─ True: Inject Button Press (Index: 0)
  │   └─ False: Inject Button Release (Index: 0)
```

**C++ Example:**
```cpp
// Override in your experience subclass
void AAIFacemaskExperience_VR::ProcessVRControllerInput_Implementation()
{
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC) return;

    // Right grip button = Forward
    if (PC->IsInputKeyDown(EKeys::MotionController_Right_Grip1))
        InputAdapter->InjectButtonPress(0);
    else
        InputAdapter->InjectButtonRelease(0);
    
    // Left trigger = Backward
    if (PC->IsInputKeyDown(EKeys::MotionController_Left_Trigger))
        InputAdapter->InjectButtonPress(1);
    else
        InputAdapter->InjectButtonRelease(1);
}
```

**Why Blueprint?**
- VR controller mappings vary by platform (Meta, Vive, Index)
- Developers can customize without C++ changes
- Easy to test different button layouts

---

### 4. AI / Scripted Events

```cpp
// In AI controller, sequencer, or game logic
void TriggerNextState()
{
    if (ShouldAdvanceExperience())
        Experience->InputAdapter->InjectButtonPress(0);
}
```

**Use cases:**
- AI-driven narrative progression
- Timed events
- Trigger volumes
- Interactive objects

---

## API Reference

### Public Methods

```cpp
// Inject input (works on server or client)
void InjectButtonPress(int32 ButtonIndex);
void InjectButtonRelease(int32 ButtonIndex);
void InjectAxisValue(int32 AxisIndex, float Value);

// Query current state (for polling)
bool IsButtonPressed(int32 ButtonIndex) const;
float GetAxisValue(int32 AxisIndex) const;
```

### Delegates

```cpp
// Subscribe to these in your experience
UPROPERTY(BlueprintAssignable)
FOnInputButtonPressed OnButtonPressed;      // void(int32 ButtonIndex)

UPROPERTY(BlueprintAssignable)
FOnInputButtonReleased OnButtonReleased;    // void(int32 ButtonIndex)

UPROPERTY(BlueprintAssignable)
FOnInputAxisChanged OnAxisChanged;          // void(int32 AxisIndex, float Value)
```

### Configuration Properties

```cpp
// Embedded device reference
UPROPERTY(EditAnywhere, BlueprintReadWrite)
TObjectPtr<UEmbeddedDeviceController> EmbeddedDeviceController;

// Enable/disable input sources
UPROPERTY(EditAnywhere, BlueprintReadWrite)
bool bEnableEmbeddedSystemInput = true;

UPROPERTY(EditAnywhere, BlueprintReadWrite)
bool bEnableVRControllerInput = false;

// Channel counts (0-32)
UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1", ClampMax = "32"))
int32 ButtonCount = 4;

UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "32"))
int32 AxisCount = 0;
```

---

## How It Works

### Authority (Server / Listen Server Host)

```
InputAdapter->TickComponent()
  └─ if (HasAuthority())
      ├─ ProcessEmbeddedSystemInput()      ← Read ESP32 buttons
      │   └─ for each button:
      │       └─ if (state changed):
      │           ├─ ReplicatedButtonStates |= (1 << index)
      │           └─ OnButtonPressed.Broadcast(index)
      │
      └─ ProcessVRControllerInput()         ← Blueprint override
          └─ InjectButtonPress(0)
              └─ UpdateButtonState()
                  ├─ ReplicatedButtonStates |= (1 << 0)
                  └─ OnButtonPressed.Broadcast(0)
```

### Clients (No Authority)

```
OnRep_ButtonStates() fires automatically
  └─ for each button:
      └─ if (replicated != previous):
          └─ OnButtonPressed.Broadcast(index)
```

### Client Requesting Input

```
Experience->InputAdapter->InjectButtonPress(0)
  └─ InputAdapter checks authority:
      ├─ Has authority?  → UpdateButtonState() directly
      └─ No authority?   → ServerInjectButtonPress() RPC
                           └─ Server executes → UpdateButtonState()
                               └─ Replicates to all clients
```

**Key Point:** You never need to check authority or write RPCs yourself!

---

## Dedicated vs. Listen Server

### Dedicated Server

```
Server PC (No HMD)
  └─ InputAdapter reads ESP32 buttons
      └─ Updates replicated state
          └─ All clients receive state updates

HMD Clients
  └─ Receive replicated button states
      └─ OnButtonPressed fires automatically
```

**Use case:** Production LBE venues with physical wrist buttons

---

### Listen Server

```
Host PC (with HMD)
  └─ InputAdapter reads:
      ├─ ESP32 buttons (if connected)
      └─ VR controllers (Blueprint override)
          └─ Both work simultaneously!

HMD Clients
  └─ Receive replicated button states
      └─ OnButtonPressed fires automatically
```

**Use case:** Development/testing with VR controllers

---

## Template Examples

### AIFacemask Experience

```cpp
// 4 wrist buttons (2 left, 2 right)
InputAdapter->ButtonCount = 4;
InputAdapter->bEnableEmbeddedSystemInput = true;  // ESP32
InputAdapter->bEnableVRControllerInput = true;    // VR testing

void AAIFacemaskExperience::OnInputButtonPressed(int32 ButtonIndex)
{
    if (ButtonIndex == 0 || ButtonIndex == 2) ExperienceLoop->AdvanceState();
    if (ButtonIndex == 1 || ButtonIndex == 3) ExperienceLoop->RetreatState();
}
```

---

### Flight Sim Experience

```cpp
// HOTAS controls: 16 buttons + 4 axes
InputAdapter->ButtonCount = 16;  // Fire, landing gear, etc.
InputAdapter->AxisCount = 4;     // Throttle, pitch, roll, yaw

void AFlightSimExperience::OnInputButtonPressed(int32 ButtonIndex)
{
    if (ButtonIndex == 0) FireWeapon();
    if (ButtonIndex == 1) ToggleLandingGear();
}

void AFlightSimExperience::OnInputAxisChanged(int32 AxisIndex, float Value)
{
    if (AxisIndex == 0) SetThrottle(Value);
    if (AxisIndex == 1) SetPitch(Value);
}
```

---

### Moving Platform Experience

```cpp
// Operator control panel: 8 buttons + emergency stop
InputAdapter->ButtonCount = 9;

void AMovingPlatformExperience::OnInputButtonPressed(int32 ButtonIndex)
{
    if (ButtonIndex == 8) EmergencyStop();  // Red button
    else ExecuteMovementSequence(ButtonIndex);
}
```

---

## Benefits

### For Developers
✅ ~7 lines of code per template (vs. ~80 lines with manual implementation)  
✅ No authority checks required  
✅ No RPC code required  
✅ No replication code required  
✅ Blueprint-friendly  

### For Templates
✅ Reusable across all experiences  
✅ Decoupled from experience logic  
✅ Easy to add new input sources  

### For Deployment
✅ Dedicated server support (ESP32, operator panels)  
✅ Listen server support (VR controllers, testing)  
✅ Mixed input (ESP32 + VR simultaneously)  

---

## Code Location

**Component Header:** `HoloCadeInputAdapter.h` (this directory)  
**Component Implementation:** `../../Private/Input/HoloCadeInputAdapter.cpp`  
**Enhanced Input Helper:** `HoloCadePlayerController.h` (this directory)  
**Helper Implementation:** `../../Private/Input/HoloCadePlayerController.cpp`

---

## Next Steps

1. **Refactor AIFacemaskExperience** - Replace current input code with InputAdapter
2. **Add VR Controller Blueprint** - Override `ProcessVRControllerInput` for testing
3. **Test Dedicated Server** - Verify ESP32 button input works
4. **Test Listen Server** - Verify VR controller input works
5. **Apply to Other Templates** - FlightSim, MovingPlatform, Gunship, CarSim

---

## See Also

- **../../HoloCadeExperiences/MULTIPLAYER_TODO.md** - Next steps for multiplayer (Steps 1, 2, 5, 6)
- **../../EmbeddedSystems/README.md** - How to connect ESP32, Arduino, etc.
- **HoloCadeInputAdapter.h** - Full C++ API documentation with inline comments

