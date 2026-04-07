# Superhero Flight Experience - Hardware Specification

**Custom DIY Winch System for Dual-Winch Suspension**

This document specifies the hardware components and wiring for a custom-built winch system using ESP32 and industrial components. This approach provides full control, cost savings, and avoids proprietary PLC systems.

---

## Overview

**System Requirements:**
- **Capacity:** 300+ lbs per winch (600+ lbs total with redundancy)
- **Position Feedback:** Rotary encoder on winch drum (one per winch)
- **Tension Monitoring:** Load cell with HX711 amplifier (one per winch - required for ride classification)
- **Speed Control:** Variable speed (0-12 inches/second)
- **Safety:** Electromagnetic brake (one per winch - required for ride classification), emergency stop
- **Control:** ESP32-based ECU
- **Structural Support:** Custom Warren truss design (mild steel, 8ft width, 9-10ft height)

---

## Truss Design & Structural Support

### Truss Configuration

**Primary Truss Structure:**
- **Type:** Warren truss with parallel chord design
- **Chords:** QTY 2 square steel tubes, aligned in parallel
- **Chord Material:** Mild steel square tube
- **Chord Dimensions:** 2" × 2" square tube (exact wall thickness TBD based on load calculations)
- **Chord Spacing:** 20" between inner edges (24" outer-edge width, chord-to-chord)
- **Truss Length:** 8 feet (96 inches)
- **Truss Height:** 9-10 feet above player's head (target: 9', maximum: 10' - check local regulations for specific requirements)
- **Web Members:** 45-degree Warren truss pattern
  - **Material:** Mild steel round tube
  - **Dimensions:** 1" diameter, 1/8" wall thickness
  - **Pattern:** Diagonal bracing between chords at 45-degree angles
- **Total Weight:** Approximately 120 lbs (truss alone, excluding winches and hardware)

**Mounting System:**
- **Compatibility:** Custom load truss can be bolted to any standard vertical truss mounting system
- **Mounting Points:** Standard truss connection hardware (bolted connections)
- **Vertical Support:** Standard vertical truss legs or columns (commercial truss system)
- **Base Plate:** Battery enclosure mounts to truss leg base plate (see Battery Placement section)

**Winch Mounting:**
- **Front Winch:** Mounted to truss chord (left side, 2 feet from center)
- **Rear Winch:** Mounted to truss chord (right side, 2 feet from center)
- **Spacing:** Winches spaced 2 feet apart (center-to-center)
- **Mounting Hardware:** Custom brackets bolted to truss chords

### Structural Engineering Considerations

**Load Calculations:**
- **Static Load:** 300 lbs per winch (600 lbs total) + truss weight (120 lbs) + hardware (~50 lbs) = ~770 lbs total
- **Dynamic Load:** Account for acceleration/deceleration forces (safety factor recommended: 3-5x static load)
- **Safety Factor:** Minimum 5:1 safety factor recommended for human suspension applications
- **Deflection:** Limit vertical deflection to < 1/360 of span (approximately 0.27" for 8ft span)

**Material Specifications:**
- **Mild Steel:** A36 or equivalent (readily available, cost-effective)
- **Welding:** All welds must be performed by certified welder per AWS D1.1 standards
- **Surface Treatment:** Powder coating or paint for corrosion protection
- **Inspection:** Periodic visual inspection for cracks, deformation, or corrosion

**Professional Engineering:**
- ⚠️ **Required:** Engage licensed structural engineer for:
  - Load calculations and stress analysis
  - Welding specifications and procedures
  - Connection design (bolted joints, mounting brackets)
  - Deflection analysis
  - Final design approval and stamping

### Regulatory Compliance & Safety Recommendations

**⚠️ IMPORTANT DISCLAIMER:**
- **Regulatory Variations:** Amusement ride regulations, building codes, and safety standards vary significantly by state, city, and jurisdiction
- **No Liability:** The author of HoloCade disclaims all liability for use of this cursory documentation
- **Professional Consultation Required:** This documentation is provided for informational purposes only and does not constitute professional engineering, legal, or regulatory advice
- **Compliance Verification:** You must check local and state documentation, consult with structural engineers, and engage amusement ride inspectors to ensure full compliance with all applicable regulations
- **Your Responsibility:** It is your responsibility to verify all safety requirements, obtain necessary permits, and ensure compliance with all applicable laws and regulations

**Safety Recommendations (Informational Only):**
- **Height Limitation:** Target height of 9 feet above player's head, maximum 10 feet (check local regulations for specific height requirements)
- **No Gravity Drop:** System design uses controlled winch descent only (no free-fall or gravity drop features)
- **Controlled Motion:** All player movement is controlled by winch motors with safety brakes
- **Emergency Descent:** Emergency stop uses controlled winch descent (not gravity drop)
- **Safety Brakes:** Electromagnetic brakes prevent uncontrolled descent (fail-safe design)

**Recommended Documentation (Check Local Requirements):**
- Engineering drawings (truss design, load calculations) - may be required by local authorities
- Material certifications (steel grade, welding procedures) - may be required for inspection
- Installation procedures and inspection checklist
- Height verification measurements
- Safety system documentation (brakes, E-Stop, redundancy)
- Professional engineering stamp (may be required by local building codes)

---

## Component List (Per Winch)

### 1. Motor & Motor Controller

**Option A: RoboClaw Motor Controller (Recommended)**
- **Part:** RoboClaw 2x30A Motor Controller
- **Supplier:** Pololu, Amazon
- **Price:** ~$150-200
- **Features:**
  - Dual-channel (can control 2 winches with one controller)
  - Built-in encoder support
  - PWM input from ESP32
  - 30A continuous, 60A peak
  - Overcurrent protection
  - **Pros:** Professional, reliable, built-in safety
  - **Cons:** Higher cost

**Option B: Sabertooth 2x32 Motor Controller**
- **Part:** Dimension Engineering Sabertooth 2x32
- **Supplier:** Dimension Engineering, Amazon
- **Price:** ~$150-200
- **Features:**
  - Dual-channel
  - PWM input
  - 32A continuous
  - **Pros:** Similar to RoboClaw
  - **Cons:** Higher cost

**Option C: Custom H-Bridge (DIY)**
- **Part:** BTS7960 H-Bridge Module (43A) or similar
- **Supplier:** Amazon, AliExpress
- **Price:** ~$10-20 per module
- **Features:**
  - PWM + direction control
  - Requires external current sensing
  - **Pros:** Very low cost
  - **Cons:** Requires more engineering, less safety features

**Motor:**
- **Part:** 24V DC Gearmotor (500-1000W, 300+ lb capacity)
- **Examples:**
  - BaneBots RS-775 with gearbox (1:50 or 1:100 ratio)
  - CIM Motor with gearbox
  - 24V 500W-1000W gearmotor from Automation Direct, McMaster-Carr
- **Supplier:** VEX Robotics, Automation Direct, McMaster-Carr, Amazon
- **Price:** ~$100-300
- **Specifications:**
  - 24V DC operation
  - 500-1000W power rating
  - Gear ratio: 50:1 to 100:1 (for torque)
  - Output RPM: 50-200 RPM (for 12 inches/second winch speed)

**Battery System (24V LiFePO4):**
- **Part:** 24V LiFePO4 Battery Pack (8S configuration)
- **Supplier:** Battle Born, Ampere Time, Amazon, AliExpress
- **Price:** ~$200-500 (varies by capacity)
- **Specifications:**
  - 24V nominal (25.6V fully charged, 20V discharged)
  - Capacity: 50-100 Ah recommended (see capacity calculations below)
  - Built-in BMS (Battery Management System) required
  - LiFePO4 chemistry (safer than LiPo, longer cycle life)
  - **Pros:** Portable, no AC power required, safer chemistry
  - **Cons:** Requires charging infrastructure, weight (~15-30 lbs)

**Battery Capacity Calculation:**
```
Peak Power Draw: 2 winches × 1000W = 2000W
Peak Current: 2000W / 24V = ~83A (peak, short duration)

Average Power Draw: 2 winches × 500W = 1000W
Average Current: 1000W / 24V = ~42A

For 1 hour operation: 42A × 1 hour = 42 Ah minimum
For 2 hour operation: 42A × 2 hours = 84 Ah minimum
For 4 hour operation: 42A × 4 hours = 168 Ah minimum

Recommended: 50-100 Ah for 1-2 hour sessions, 100-200 Ah for full-day operation
```

**Battery Charger (Integrated in Battery Enclosure):**
- **Part:** 24V LiFePO4 Battery Charger (10A charging current)
- **Supplier:** Battle Born, Ampere Time, Amazon
- **Price:** ~$50-100
- **Specifications:**
  - 24V output (matches battery voltage)
  - 10A charging current (balanced charging speed vs. battery life)
  - AC input (110V or 220V, via extension cable)
  - Automatic charge termination (BMS-controlled)
  - Built-in LED status indicators (charging, complete, fault)
  - **Mounting:** Integrated into battery enclosure (mounted alongside battery)
  - **Note:** Charge during off-hours (overnight) or between sessions. Extension cable must be stowed during operation.

**Battery Management System (BMS):**
- **Part:** Built-in BMS (included with quality battery packs)
- **Features:**
  - Overcharge protection (stops charging at 28.8V)
  - Over-discharge protection (stops discharge at 20V)
  - Overcurrent protection (limits current draw)
  - Cell balancing (ensures all cells charge equally)
  - Temperature monitoring (prevents charging/discharging in extreme temps)
  - **Note:** Most quality LiFePO4 batteries include BMS - verify before purchase

---

### 2. Position Feedback (Encoder)

**Rotary Encoder:**
- **Part:** US Digital E4T Miniature Optical Kit Encoder (1000 PPR)
- **Alternative:** AMT102-V (1024 PPR) or similar
- **Supplier:** US Digital, Amazon, Digi-Key
- **Price:** ~$50-150
- **Specifications:**
  - 1000+ pulses per revolution (PPR)
  - Quadrature output (A/B channels)
  - 5V logic level
  - Mount on winch drum shaft

**Encoder Mounting:**
- **Option A:** Direct coupling to winch drum shaft
- **Option B:** Belt/pulley drive (1:1 or geared ratio)
- **Note:** Encoder must rotate with winch drum to measure cable position

---

### 3. Tension Monitoring (Load Cell) - **ONE PER WINCH**

**⚠️ Required for Ride Classification:** One load cell per winch is required for proper tension monitoring and safety compliance. This provides independent monitoring of each winch's load, which is essential if the system is classified as a ride by any state.

**Load Cell (Per Winch):**
- **Quantity:** 1 per winch (2 total for dual-winch system)
- **Part:** 500 lb S-Type Load Cell (strain gauge)
- **Supplier:** SparkFun, Amazon, AliExpress
- **Price:** ~$20-50 per load cell
- **Specifications:**
  - 500 lb capacity (with safety margin for 300 lb max load)
  - 4-wire strain gauge (Wheatstone bridge)
  - Mount between winch cable and harness attachment point
  - **Mounting:** One load cell per winch, mounted in-line with winch cable

**Load Cell Amplifier (Per Winch):**
- **Quantity:** 1 per winch (2 total for dual-winch system)
- **Part:** HX711 24-Bit ADC Amplifier Module
- **Supplier:** SparkFun, Amazon, AliExpress
- **Price:** ~$5-10 per amplifier
- **Specifications:**
  - 24-bit ADC resolution
  - SPI-like interface (SCK, DT pins)
  - 5V operation
  - **Library:** Use `HX711.h` Arduino library
- **Note:** Each winch requires its own HX711 amplifier for independent tension monitoring

---

### 4. Safety Systems - **ONE PER WINCH**

**⚠️ Required for Ride Classification:** One electromagnetic brake per winch is required for independent safety control. This provides redundant braking capability and is essential for ride classification compliance.

**Electromagnetic Brake (Per Winch):**
- **Quantity:** 1 per winch (2 total for dual-winch system)
- **Part:** 24V DC Electromagnetic Brake (fail-safe)
- **Supplier:** Automation Direct, McMaster-Carr, Amazon
- **Price:** ~$100-200 per brake
- **Specifications:**
  - 24V DC operation
  - Fail-safe (brake engages when power is removed)
  - Mount on motor shaft or gearbox output
  - **Mounting:** One brake per winch motor/gearbox
  - **Note:** Brake engages when ESP32 pin is LOW (fail-safe)
  - **Redundancy:** Each winch has independent brake control for safety compliance

**Emergency Stop Button:**
- **Part:** Industrial E-Stop Button (NO contact, red mushroom)
- **Supplier:** Automation Direct, McMaster-Carr, Amazon
- **Price:** ~$20-50
- **Specifications:**
  - Normally Open (NO) contact
  - Active LOW (button pressed = LOW signal)
  - Mount in accessible location (console, wall, etc.)

**Relay for Brake Control (Per Winch):**
- **Quantity:** 1 per winch (2 total for dual-winch system)
- **Part:** 5V Relay Module (if brake requires higher current)
- **Supplier:** Amazon, AliExpress
- **Price:** ~$5-10 per relay module
- **Note:** ESP32 GPIO can drive small relays directly, but use relay module for isolation
- **Mounting:** One relay module per brake for independent control

---

### 5. ESP32 Development Board

**ESP32 DevKit:**
- **Part:** ESP32 DevKit v1 or similar
- **Supplier:** SparkFun, Adafruit, Amazon
- **Price:** ~$10-20
- **Specifications:**
  - WiFi capability (for UDP communication)
  - Multiple GPIO pins (PWM, interrupts, ADC)
  - 3.3V logic level

**Power for ESP32:**
- **Part:** 5V USB power supply or 3.3V regulator
- **Supplier:** Amazon
- **Price:** ~$5-10
- **Note:** ESP32 can be powered via USB or external 5V supply

---

## Wiring Diagram

### Motor Control Wiring

```
ESP32 GPIO 12 (PWM) ──→ Motor Controller PWM Input (Front Winch)
ESP32 GPIO 13 (PWM) ──→ Motor Controller PWM Input (Rear Winch)
ESP32 GPIO 14 (Digital) ──→ Motor Controller Direction Pin (Front)
ESP32 GPIO 15 (Digital) ──→ Motor Controller Direction Pin (Rear)

24V LiFePO4 Battery ──→ Motor Controller Power Input
Motor Controller Output ──→ 24V DC Gearmotor
```

**Battery Wiring:**
```
24V LiFePO4 Battery (+) ──→ Motor Controller Power Input (+)
24V LiFePO4 Battery (-) ──→ Motor Controller Power Input (-)
24V LiFePO4 Battery (+) ──→ Brake Power Input (+) [via relay]
24V LiFePO4 Battery (-) ──→ Brake Power Input (-) [via relay]

Battery Charger (in enclosure) ──→ Battery (permanently connected)
Charging Extension Cable ──→ Battery Charger AC Input (plug in for charging)
```

**⚠️ Safety Notes:**
- **Battery Disconnect Switch:** Use disconnect switch to isolate battery from system when needed (maintenance, emergency)
- **Charging Protocol:** 
  - Extension cable must be **stowed and unplugged** during operation
  - Plug in extension cable **after hours** for overnight charging
  - Unplug and stow extension cable **before operation** begins
  - Visual inspection: Ops Tech verifies cable is stowed before each session
- **Charger Integration:** Charger is permanently connected to battery inside enclosure (no need to disconnect for charging)

### Encoder Wiring

```
Front Encoder A ──→ ESP32 GPIO 18 (Interrupt-capable)
Front Encoder B ──→ ESP32 GPIO 19
Front Encoder VCC ──→ 5V (or 3.3V if encoder supports)
Front Encoder GND ──→ GND

Rear Encoder A ──→ ESP32 GPIO 21 (Interrupt-capable)
Rear Encoder B ──→ ESP32 GPIO 22
Rear Encoder VCC ──→ 5V (or 3.3V if encoder supports)
Rear Encoder GND ──→ GND
```

### Load Cell Wiring

```
Front Load Cell ──→ HX711 Amplifier (4-wire: E+, E-, A+, A-)
HX711 SCK ──→ ESP32 GPIO 25
HX711 DT ──→ ESP32 GPIO 26
HX711 VCC ──→ 5V
HX711 GND ──→ GND

Rear Load Cell ──→ HX711 Amplifier
HX711 SCK ──→ ESP32 GPIO 32
HX711 DT ──→ ESP32 GPIO 33
HX711 VCC ──→ 5V
HX711 GND ──→ GND
```

### Safety Brake Wiring (One Per Winch)

```
Front Winch:
ESP32 GPIO 27 ──→ Front Relay Module Input
Front Relay Output ──→ Front 24V Electromagnetic Brake
24V LiFePO4 Battery ──→ Front Brake (via front relay)

Rear Winch:
ESP32 GPIO 4 ──→ Rear Relay Module Input
Rear Relay Output ──→ Rear 24V Electromagnetic Brake
24V LiFePO4 Battery ──→ Rear Brake (via rear relay)
```

**Note:** Each winch has its own independent brake and relay. Brake engages when GPIO is LOW (fail-safe). Use relay for isolation and higher current capacity. Each brake draws power from battery (typically 1-2A per brake). **This redundancy is required for ride classification compliance.**

### Emergency Stop Wiring

```
E-Stop Button (NO contact) ──→ ESP32 GPIO 0 (with pull-up)
E-Stop Button Other Terminal ──→ GND
```

**Note:** Button pressed = LOW signal (triggers interrupt)

---

## Mechanical Assembly

### Winch Drum Design

**Drum Specifications:**
- **Diameter:** 2-4 inches (determines cable speed)
- **Cable Capacity:** 50-100 feet of cable
- **Material:** Steel or aluminum
- **Mounting:** Direct to gearmotor output shaft or via coupling

**Cable Speed Calculation:**
```
Speed (inches/second) = (Motor RPM / Gear Ratio) * (Drum Circumference / 60)
Example: 200 RPM motor, 50:1 gearbox, 2" diameter drum
Speed = (200 / 50) * (6.28 / 60) = 4 * 0.105 = 0.42 inches/second
```

**For 12 inches/second target:**
- Need higher motor RPM, lower gear ratio, or larger drum diameter
- Example: 2000 RPM motor, 20:1 gearbox, 2" drum = ~10.5 inches/second

### Encoder Mounting

**Option A: Direct Shaft Coupling**
- Couple encoder directly to winch drum shaft
- Requires flexible coupling or precision alignment
- **Pros:** Direct measurement, no backlash
- **Cons:** Requires precise alignment

**Option B: Belt/Pulley Drive**
- Use timing belt or pulley to drive encoder from drum
- 1:1 ratio recommended (or known ratio for calculation)
- **Pros:** Easier mounting, isolation from vibration
- **Cons:** Potential for belt slip (use timing belt)

### Load Cell Mounting (One Per Winch)

**Mounting Location:**
- **Front Winch:** Load cell mounted between front winch cable and front harness attachment point (shoulder-hook)
- **Rear Winch:** Load cell mounted between rear winch cable and rear harness attachment point (pelvis-hook)
- Use swivel/clevis to prevent side loading
- Protect from impact and weather

**Mounting Hardware (Per Winch):**
- Winch cable → Load cell → Swivel/clevis → Harness attachment point
- Ensure load cell is in tension only (no compression or side loads)
- **Independent Monitoring:** Each winch has its own load cell for independent tension measurement

---

## Calibration Procedures

### Encoder Calibration

1. **Measure Winch Drum Circumference:**
   - Measure drum diameter: `D` (inches)
   - Calculate circumference: `C = π * D` (inches)

2. **Configure Encoder Pulses Per Inch:**
   ```cpp
   const float encoderPulsesPerRevolution = 1000.0f;  // Your encoder PPR
   const float winchDrumCircumference = 6.28f;      // Measured circumference
   const float encoderPulsesPerInch = encoderPulsesPerRevolution / winchDrumCircumference;
   ```

3. **Test Position Accuracy:**
   - Move winch known distance (e.g., 12 inches)
   - Verify encoder pulse count matches expected value
   - Adjust `encoderPulsesPerInch` if needed

### Load Cell Calibration (Per Winch)

**⚠️ Important:** Each winch's load cell must be calibrated independently.

**Front Winch Load Cell:**
1. **Tare (Zero) Calibration:**
   - With no load, read front HX711 raw value
   - Set `frontLoadCellZeroOffset` to this value

2. **Scale Calibration:**
   - Apply known weight (e.g., 100 lbs) to front winch
   - Read front HX711 raw value
   - Calculate calibration factor:
     ```cpp
     frontLoadCellCalibrationFactor = KnownWeight / (RawValue - ZeroOffset);
     ```

3. **Test Accuracy:**
   - Apply various known weights to front winch
   - Verify calculated weight matches actual weight
   - Adjust calibration factor if needed

**Rear Winch Load Cell:**
- Repeat same calibration procedure for rear winch load cell
- Use separate calibration factors: `rearLoadCellZeroOffset` and `rearLoadCellCalibrationFactor`

### Motor Speed Calibration

1. **Measure Maximum Speed:**
   - Set PWM to 255 (100%)
   - Measure time to move known distance
   - Calculate actual max speed (inches/second)

2. **Configure Speed Mapping:**
   ```cpp
   const float maxSpeed = 12.0f;  // Measured max speed (inches/second)
   // PWM = (TargetSpeed / maxSpeed) * 255
   ```

---

## Battery Management & Monitoring

### Battery Voltage Monitoring

**ESP32 ADC Input for Battery Voltage:**
- **Part:** Voltage Divider Circuit (2 resistors)
- **Wiring:**
  ```
  24V Battery (+) ──→ [R1: 10kΩ] ──→ ESP32 GPIO 34 (ADC1_CH6)
                                      ──→ [R2: 2.2kΩ] ──→ GND
  ```
- **Calculation:**
  ```
  ADC Reading (0-4095) → Voltage = (ADC / 4095) * 3.3V * (R1+R2)/R2
  Example: ADC = 3000 → Voltage = (3000/4095) * 3.3 * (10k+2.2k)/2.2k = 0.73 * 3.3 * 5.45 = 13.1V
  Wait, that's wrong for 24V. Let me recalculate:
  
  For 24V max: Voltage Divider Ratio = 24V / 3.3V = 7.27
  R1 = 10kΩ, R2 = 1.5kΩ → Ratio = (10k+1.5k)/1.5k = 7.67 ✓
  
  Voltage = (ADC / 4095) * 3.3V * 7.67
  Example: ADC = 3000 → Voltage = (3000/4095) * 3.3 * 7.67 = 18.5V
  ```

**Firmware Implementation:**
```cpp
// Battery voltage monitoring (add to firmware)
const int batteryVoltagePin = 34;  // ADC1_CH6 (GPIO 34)
const float voltageDividerRatio = 7.67f;  // (R1+R2)/R2

float readBatteryVoltage() {
  int adcValue = analogRead(batteryVoltagePin);
  float voltage = (adcValue / 4095.0f) * 3.3f * voltageDividerRatio;
  return voltage;
}

// Battery state of charge (SOC) estimation
float estimateBatterySOC(float voltage) {
  // LiFePO4 8S: 28.8V = 100%, 20V = 0%
  // Linear approximation (not perfect, but good enough)
  float soc = ((voltage - 20.0f) / (28.8f - 20.0f)) * 100.0f;
  return clamp(soc, 0.0f, 100.0f);
}
```

### Battery Safety Features

1. **Low Voltage Protection:**
   - Monitor battery voltage continuously
   - Stop winch operation if voltage drops below 22V (10% SOC)
   - Engage brakes and enter safe mode
   - Send low battery warning to Unreal Engine server

2. **Overcurrent Protection:**
   - Motor controller should have built-in overcurrent protection
   - Monitor current draw (if motor controller provides current feedback)
   - Limit peak current to prevent battery damage

3. **Battery Disconnect Switch:**
   - Install manual battery disconnect switch for emergency isolation
   - Mount in accessible location (near console or E-Stop)
   - Use high-current switch (100A+ rating)

4. **Charging Safety:**
   - **Extension Cable Protocol:** Extension cable must be stowed and unplugged during operation
   - **After-Hours Charging:** Plug in extension cable only after operation hours (overnight charging)
   - **Pre-Operation Check:** Ops Tech must verify extension cable is unplugged and stowed before each session
   - **Charger Location:** Charger is integrated in battery enclosure (permanently connected to battery)
   - **Visual Inspection:** Perform visual check of cable storage compartment before operation
   - Monitor battery temperature during charging (BMS handles this automatically)
   - Never charge damaged or swollen batteries

### Battery Life & Maintenance

**Cycle Life:**
- LiFePO4: 2000-5000 cycles (80% capacity retention)
- Daily use (1 cycle/day): 5-13 years of operation
- Proper charging/discharging extends life

**Storage:**
- Store at 50-80% SOC if not in use for extended periods
- Store in cool, dry location
- Check voltage monthly if stored long-term

**Replacement Indicators:**
- Capacity drops below 80% of original
- Voltage sags significantly under load
- BMS reports cell imbalance issues

## Safety Considerations

### Hardware Safety

1. **Fail-Safe Brake (One Per Winch):**
   - Each winch has its own independent electromagnetic brake
   - Brake engages when power is removed (LOW signal)
   - Test brake engagement on power loss for each winch independently
   - Verify each brake holds load when engaged
   - **Redundancy:** If one brake fails, the other winch's brake can still provide safety

2. **Emergency Stop:**
   - Hardware E-Stop button interrupts motor control
   - E-Stop immediately engages brakes
   - E-Stop must be easily accessible

3. **Overload Protection (Per Winch):**
   - Monitor each winch's load cell independently
   - Stop motors if either winch's load exceeds 300 lbs
   - Implement software limits in firmware for each winch
   - **Independent Monitoring:** Each winch's tension is monitored separately for safety compliance

4. **Position Limits:**
   - Software limits: Stop at max height (120 inches)
   - Hardware limit switches (optional but recommended)
   - Prevent over-travel in both directions

### Software Safety

1. **Redundancy:**
   - Dual-winch system provides redundancy
   - If one winch fails, other can support load (at reduced capacity)
   - Monitor both winches independently

2. **Watchdog Timer:**
   - Implement watchdog to detect firmware crashes
   - Auto-engage brakes on watchdog timeout

3. **Network Timeout:**
   - If no commands received for 2+ seconds, engage brakes
   - Prevents runaway if network fails

4. **Battery Low Voltage Protection:**
   - Monitor battery voltage continuously
   - Stop operation if voltage < 22V (10% SOC)
   - Engage brakes and send warning to server
   - Prevent deep discharge (damages battery)

---

## Cost Estimate (Per Winch)

| Component | Quantity | Low Cost | High Cost |
|-----------|----------|----------|-----------|
| Motor Controller (RoboClaw) | 1 per winch | $150 | $200 |
| 24V DC Gearmotor | 1 per winch | $100 | $300 |
| Rotary Encoder | 1 per winch | $50 | $150 |
| **Load Cell** | **1 per winch** | **$20** | **$50** |
| **HX711 Amplifier** | **1 per winch** | **$5** | **$10** |
| **Electromagnetic Brake** | **1 per winch** | **$100** | **$200** |
| **Relay Module (Brake Control)** | **1 per winch** | **$5** | **$10** |
| E-Stop Button | 1 (shared) | $20 | $50 |
| ESP32 DevKit | 1 (shared) | $10 | $20 |
| **24V LiFePO4 Battery (50-100 Ah)** | **1 (shared)** | **$200** | **$500** |
| **Battery Charger (10A, integrated)** | **1 (shared)** | **$50** | **$100** |
| **Battery Enclosure (with charger mount)** | **1 (shared)** | **$50** | **$150** |
| **Charging Extension Cable (10-15 ft)** | **1 (shared)** | **$10** | **$30** |
| **Battery Disconnect Switch** | **1 (shared)** | **$20** | **$50** |
| Cables, Connectors, Mounting | Per system | $50 | $100 |
| **Total (Per Winch)** | **1 winch** | **$435** | **$920** |
| **Total (2 Winches + Shared Components)** | **2 winches** | **$1,550** | **$3,560** |

**Note:** Load cells, brakes, and relays are required **one per winch** for independent monitoring and safety control. This is essential for ride classification compliance.

**Note:** Battery system is shared between both winches (single 24V battery powers both motor controllers). Battery capacity should be sized for total system power draw (2 winches). Battery enclosure includes integrated 10A charger and storage for charging extension cable.

**⚠️ Per-Winch Components (Required for Ride Classification):**
- **Load Cell:** 1 per winch (2 total) - Independent tension monitoring
- **HX711 Amplifier:** 1 per winch (2 total) - One amplifier per load cell
- **Electromagnetic Brake:** 1 per winch (2 total) - Independent brake control
- **Relay Module:** 1 per winch (2 total) - One relay per brake

This redundancy is essential if the system is classified as a ride by any state, as it provides independent monitoring and safety control for each winch.

**Note:** DIY H-Bridge option reduces cost by ~$100-150 per winch, but requires more engineering.

**Battery Cost Notes:**
- Battery cost varies significantly by capacity and quality
- 50 Ah battery: ~$200-300 (1-2 hour sessions)
- 100 Ah battery: ~$400-600 (full-day operation)
- 200 Ah battery: ~$800-1200 (extended operation, multiple sessions)
- Higher capacity = longer runtime, but also higher weight and cost

---

## Recommended Suppliers

**Electronics & Control:**
- **Pololu** - Motor controllers, encoders
- **SparkFun** - ESP32, load cells, HX711
- **Automation Direct** - Industrial components, brakes, E-Stops
- **Digi-Key / Mouser** - Electronic components
- **Amazon** - General components, cables, connectors

**Structural & Mechanical:**
- **McMaster-Carr** - Mechanical components, hardware, steel tubing
- **Metal Supermarkets** - Steel square tube, round tube (local pickup)
- **Online Metals** - Steel tubing, custom cuts
- **Local Steel Suppliers** - Square tube, round tube, welding supplies

**Battery & Power:**
- **Battle Born Batteries** - LiFePO4 batteries, chargers
- **Ampere Time** - LiFePO4 batteries, chargers
- **Amazon** - Battery chargers, extension cables

**Professional Services:**
- **Structural Engineer** - Truss design, load calculations, regulatory compliance
- **Certified Welder** - Truss fabrication per engineering drawings
- **Amusement Ride Inspector** - Regulatory compliance verification (if required by local regulations)
- **Legal Counsel** - Review local and state regulations, liability considerations

---

## Battery Selection Guide

### Capacity Recommendations

**For 1-2 Hour Sessions (50-100 Ah):**
- Suitable for: Pop-up installations, trade shows, short sessions
- Weight: ~15-20 lbs
- Cost: $200-400
- **Example:** Battle Born 50 Ah, Ampere Time 50 Ah

**For Full-Day Operation (100-200 Ah):**
- Suitable for: Permanent installations, all-day operation
- Weight: ~25-40 lbs
- Cost: $400-800
- **Example:** Battle Born 100 Ah, Ampere Time 100 Ah

**For Extended Operation (200+ Ah):**
- Suitable: Multiple sessions per day, high-traffic venues
- Weight: 40+ lbs
- Cost: $800-1200
- **Example:** Battle Born 200 Ah, custom-built packs

### Battery Placement & Enclosure

**Mounting Location:**
- **Primary Mounting:** Battery is mounted near the base of one leg of the dual-winch truss chassis
- **Rationale:** 
  - Low center of gravity (improves stability)
  - Easy access for charging/disconnect operations
  - Protected location (less likely to be damaged during operation)
  - Weight distribution (battery weight helps stabilize truss base)
- **Mounting Method:** Secure battery box/enclosure bolted to truss leg base plate
- **Ventilation:** Ensure battery enclosure has adequate ventilation (LiFePO4 generates minimal heat, but ventilation prevents overheating)

**Battery Enclosure Design:**
- **Integrated Components:**
  - 24V LiFePO4 battery pack (50-200 Ah)
  - 10A LiFePO4 battery charger (built into enclosure)
  - Battery disconnect switch (mounted on enclosure exterior)
  - Charging extension cable storage compartment
- **Enclosure Features:**
  - Weather-resistant (IP54 or better for indoor/outdoor use)
  - Lockable access door (prevents unauthorized access during operation)
  - Ventilation slots/grilles (maintains airflow around battery)
  - Cable management (internal routing for power cables)
  - Mounting brackets (bolts to truss leg base plate)

**Charging Extension Cable:**
- **Purpose:** AC power cable for connecting battery charger to wall outlet
- **Specifications:**
  - Length: 10-15 feet (allows charging without moving battery)
  - Rating: 15A minimum (for 10A charger with safety margin)
  - Connector: Standard NEMA 5-15P (US) or appropriate for your region
  - Storage: Coiled and stowed in dedicated compartment within battery enclosure
- **Safety Protocol:**
  - **During Operation:** Extension cable must be stowed and unplugged
  - **After Hours:** Ops Tech plugs in extension cable for overnight charging
  - **Before Operation:** Ops Tech unplugs and stows extension cable
  - **Visual Check:** Ops Tech verifies cable is stowed before each session

**Cable Management:**
- **Battery to Motor Controllers:**
  - Use appropriate wire gauge for high current (8-10 AWG for battery to motor controller)
  - Keep battery cables as short as possible (reduces voltage drop)
  - Use proper connectors (Anderson PowerPole, XT90, or similar high-current connectors)
  - Route cables along truss structure (protect from pinch points and abrasion)
- **Charging Cable:**
  - Stow extension cable in dedicated compartment when not in use
  - Use cable management (velcro straps, cable ties) to keep cable organized
  - Ensure cable does not interfere with winch operation or player movement

## Next Steps

1. **Regulatory Research** - Check local and state amusement ride regulations, building codes, and safety standards
2. **Structural Engineering** - Engage licensed structural engineer for truss design, load calculations, and regulatory compliance verification
3. **Source Components** - Order parts based on this specification
4. **Select Battery Capacity** - Choose based on expected runtime requirements
5. **Design Battery Enclosure** - Design enclosure with integrated charger mount and cable storage compartment
6. **Truss Fabrication** - Fabricate truss per engineering drawings (certified welder, material certifications)
7. **Mechanical Design** - Design winch drum, mounting brackets, cable routing, battery mounting on truss leg base
8. **Electrical Assembly** - Wire all components per wiring diagram, install battery disconnect switch, integrate charger in enclosure
9. **Firmware Integration** - Uncomment NOOP sections in firmware, add HX711 library, add battery voltage monitoring
10. **Calibration** - Perform encoder, load cell, and motor speed calibration
11. **Battery Testing** - Test battery capacity, charging, and low-voltage protection
12. **Charging Protocol Testing** - Verify extension cable stowage, charging procedure, and safety checks
13. **Structural Inspection** - Professional inspection of truss, welds, and mounting connections (may be required by local authorities)
14. **Height Verification** - Measure and document truss height (check local regulations for specific height requirements)
15. **Safety Testing** - Test all safety systems (brake, E-Stop, overload protection, battery low-voltage)
16. **Integration Testing** - Test with Unreal Engine server
17. **Regulatory Compliance** - Submit documentation to local amusement ride authorities and obtain necessary permits (if required by local regulations)

---

## Firmware Notes

The firmware (`SuperheroFlightExperience_ECU.ino`) includes:
- ✅ Hardware pin definitions for all components
- ✅ Encoder ISR functions (uncomment to enable)
- ✅ Load cell HX711 interface (add library and uncomment)
- ✅ Brake control (fail-safe logic)
- ✅ Emergency stop ISR (uncomment to enable)
- ⚠️ **NOOP Sections:** Encoder interrupts, HX711 initialization, E-Stop interrupt need to be uncommented and configured

**Required Arduino Libraries:**
- `HX711.h` - For load cell amplifier (install via Library Manager)

**Battery Monitoring (Firmware Addition):**
- Add battery voltage monitoring pin (GPIO 34, ADC1_CH6)
- Implement voltage divider reading function
- Add battery SOC estimation
- Add low-voltage protection logic
- Send battery voltage/SOC to Unreal Engine server (add to telemetry struct)

---

## Support

For questions or issues with hardware integration:
1. Check firmware comments for NOOP sections
2. Verify wiring matches diagrams
3. Test components individually before full integration
4. Consult component datasheets for specific requirements

---

## Legal Disclaimer

**⚠️ IMPORTANT - READ CAREFULLY:**

This documentation is provided "as-is" for informational purposes only. The author of HoloCade disclaims all liability for:
- Use of this documentation
- Any injuries, damages, or losses resulting from use of this hardware specification
- Regulatory compliance or non-compliance
- Structural failures or safety incidents
- Any other consequences of implementing this design

**Your Responsibilities:**
- Verify all safety requirements with licensed professionals
- Check local and state regulations before installation
- Obtain necessary permits and inspections
- Engage structural engineers for design verification
- Consult with amusement ride inspectors (if required)
- Ensure compliance with all applicable laws and regulations
- Maintain adequate insurance coverage
- Follow all manufacturer specifications for components

**Regulatory Compliance:**
- Amusement ride regulations vary significantly by state, city, and jurisdiction
- Building codes and safety standards may require professional engineering
- Some jurisdictions may require permits, inspections, or certifications
- This documentation does not guarantee compliance with any specific regulations
- You are solely responsible for ensuring compliance with all applicable laws

**Professional Consultation Required:**
This documentation is not a substitute for professional engineering, legal, or regulatory advice. Always consult with licensed professionals before implementing this design.

---

**Last Updated:** 2025-01-XX
**Version:** 1.0

