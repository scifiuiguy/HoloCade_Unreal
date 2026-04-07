# Superhero Flight Experience

**Embedded firmware and hardware specifications for SuperheroFlightExperience dual-winch suspended harness system.**

This experience enables players to experience free-body flight (flying like Superman) using a redundant dual-winch system with 10-finger/arm gesture-based control. **Note:** This is distinct from `FlightSimExperience` (2DOF gyroscope HOTAS cockpit for jet/spaceship simulation). Superhero Flight uses gesture control only - no HOTAS, no button events, no 6DOF body tracking required.

---

## Overview

The Superhero Flight Experience uses a **redundant dual-winch system** to suspend players in a harness and enable five distinct flight modes:

1. **Standing Mode** - Player upright, feet on ground
2. **Hovering Mode** - Player lifted to `airHeight`, upright position
3. **Flight-Up Mode** - Player lifted to `airHeight`, upright, arms pointing up
4. **Flight-Forward Mode** - Player lifted to `proneHeight`, prone position, arms pointing forward
5. **Flight-Down Mode** - Player lifted to `airHeight`, upright, arms pointing down

### Control System

Control is based on two gesture inputs:
1. **Fist vs Open Hand** - Fist state (both hands closed = flight motion, single hand release = hover/stop)
2. **HMD-to-Hands Vector** - Distance/worldspace-relative angle between HMD-to-hands and world ground plane

The `UFlightHandsController` component (client-side on HMD) analyzes these gestures and converts them into control events that are replicated to the server via Unreal Replication.

---

## Hardware Architecture

> **📋 Hardware Specifications:** See **[Hardware_Specification.md](Hardware_Specification.md)** for complete hardware component list, wiring diagrams, calibration procedures, cost estimates, **truss design specifications**, and **regulatory compliance recommendations** for the custom DIY winch system.

### Truss Structure

The dual-winch system is supported by a custom Warren truss structure:
- **Design:** 8-foot-wide Warren truss with parallel square steel tube chords
- **Height:** 9-10 feet above player's head (target: 9', maximum: 10' - check local regulations for specific requirements)
- **Mounting:** Bolts to standard vertical truss mounting systems
- **Weight:** ~120 lbs (truss alone)
- **Battery Location:** Battery enclosure mounted near base of one truss leg

See `Hardware_Specification.md` → Truss Design & Structural Support for complete specifications.

### Dual-Winch System

The system uses **two winches per player** mounted to either side of a truss, spaced two feet apart:

- **Front Winch (Shoulder-Hook):**
  - Attaches to primary attachment point of EasyFit harness at shoulder-blade level
  - Carries primary weight of player
  - Position: `airHeight` for hovering/flight-up/flight-down, `airHeight` for flight-forward

- **Rear Winch (Pelvis-Hook):**
  - Attaches to apron-style harness swaddling player's legs and torso at pelvis level
  - Provides stability and prone positioning
  - Position: `airHeight` for hovering/flight-up/flight-down, `proneHeight` for flight-forward

**Default State (Standing Mode):**
- Front winch: Taught and ready to lift (at `standingGroundHeight`)
- Rear winch: Slack but tight enough to prevent sway (at `standingGroundHeight`)

**Winch Redundancy:**
- If one winch fails, system can operate in degraded mode with remaining winch (safety-critical)
- Both winches have independent motor controllers, position feedback, and tension monitoring

### Harness System

- **EasyFit Harness** (Petzl EasyFit or similar) - Primary safety harness
  - Primary attachment point at shoulder-blade level (front winch)
  - Full-body fall arrest protection

- **Apron-Style Harness** - Secondary harness for prone positioning
  - Swaddles player's legs and torso
  - Attachment point at pelvis level (rear winch)
  - Enables transition from upright to prone position

### Height Calibration System

**433MHz wireless up-down clicker** (portable remote, carried by Ops Tech) for on-the-fly player height calibration during harness strapping:

- **433MHz USB Receiver Dongle** connected directly to server PC (no custom PCB needed)
- Ops Tech uses wireless clicker to send up/down commands while strapping player
- Commands received by USB dongle and routed directly to game server
- Server sends winch position commands to main Superhero Flight ECU
- Once calibration complete, Ops Tech queues experience to begin
- Server acknowledges current winch positions as baseline `standingGroundHeight`

**Hardware:**
- **433MHz Wireless Remote** (up/down buttons) - Standard 433MHz RF remote transmitter
- **433MHz USB Receiver Dongle** - USB dongle that plugs into server PC (e.g., RTL-SDR, CC1101 USB module, or similar)
- **No Custom PCB Required** - Uses off-the-shelf USB receiver modules

**⚠️ Security Considerations:**

The `RF433MHz` low-level API provides comprehensive security features (rolling code validation, replay attack prevention, AES encryption). See the main README (`HoloCade_Unreal/Plugins/HoloCade/README.md`) → RF433MHz API → Security Features for complete documentation.

**Physical Safety Interlocks (Required at Experience Level):**

The `RF433MHz` API provides button events - the experience must enforce safety. `SuperheroFlightExperience` implements the following interlocks:

1. **Calibration Mode Only** - RF button events only processed when `playSessionActive = false`
2. **Movement Limits** - Winch movement limited to small increments (e.g., ±6 inches per button press)
3. **Emergency Stop Precedence** - E-stop always takes precedence over calibration commands
4. **Physical Presence Requirement** - Ops Tech must be physically present (line-of-sight to player)
5. **Timeout Protection** - Calibration mode auto-disables after 5 minutes of inactivity
6. **Network Isolation** - USB receiver connected to isolated server PC (not on public network)

See the main README for detailed implementation examples.

---

## Protocol & Channel Mapping

### Communication Protocol

**Binary HoloCade protocol** over UDP (WiFi/Ethernet)

### Channel Mapping (Game Engine → Superhero Flight ECU)

| Channel | Type | Description | Range/Values |
|---------|------|-------------|--------------|
| **0** | Float | Front winch position (inches) | Relative to `standingGroundHeight` |
| **1** | Float | Front winch speed (inches/second) | 0.0+ |
| **2** | Float | Rear winch position (inches) | Relative to `standingGroundHeight` |
| **3** | Float | Rear winch speed (inches/second) | 0.0+ |
| **6** | Int32 | Game state | 0=standing, 1=hovering, 2=flight-up, 3=flight-forward, 4=flight-down |
| **7** | Bool | Emergency stop | true = stop all systems, return to standing |
| **9** | Bool | Play session active | true = winches can operate |
| **10** | Float | Standing ground height acknowledgment | Current winch position becomes new baseline |
| **11** | Float | Air height parameter (inches) | Height for hovering/flight-up/flight-down |
| **12** | Float | Prone height parameter (inches) | Height for flight-forward (prone position) |
| **13** | Float | Player height compensation (multiplier) | Adjusts `proneHeight` for player size |

### Channel Mapping (Superhero Flight ECU → Game Engine)

| Channel | Type | Description | Update Rate |
|---------|------|-------------|-------------|
| **310** | Struct | `FSuperheroFlightDualWinchState` - Dual-winch telemetry | 20 Hz (50ms) |
| **311** | Struct | `FSuperheroFlightTelemetry` - System health, temperatures, fault states | 1 Hz (1000ms) |

### Struct Definitions

**FSuperheroFlightDualWinchState** (Channel 310):
```cpp
struct FSuperheroFlightDualWinchState {
  float FrontWinchPosition;      // inches (relative to standingGroundHeight)
  float FrontWinchSpeed;         // inches/second
  float FrontWinchTension;       // load cell reading (lbs or N)
  float RearWinchPosition;       // inches (relative to standingGroundHeight)
  float RearWinchSpeed;          // inches/second
  float RearWinchTension;        // load cell reading (lbs or N)
  int32 GameState;               // 0=standing, 1=hovering, 2=flight-up, 3=flight-forward, 4=flight-down
  bool SafetyState;              // true = safe to operate, false = safety interlock active
  uint32 Timestamp;              // Timestamp when state was captured (ms)
};
```

**FSuperheroFlightTelemetry** (Channel 311):
```cpp
struct FSuperheroFlightTelemetry {
  float FrontWinchMotorTemp;     // °C
  float RearWinchMotorTemp;      // °C
  bool FrontWinchFault;          // true = fault detected
  bool RearWinchFault;           // true = fault detected
  bool WinchRedundancyStatus;    // true = both winches operational, false = degraded mode
  float SystemVoltage;           // V
  float SystemCurrent;           // A
  uint32 Timestamp;              // Timestamp when telemetry was captured (ms)
};
```

**Note:** There are **no button events** in Superhero Flight Experience. All control is gesture-based via `UFlightHandsController`.

---

## Game States & Winch Positions

### Standing Mode
- **Front Winch:** `standingGroundHeight` (taught, ready to lift)
- **Rear Winch:** `standingGroundHeight` (slack but stable)
- **Player Position:** Upright, feet on ground
- **Transition Trigger:** Player points double fists upward

### Hovering Mode
- **Front Winch:** `airHeight` (inches off ground)
- **Rear Winch:** `airHeight` (inches off ground)
- **Player Position:** Upright, lifted off ground
- **Transition Trigger:** Player releases either fist (stops flight motion)

### Flight-Up Mode
- **Front Winch:** `airHeight` (inches off ground)
- **Rear Winch:** `airHeight` (inches off ground)
- **Player Position:** Upright, arms pointing up
- **Transition Trigger:** Player points double fists upward (from standing) or changes arm angle

### Flight-Forward Mode
- **Front Winch:** `airHeight` (inches off ground)
- **Rear Winch:** `proneHeight` (inches off ground, adjusted by `playerHeightCompensation`)
- **Player Position:** Prone, arms pointing forward
- **Transition Trigger:** Player points double fists forward (from flight-up, via `upToForwardAngle` threshold)
- **Note:** Player body is prone, never inverted (feet/legs never above head)

### Flight-Down Mode
- **Front Winch:** `airHeight` (inches off ground)
- **Rear Winch:** `airHeight` (inches off ground)
- **Player Position:** Upright, arms pointing down
- **Transition Trigger:** Player points double fists downward (from flight-forward, via `forwardToDownAngle` threshold)
- **Note:** Same winch position as hovering - player remains upright, never inverted

### State Transitions

All state transitions use **smooth interpolation** between winch positions. The system never inverts the player (feet/legs never above head).

---

## Gesture Control System

### FlightHandsController

The `UFlightHandsController` component (client-side on HMD) converts 10-finger/arm gestures into control events:

1. **Fist Detection:**
   - Both fists closed = flight motion enabled
   - Single fist release = hover/stop (flight motion disabled)

2. **Gesture Direction:**
   - Analyzes HMD position relative to normalized center point between two hands
   - Calculates vector from HMD to hands center
   - Determines angle relative to world ground plane
   - Detects transitions via angle thresholds:
     - **Up → Forward:** `upToForwardAngle` (degrees)
     - **Forward → Down:** `forwardToDownAngle` (degrees)

3. **Flight Speed Throttle:**
   - Calculates normalized distance between HMD and average point between hands
   - Attenuated by player's `armLength` parameter
   - Determines flight speed throttle (0.0-1.0)
   - Multiplied by `flyingForwardSpeed`, `flyingUpSpeed`, or `flyingDownSpeed` based on mode

4. **Replication:**
   - Gesture events replicated to server via Unreal Replication
   - **Note:** Multiplayer replication is mostly NOOP for initial pass (documented as NOOP)

### Gesture Debugger

The `UGestureDebugger` component (HMD HUD) provides visualization for Ops Tech:

- Hand positions (debug rays)
- Normalized center point between hands
- Gesture direction vectors
- Transition angle thresholds (`upToForwardAngle`, `forwardToDownAngle`)
- Current flight mode
- Arm extension percentage
- Virtual altitude raycast visualization

Helps Ops Tech calibrate gesture sensitivity and verify player control.

---

## Virtual Altitude System

The virtual altitude system enables players to land on developer-defined "landable" surfaces:

1. **Raycasting:**
   - Cast ray from player's HMD position (minus player height) along worldspace down vector
   - Detects collision with "landable" surfaces (developer-defined collision volumes)
   - Calculates `virtualAltitude` as distance from player to landable surface hit point

2. **Landing Logic:**
   - When player is above landable surface, they can elect to stop flying and hover by releasing either fist
   - When `virtualAltitude` closes toward zero during flight-down mode, system transitions from flight-down to standing mode
   - Both winches slacken from `airHeight` back to `standingGroundHeight`

3. **Separation from Physical Heights:**
   - Virtual altitude is **separate** from physical `airHeight` and `proneHeight` parameters
   - Physical heights are real-world winch positions
   - Virtual altitude is game-world distance to landable surfaces

---

## Server-Side Parameters

All parameters are exposed in Command Console for Ops Tech control:

| Parameter | Type | Description | Default |
|-----------|------|-------------|---------|
| `airHeight` | Float (inches) | Height for hovering/flight-up/flight-down | 24.0 |
| `proneHeight` | Float (inches) | Height for flight-forward (prone position) | 36.0 |
| `standingGroundHeight` | Float (inches) | Calibrated per-player baseline (read-only) | 0.0 |
| `playerHeightCompensation` | Float (multiplier) | Adjusts `proneHeight` for player size | 1.0 |
| `flyingForwardSpeed` | Float | Maximum forward flight speed | 10.0 |
| `flyingUpSpeed` | Float | Maximum upward flight speed | 5.0 |
| `flyingDownSpeed` | Float | Maximum downward flight speed | 8.0 |
| `armLength` | Float (inches) | Auto-calibrated from player height, manually adjustable | Auto |
| `upToForwardAngle` | Float (degrees) | Angle threshold for transition from up to forward | 45.0 |
| `forwardToDownAngle` | Float (degrees) | Angle threshold for transition from forward to down | 45.0 |

**Real-Time Displays:**
- Flight speed percentage (shows if player hitting 100% throttle at full extension)
- Gesture debugger toggle (enable/disable HUD debug rays)
- Winch redundancy status display

---

## Hardware Requirements

### Winch System

- **2× Electric Winches** (one front, one rear per player)
  - **Motor Type:** DC motor with position feedback (encoder or potentiometer)
  - **Capacity:** 300-500 lbs per winch (redundant system)
  - **Speed Range:** 0-12 inches/second (adjustable)
  - **Position Feedback:** Encoder or potentiometer (required for closed-loop control)
  - **Mounting:** Truss-mounted, two feet apart

### Harness System

- **EasyFit Harness** (Petzl EasyFit or similar)
  - Primary attachment point at shoulder-blade level
  - Full-body fall arrest protection
  - Size 1-2 (adjustable)

- **Apron-Style Harness**
  - Swaddles player's legs and torso
  - Attachment point at pelvis level
  - Enables prone positioning

### ECU Hardware

- **Main Superhero Flight ECU:**
  - ESP32 (recommended) or other WiFi/Ethernet-capable microcontroller
  - Dual motor drivers (one per winch)
  - Dual load cell interfaces (tension monitoring)
  - Position sensor interfaces (encoders or potentiometers)
  - Power supply: 12V-24V DC (separate from winch motors)

- **Height Calibration System:**
  - **433MHz Wireless Remote** (up/down buttons) - Portable clicker for Ops Tech
  - **433MHz USB Receiver Dongle** - USB dongle connected to server PC (e.g., RTL-SDR, CC1101 USB module)
  - **No Custom PCB Required** - Uses off-the-shelf USB receiver modules
  - Commands routed directly to game server (no intermediate ECU)

### Safety Systems

- **Emergency Descent Mechanism:**
  - Manual e-stop button (accessible to Ops Tech)
  - Automatic e-stop on fault detection
  - Both winches descend to `standingGroundHeight` on e-stop

- **Weight/Height Limit Sensors:**
  - Load cells on both winches (tension monitoring)
  - Height limit switches (optional, for extra safety)
  - Prevents operation if limits exceeded

- **Soft Pad:**
  - Padded surface below player for extra safety
  - Minimum 6 inches thick, covers entire play area

- **E-Stop Integration:**
  - Emergency stop button accessible to Ops Tech
  - Integrates with winch control system
  - Stops all motion immediately

---

## Safety Considerations

1. **Never Invert Player:**
   - Player body is only ever upright or prone
   - Feet/legs never above head
   - Flight-down mode uses same winch position as hovering (upright)

2. **Winch Redundancy:**
   - System can operate in degraded mode if one winch fails
   - Both winches have independent safety systems
   - Redundancy status reported to Command Console

3. **Weight/Height Limits:**
   - System prevents operation if weight/height limits exceeded
   - Load cells monitor tension on both winches
   - Safety interlocks prevent unsafe operation

4. **Emergency Descent:**
   - Manual e-stop triggers immediate descent to `standingGroundHeight`
   - Automatic e-stop on fault detection
   - Both winches descend simultaneously

5. **Soft Pad:**
   - Padded surface below player for extra safety
   - Covers entire play area
   - Minimum 6 inches thick

6. **Calibration Security:**
   - Calibration only works when `playSessionActive = false` (calibration mode)
   - Winch movement limited to small increments during calibration (e.g., ±6 inches per button press)
   - Emergency stop always active and takes precedence over calibration commands
   - Calibration mode auto-disables after 5 minutes of inactivity (timeout)
   - Rolling code validation prevents replay attacks (if remote supports it)
   - Physical presence required: Ops Tech must be line-of-sight to player during calibration

---

## Configuration

### ECU Firmware Configuration

Edit the following in `SuperheroFlightExperience_ECU.ino`:

1. **WiFi credentials:**
   ```cpp
   const char* ssid = "VR_Arcade_LAN";
   const char* password = "your_password_here";
   ```

2. **Unreal Engine PC IP address:**
   ```cpp
   IPAddress unrealIP(192, 168, 1, 100);
   uint16_t unrealPort = 8888;
   ```

3. **Winch motor pins:**
   ```cpp
   frontWinchConfig.motorPin = 12;  // PWM pin for front winch motor
   rearWinchConfig.motorPin = 13;   // PWM pin for rear winch motor
   ```

4. **Position sensor pins:**
   ```cpp
   frontWinchConfig.positionSensorPin = A0;  // Analog pin for front winch encoder/pot
   rearWinchConfig.positionSensorPin = A1;    // Analog pin for rear winch encoder/pot
   ```

5. **Load cell pins:**
   ```cpp
   frontWinchConfig.loadCellPin = A2;  // Analog pin for front winch load cell
   rearWinchConfig.loadCellPin = A3;   // Analog pin for rear winch load cell
   ```

6. **Safety limits:**
   ```cpp
   config.maxWeight = 300.0f;      // lbs (per winch)
   config.maxHeight = 120.0f;      // inches (relative to standingGroundHeight)
   config.emergencyDescentSpeed = 6.0f;  // inches/second
   ```

### Height Calibration System Configuration

1. **433MHz USB Receiver Dongle:**
   - Plug USB dongle into server PC
   - Install driver/software for USB dongle (varies by module)
   - Configure receiver to listen on 433MHz frequency
   - Map up/down button codes to winch commands

2. **433MHz Wireless Remote:**
   - Pair remote with USB receiver (learn button codes)
   - Test up/down commands
   - Remote should have at least 2 buttons (up, down) or 4 buttons (up, down, fine-up, fine-down)

3. **Server Integration:**
   - SuperheroFlightExperience uses `RF433MHz` low-level API module
   - API provides abstraction layer (`I433MHzReceiver` interface) for different USB receiver modules
   - Each module type has its own implementation (RTL-SDR, CC1101, RFM69, Generic)
   - Game server code uses the abstraction interface, not module-specific APIs
   - USB receiver implementations decode button events (with rolling code validation if supported)
   - API validates button events (rolling code check, replay attack prevention)
   - SuperheroFlightExperience enforces safety interlocks (calibration mode only, movement limits, timeout)
   - Experience maps button events to winch position commands
   - Experience sends commands to main Superhero Flight ECU via UDP

**Recommended USB Receiver Modules:**
- **RTL-SDR USB Dongle** - Software-defined radio, can receive 433MHz with appropriate software
- **CC1101 USB Module** - Dedicated 433MHz transceiver module with USB interface
- **RFM69/RFM95 USB Module** - LoRa/RF modules with USB interface (433MHz capable)
- **Generic 433MHz USB Receiver** - Off-the-shelf USB dongles available on Amazon/eBay

**Note:** Different modules have different low-level drivers/APIs (libusb, serial/COM ports, proprietary SDKs), but HoloCade's abstraction layer (`I433MHzReceiver`) normalizes the interface so game server code is module-agnostic. Developers choose their preferred module and use the corresponding implementation class (e.g., `RTL433MHzReceiver`, `CC1101433MHzReceiver`).

---

## Dependencies

- `HoloCade_Wireless_RX.h` - For receiving commands from Unreal/Unity
- `HoloCade_Wireless_TX.h` - For sending telemetry to Unreal/Unity

Copy these files from `FirmwareExamples/Base/Templates/` to your sketch directory.

---

## Usage in Unreal Engine

### Basic Setup

```cpp
// In SuperheroFlightExperience
ASuperheroFlightExperience* FlightExp = GetWorld()->SpawnActor<ASuperheroFlightExperience>();

// Configure parameters
FlightExp->AirHeight = 24.0f;  // inches
FlightExp->ProneHeight = 36.0f;  // inches
FlightExp->FlyingForwardSpeed = 10.0f;
FlightExp->FlyingUpSpeed = 5.0f;
FlightExp->FlyingDownSpeed = 8.0f;

// Initialize experience
FlightExp->InitializeExperience();

// Ops Tech calibrates player height (via 433MHz wireless clicker → USB receiver → server)
// Then queues experience to begin
FlightExp->AcknowledgeStandingGroundHeight();  // Sets current winch position as baseline
```

### Gesture Control

The `UFlightHandsController` component (client-side on HMD) automatically:
- Detects fist vs open hand gestures
- Analyzes HMD-to-hands vector
- Calculates flight speed throttle
- Replicates gesture events to server

Server receives gesture events and commands winches accordingly.

### Virtual Altitude Landing

```cpp
// Developer defines landable surfaces in level
// System automatically raycasts and detects landable surfaces
// When virtualAltitude closes to zero, system transitions to standing mode
```

---

## Multiplayer Support

**Note:** Multiplayer support is mostly NOOP for initial pass (documented as NOOP).

Future implementation will support:
- Up to 4 players, each on their own dual-winch harness
- Unreal Replication to transport gesture-based triggers to server for relay
- Synchronized player positions and flight states

---

## Optional Motion Platform (v2.0)

**Note:** Optional 2DOF or 3DOF hydraulic motion platform support is planned for v2.0.

- Platform ECU is **separate** from winch ECU
- Both orchestrated by `USuperheroFlightECUController` on server
- Cannot repurpose existing 4DOF template (Gunship) or 2DOF gyro template (FlightSim)
- Requires new 2DOF/3DOF hydraulic platform controllers built from scratch
- Platform provides additional motion feedback during takeoff/landing sequences

See roadmap v2.0 for details.

---

## Troubleshooting

### Winch Not Responding

1. Check ECU connection (WiFi/Ethernet)
2. Verify play session is active (Channel 9 = true)
3. Check emergency stop status (Channel 7 = false)
4. Verify winch motor power supply
5. Check position sensor feedback

### Gesture Control Not Working

1. Enable gesture debugger in HMD HUD
2. Verify hand tracking is active
3. Check `armLength` calibration (auto-calibrated, manually adjustable)
4. Verify `upToForwardAngle` and `forwardToDownAngle` thresholds
5. Check gesture event replication to server (mostly NOOP for initial pass)

### Player Not Transitioning to Prone

1. Check `proneHeight` parameter
2. Verify `playerHeightCompensation` multiplier
3. Check rear winch position feedback
4. Verify gesture direction detection (use gesture debugger)

### Safety Interlock Active

1. Check weight limits (load cell readings)
2. Check height limits (position sensor readings)
3. Verify both winches are operational (redundancy status)
4. Check for fault conditions (motor temperature, system voltage)

---

## Cost Considerations

See `COST_ANALYSIS.md` (in parent directory) for detailed cost breakdown. Typical costs:

- **Dual Winch System:** $2,000-$4,000 (per player station)
- **Harness System:** $300-$400 (EasyFit + apron harness)
- **ECU Hardware:** $50-$100 (ESP32 + motor drivers + sensors)
- **Safety Systems:** $200-$300 (e-stop, load cells, soft pad)
- **Total per Player Station:** ~$2,550-$4,800

---

## References

- **Main README:** See `HoloCade_Unreal/Plugins/HoloCade/README.md` for complete SDK documentation
- **Hardware Specs:** See `SuperheroFlightExperience_Hardware_Specs.md` (to be created)
- **IO Flow Documentation:** See `SuperheroFlightExperience_IO_Flow_Documentation.md` (to be created)
- **Gunship Experience:** See `GunshipExperience/FourPlayerRig/README.md` for similar dual-ECU architecture reference

---

Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

