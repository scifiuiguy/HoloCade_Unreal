# Flight Sim Experience ECU

Embedded control unit for 2DOF continuous rotation gyroscope system used in FlightSimExperience.

## Overview

This ECU controls two servo motors for continuous pitch and roll rotation, enabling full 360° (and beyond) rotation for flight simulator experiences. Unlike hydraulic actuator systems, this uses servo motors with continuous rotation capability.

## Hardware

- **2x Continuous Rotation Servo Motors** (or standard servos modified for continuous rotation)
- **ESP32** (recommended) or other WiFi-capable microcontroller
- **Servo motor drivers** (PWM-capable pins)
- **Power supply** for servo motors (separate from microcontroller power)
- **Optional:** Position encoders for feedback (if using modified servos)

## Protocol & Channel Mapping

### Channel-Based Commands

| Channel | Type | Description | Range |
|---------|------|-------------|-------|
| **0** | Float | Pitch (degrees) | Unlimited (continuous rotation) |
| **1** | Float | Roll (degrees) | Unlimited (continuous rotation) |
| **4** | Float | Duration (seconds) | 0.1+ seconds |
| **7** | Bool | Emergency Stop | true = stop all systems |
| **8** | Bool | Return to Neutral | true = return to 0° pitch and roll |

### Struct Packets

- **Channel 102:** `FGyroState` struct (pitch and roll only)
  ```cpp
  struct FGyroState {
    float Pitch;  // degrees (unlimited)
    float Roll;   // degrees (unlimited)
  };
  ```

## Usage in Unreal/Unity

### Unreal Engine

```cpp
// In FlightSimExperience
AFlightSimExperience* FlightSim = GetWorld()->SpawnActor<AFlightSimExperience>();

// Send continuous rotation (can exceed 360°)
FlightSim->SendContinuousRotation(720.0f, 360.0f, 4.0f);  // Two barrel rolls!

// Emergency stop
FlightSim->EmergencyStop();

// Return to neutral
FlightSim->ReturnToNeutral(3.0f);
```

## Configuration

Edit the following in `FlightSimExperience_ECU.ino`:

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

3. **Servo pin assignments:**
   ```cpp
   gyroConfig.pitchServoPin = 12;  // PWM pin for pitch servo
   gyroConfig.rollServoPin = 13;   // PWM pin for roll servo
   ```

4. **Servo pulse widths** (adjust for your servos):
   ```cpp
   gyroConfig.servoMinPulseWidth = 500;      // Minimum pulse width (microseconds)
   gyroConfig.servoMaxPulseWidth = 2500;     // Maximum pulse width (microseconds)
   gyroConfig.servoCenterPulseWidth = 1500;  // Center/stop pulse width
   ```

5. **Maximum rotation speed:**
   ```cpp
   gyroConfig.maxRotationSpeedDegreesPerSecond = 90.0f;  // Adjust as needed
   ```

## Dependencies

- `HoloCade_Wireless_RX.h` - For receiving commands from Unreal/Unity
- `HoloCade_Wireless_TX.h` - For sending feedback to Unreal/Unity
- `GyroscopeController.h` - Servo motor control logic

Copy these files from `FirmwareExamples/Base/Templates/` to your sketch directory.

## Platform-Specific Notes

### ESP32

Use `ledcSetup()` and `ledcWrite()` for PWM control:

```cpp
// In GyroscopeController::initializeServos()
ledcSetup(0, 50, 16);  // Channel 0, 50Hz, 16-bit resolution
ledcAttachPin(config.pitchServoPin, 0);
ledcSetup(1, 50, 16);  // Channel 1, 50Hz, 16-bit resolution
ledcAttachPin(config.rollServoPin, 1);

// In GyroscopeController::updateServos()
ledcWrite(0, pitchPulse);
ledcWrite(1, rollPulse);
```

### Arduino

Use the Servo library:

```cpp
#include <Servo.h>

Servo pitchServo;
Servo rollServo;

// In GyroscopeController::initializeServos()
pitchServo.attach(config.pitchServoPin);
rollServo.attach(config.rollServoPin);

// In GyroscopeController::updateServos()
pitchServo.writeMicroseconds(pitchPulse);
rollServo.writeMicroseconds(rollPulse);
```

## Safety

- **Emergency Stop:** Channel 7 immediately stops all motion
- **Return to Neutral:** Channel 8 smoothly returns to 0° pitch and roll
- **Maximum Rotation Speed:** Limited by `maxRotationSpeedDegreesPerSecond` to prevent excessive acceleration

## Bidirectional IO

The ECU sends position feedback to Unreal every 100ms (10 Hz) on Channel 102 as `FGyroState` struct packets.

## Continuous Rotation

Unlike hydraulic actuators with limit switches, this gyroscope system supports:
- **Unlimited rotation** in either direction
- **Negative angles** for counter-clockwise rotation
- **Values beyond 360°** for multiple full rotations
- Example: 450° = 1.25 rotations clockwise, -90° = 0.25 rotations counter-clockwise

---

# Flight Sim Experience - VR Tracking Recommendations

## ⚠️ Critical: Tracking System Requirements for Space Reset

The Flight Sim Experience includes a **Space Reset** feature (`bSpaceReset`) that decouples the virtual cockpit from the physical cockpit during gravity reset. This feature is designed for space flight experiences where there is no gravity, but the physical cockpit must return to upright position.

### Recommended Tracking Solution: Outside-In with Cockpit-Mounted Trackers

**HoloCade strongly recommends using outside-in tracking with trackers mounted to the cockpit frame** for Flight Sim experiences that use Space Reset.

#### Recommended Default Configuration

- **HMD:** Bigscreen Beyond 2
- **Tracking System:** SteamVR Lighthouse (Base Station 2.0)
- **Tracker Mounting:** SteamVR Trackers mounted directly to the cockpit frame (not the player)
- **Tracking Reference:** HMD tracks relative to cockpit, not physical world

#### Why This Matters

When `bSpaceReset` and `bGravityReset` are both enabled:
1. The physical cockpit smoothly returns to zero (upright) position
2. The virtual cockpit remains frozen in its current orientation (simulating zero-g)
3. **The HMD must track relative to the cockpit, not the physical world**

If the HMD tracks relative to the physical world (inside-out tracking like Quest, or outside-in with world-mounted base stations), the player's head will appear to rotate upside down as the physical cockpit returns to upright while the virtual cockpit stays frozen.

### ⚠️ Warning: HMD Correction Complexity

**If you choose to use inside-out tracking (Quest, Pico, etc.) or outside-in tracking with world-mounted base stations, you will need to implement HMD correction yourself.**

HMD correction for Space Reset is a **complex and error-prone problem** that involves:
- Tracking the physical cockpit rotation
- Calculating the offset between physical and virtual cockpit
- Applying inverse rotation to the HMD transform
- Handling edge cases (tracking loss, calibration drift, etc.)
- Managing coordinate system transformations
- Dealing with latency and smoothing

**HoloCade does not provide HMD correction in the plugin.** We recommend using cockpit-mounted trackers to avoid this complexity entirely.

### Alternative Tracking Solutions

If you must use inside-out tracking or world-mounted base stations:

1. **Implement your own HMD correction system** that:
   - Reads physical cockpit rotation from `GetCurrentGyroState()`
   - Calculates the offset between physical and virtual cockpit
   - Applies inverse rotation to HMD camera transform
   - Handles all edge cases and coordinate system conversions

2. **Test extensively** - HMD correction bugs can cause:
   - Motion sickness
   - Disorientation
   - Tracking loss
   - Visual glitches

3. **Consider disabling Space Reset** if HMD correction proves too problematic

### Setup Instructions for Recommended Configuration

1. **Mount SteamVR Base Stations** to the cockpit frame (not the room walls)
2. **Mount SteamVR Trackers** to the cockpit frame at fixed positions
3. **Configure SteamVR** to use cockpit-mounted base stations as tracking reference
4. **Calibrate** the tracking system with cockpit at zero position
5. **Test** Space Reset functionality to ensure HMD tracks correctly relative to cockpit

### Technical Details

- **Cockpit Transform Decoupling:** When `bSpaceReset = true` and `bGravityReset = true`, the virtual cockpit transform is frozen while physical cockpit returns to zero
- **Recoupling:** Virtual cockpit recouples to physical when:
  - Physical cockpit reaches near-zero position (within `ZeroThresholdDegrees`)
  - `bGravityReset` is disabled
  - Player provides stick input (resumes normal operation)

For more information, see the `AFlightSimExperience` class documentation in the Unreal Engine source code.

---

# Flight Sim Experience - Power System Recommendations

## Power Architecture: No Slip Rings Required

**HoloCade recommends mounting the battery pack directly inside the cockpit frame** - no slip rings or rotating power connections needed. This simplifies the design and reduces failure points.

### Recommended Power Solution: 6S LiFePO4 Battery Pack

Use the same **6S LiFePO4 battery pack** configuration as other HoloCade experiences (Gunship, MovingPlatform, etc.), but with the option for **higher capacity** since the battery can be permanently mounted and enclosed within the cockpit frame.

#### Battery Specifications

- **Chemistry:** LiFePO4 (Lithium Iron Phosphate) - safer than LiPo, better for commercial installations
- **Configuration:** 6S (6 cells in series)
- **Nominal Voltage:** 19.2V (6 × 3.2V per cell)
- **Cell Type:** 21700 format (3000–4000mAh per cell)
- **Pack Capacity:** 100 cells per pack (typical) = ~300–400 Ah total capacity
- **BMS:** Daly 6S LiFePO4 BMS (smart, 60–100A, Bluetooth/app-enabled)

#### Advantages of Cockpit-Mounted Battery

1. **No Slip Rings:** Eliminates rotating electrical connections (failure point)
2. **Higher Capacity Possible:** Battery can be larger/heavier since it's fixed to frame
3. **Better Enclosure:** Can be fully enclosed and protected within cockpit structure
4. **Simpler Wiring:** All power distribution stays within cockpit frame
5. **Easier Maintenance:** Battery accessible without disassembling rotating parts

### Power Distribution

The 6S battery pack (19.2V nominal) powers:

1. **Servo Motor Drives** (via power supply)
   - Professional servo drives typically require 24V or 48V DC
   - Use DC-DC boost converter: 19.2V → 24V or 48V
   - Power consumption: ~400W–750W per motor (Panasonic A6) or 1–3 kW per motor (Kollmorgen AKM)

2. **ESP32 ECU**
   - Can run directly from 19.2V via voltage regulator (19.2V → 3.3V/5V)
   - Power consumption: ~1–2W

3. **Lighthouse Base Stations** (if using recommended tracking)
   - **Power Requirements:** 12V DC, ~1.5A per base station (18W each)
   - **Total:** 2 base stations = 36W (12V @ 3A)
   - **Solution:** Separate 12V LiFePO4 battery pack (recommended - simpler and cleaner)

### Lighthouse Power: Separate 12V LiFePO4 Battery Pack (Recommended)

**HoloCade recommends using a separate 12V LiFePO4 battery pack for Lighthouse base stations** - this is much simpler than using a DC-DC converter and provides independent power for tracking.

#### Recommended Configuration

- **Battery Type:** 12V LiFePO4 battery pack
- **Capacity:** 20–50 Ah (provides 5–12 hours runtime for 2 base stations)
- **Chemistry:** LiFePO4 (same safe chemistry as main 6S pack)
- **Voltage:** 12.8V nominal (4S LiFePO4: 4 × 3.2V)
- **BMS:** 4S LiFePO4 BMS (smart, 20–30A, Bluetooth/app-enabled)
- **Mounting:** Inside cockpit frame alongside main 6S pack

#### Advantages

- **Simpler:** No DC-DC converter needed - direct 12V connection
- **Independent Power:** Tracking continues even if main battery fails
- **Lower Cost:** 12V LiFePO4 pack + BMS (~$50–$100) vs converter + wiring
- **Easier Maintenance:** Separate charging/discharging cycles
- **Better Isolation:** Tracking power isolated from motor power (reduces electrical noise)

#### Implementation

1. **Purchase 12V LiFePO4 Battery Pack:**
   - Pre-built 12V LiFePO4 pack (20–50 Ah) with integrated BMS
   - Or build from 4S LiFePO4 cells (21700 format, same as main pack)
   - Include 4S BMS with Bluetooth monitoring

2. **Mounting:**
   - Mount inside cockpit frame near main 6S battery
   - Ensure adequate ventilation
   - Use same fire-resistant enclosure considerations

3. **Wiring:**
   - Direct 12V connection to both Lighthouse base stations
   - Use appropriate gauge wire (18 AWG minimum for 3A @ 12V)
   - Install inline fuse (5A) for safety

4. **Charging:**
   - Use 4S LiFePO4 charger (same type as 6S charger, just 4S configuration)
   - Or use multi-chemistry charger that supports 4S LiFePO4
   - Charge in ops/staging area (not in cockpit)

#### Runtime Estimate

- **20 Ah pack:** 20 Ah ÷ 3A = **~6.7 hours** runtime
- **50 Ah pack:** 50 Ah ÷ 3A = **~16.7 hours** runtime

**Note:** Lighthouse base stations have very low power consumption, so even a small 20 Ah pack provides a full day of operation.

#### Cost Estimate

- **12V LiFePO4 Pack (20–50 Ah):** $40–$80 (pre-built) or $20–$40 (DIY from cells)
- **4S BMS:** $15–$25
- **4S Charger:** $25–$35
- **Total:** ~$80–$140 (vs $20–$40 for converter + complexity)

### Power Consumption Estimates

| Component | Voltage | Current | Power | Notes |
|-----------|---------|---------|-------|-------|
| **Servo Motors (2x)** | 24V/48V | 8–15A | 400W–750W | Panasonic A6 (via boost converter) |
| **Servo Motors (2x)** | 24V/48V | 20–60A | 1–3 kW | Kollmorgen AKM (via boost converter) |
| **ESP32 ECU** | 3.3V/5V | 0.3A | 1–2W | Via voltage regulator |
| **Lighthouse Base Stations (2x)** | 12V | 3A total | 36W | Separate 12V LiFePO4 battery pack |
| **Total (Panasonic A6)** | - | - | **~450W** | Average during operation |
| **Total (Kollmorgen AKM)** | - | - | **~1.1–3.1 kW** | Average during operation |

### Battery Runtime Estimates

Assuming 6S LiFePO4 pack with 300 Ah capacity (typical configuration):

- **Panasonic A6 System:** ~300 Ah ÷ 23A (450W ÷ 19.2V) = **~13 hours** runtime
- **Kollmorgen AKM System:** ~300 Ah ÷ 55–160A (1.1–3.1 kW ÷ 19.2V) = **~2–5 hours** runtime

**Note:** Actual runtime depends on:
- Duty cycle (how often servos are actively moving)
- Servo load (actual torque required)
- Battery age and health
- Temperature

### Charging

Use the same **6S hobby RC charger** as other HoloCade experiences:
- **Model:** B6AC balance charger or similar
- **Power:** 80W+ (supports 1–6S LiFePO4)
- **Connector:** XT60
- **Location:** Mount charger in ops/staging area (not in cockpit)

### Safety Considerations

1. **BMS Protection:** Always use a smart BMS with overcurrent, overvoltage, undervoltage, and temperature protection
2. **Fusing:** Install appropriate fuses on all power distribution lines
3. **Enclosure:** Battery pack should be fully enclosed within cockpit frame (fire-resistant if possible)
4. **Ventilation:** Ensure adequate ventilation around battery pack (LiFePO4 is safer than LiPo, but still needs airflow)
5. **Emergency Disconnect:** Install emergency power disconnect switch accessible to operator

### Cost Considerations

See `COST_ANALYSIS.md` for detailed power system costs. Typical power system costs:
- **6S Battery Pack:** $250–$300 (100 cells @ $2.50–$3.00/cell)
- **BMS:** $20–$25
- **Charger:** $25–$35
- **DC-DC Converters:** $50–$100 (24V/48V boost for servos only)
- **12V LiFePO4 Pack (Lighthouse):** $80–$140 (separate battery system)
- **Total Power System:** ~$445–$640 (excluding servo drives)

For more details, see the Gunship Experience cost analysis section on power systems.

