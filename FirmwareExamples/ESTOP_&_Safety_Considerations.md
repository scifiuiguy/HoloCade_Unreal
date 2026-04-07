# E-Stop & Safety Considerations

<details>
<summary><strong>⚠️Author Disclaimer:</strong></summary>

<div style="margin-left: 20px;">
This plugin provides code and advice that may or may not run on systems your local and state officials may classify as  "amusement rides" or "theme park rides" which may fall under ASTM standards or other local regulations. HoloCade's author disclaims any and all liability for any use of this code, including for safety of guests or patrons, regulatory readiness, etc. Please review the local regulations in your area prior to executing this code in any public venue. You are responsible for compliance in your state.
</div>

</details>

<details>
<summary><strong>⚠️Industrial Equipment Modification Warning:</strong></summary>

<div style="margin-left: 20px;">

**Warranty Voiding:**
- Using industrial equipment (scissor lifts, pallet stackers, forklifts, etc.) for VR motion platforms or entertainment applications **voids all manufacturer warranties**.
- Manufacturers (Genie, Skyjack, Crown, Raymond, etc.) do not support or endorse using their equipment in this manner.
- Modifications such as removing forks/rails, adding tilt chassis, bypassing safety interlocks, or integrating custom control systems are done **entirely at your own risk**.

**Liability:**
- You are solely responsible for:
  - Safety of all operators and participants
  - Structural integrity of all modifications
  - Regulatory compliance (amusement ride/theme park standards)
  - Insurance and liability coverage
  - Proper installation, testing, and maintenance
- Always consult qualified structural engineers, electrical engineers, and safety professionals before modifying industrial equipment.
- Follow all local regulations for amusement ride/theme park equipment in your jurisdiction.

**Safety Interlocks:**
- Industrial equipment includes safety interlocks (operator presence, deadman switches, weight sensors, etc.) that may need to be satisfied or bypassed for automated control.
- Bypassing safety interlocks creates significant safety risks and may violate regulations.
- Always maintain redundant safety systems (E-Stop, limit switches, software limits) when modifying industrial equipment.

**The author of HoloCade disclaims any and all liability for modifications to industrial equipment, warranty voiding, safety incidents, regulatory violations, or any other consequences of using industrial equipment in unintended applications.**

</div>

</details><br>

## Overview

E-Stop (Emergency Stop) systems are **external safety systems** implemented upstream from everything in an HoloCade experience that requires significant current, including hydraulics, servo motors, low-current PCBs that drive these systems, etc. The rule-of-thumb is... if it's physically possible for the thing to cause a bruise or worse under ANY scenario ever... E-Stop.

E-Stop systems cut power at the battery/power supply level, before power reaches the shield board or any other system components. This document provides guidance for sizing, installing, testing, and maintaining E-Stop systems for HoloCade installations.

**⚠️ Critical:** E-Stop systems are safety-critical. Never bypass, modify, or disable E-Stop circuits. Always test E-Stop functionality before each use. Consult qualified electrical engineer for installation and testing if unsure of compliance requirements.

## E-Stop Circuit Requirements

**Location:** High-side switching (cutting +V) immediately after main circuit breaker, as close to battery/power supply as possible, before barrel jack.

**Standard Practice:** Single contactor at battery/power supply cuts ALL power (high-current loads + shield board).

(lib_symbols
	(symbol "power:GND"
		(power)
		(pin_numbers
			(hide yes)
		)
		(pin_names
			(offset 0)
			(hide yes)
		)
		(exclude_from_sim no)
		(in_bom yes)
		(on_board yes)
		(property "Reference" "#PWR"
			(at 0 -6.35 0)
			(effects
				(font (size 1.27 1.27))
				(hide yes)
			)
		)
		(property "Value" "GND"
			(at 0 -3.81 0)
			(effects
				(font (size 1.27 1.27))
			)
		)
		(property "Footprint" ""
			(at 0 0 0)
			(effects
				(font (size 1.27 1.27))
				(hide yes)
			)
		)
		(property "Datasheet" ""
			(at 0 0 0)
			(effects
				(font (size 1.27 1.27))
				(hide yes)
			)
		)
		(property "Description" "Power symbol creates a global label with name \"GND\" , ground"
			(at 0 0 0)
			(effects
				(font (size 1.27 1.27))
				(hide yes)
			)
		)
		(property "ki_keywords" "global power"
			(at 0 0 0)
			(effects
				(font (size 1.27 1.27))
				(hide yes)
			)
		)
		(symbol "GND_0_1"
			(polyline
				(pts
					(xy 0 0) (xy 0 -1.27) (xy 1.27 -1.27) (xy 0 -2.54) (xy -1.27 -1.27) (xy 0 -1.27)
				)
				(stroke (width 0) (type default))
				(fill (type none))
			)
		)
		(symbol "GND_1_1"
			(pin power_in line
				(at 0 0 270)
				(length 0)
				(name "~"
					(effects
						(font (size 1.27 1.27))
					)
				)
				(number "1"
					(effects
						(font (size 1.27 1.27))
					)
				)
			)
		)
		(embedded_fonts no)
	)
)
(symbol
	(lib_id "power:GND")
	(at 162.56 153.67 0)
	(unit 1)
	(exclude_from_sim no)
	(in_bom yes)
	(on_board yes)
	(dnp no)
	(fields_autoplaced yes)
	(uuid "dd79c449-3789-44f3-a0e5-6859e6f44193")
	(property "Reference" "#PWR026"
		(at 162.56 160.02 0)
		(effects
			(font (size 1.27 1.27))
			(hide yes)
		)
	)
	(property "Value" "GND"
		(at 162.56 158.75 0)
		(effects
			(font (size 1.27 1.27))
		)
	)
	(property "Footprint" ""
		(at 162.56 153.67 0)
		(effects
			(font (size 1.27 1.27))
			(hide yes)
		)
	)
	(property "Datasheet" ""
		(at 162.56 153.67 0)
		(effects
			(font (size 1.27 1.27))
			(hide yes)
		)
	)
	(property "Description" "Power symbol creates a global label with name \"GND\" , ground"
		(at 162.56 153.67 0)
		(effects
			(font (size 1.27 1.27))
			(hide yes)
		)
	)
	(pin "1"
		(uuid "ddaf8e9a-de08-4a99-a788-d5de9882a918")
	)
	(instances
		(project "HoloCade_Universal_Shield"
			(path ""
				(reference "#PWR026")
				(unit 1)
			)
		)
	)
)
**Standards Compliance:** IEC 60204-1, NFPA 79 (high-side switching required).

**Why High-Side Switching?**
- **Fault tolerance:** Shorts to GND can't bypass high-side switch
- **Better isolation:** Removes voltage from load side when opened
- **Standards compliance:** IEC 60204-1, NFPA 79 require high-side switching
- **Easier fault detection:** Can detect failed-open contactor (voltage present when it shouldn't be)

**Circuit Topology:**
```
Battery/Power Supply (+24_12V)
    ↓
Main Circuit Breaker/Fuse (overcurrent protection)
    ↓
High-Side Relay/Contactor (controlled by E-Stop switch)
    ↓
E-Stop Switch (NC, latching) - controls relay coil
    ├─→ Relay Coil (cuts ALL power: high-current loads + board)
    └─→ Distribution (fuses, breakers, individual loads)
```

## E-Stop Contactor Sizing

The E-Stop contactor must handle the combined current draw of all systems:

### Scissor Lift Systems

- **Skyjack scissor lifts:** Reference Skyjack operator manuals for specific model current ratings
  - Typical range: 15-30A @ 24V for small lifts (SJ3219, SJ3226)
  - Typical range: 30-60A @ 24V for medium lifts (SJ4632, SJ4642)
  - Typical range: 60-100A @ 24V for large lifts (SJ6832, SJ6942)
- **Genie scissor lifts:** Reference Genie operator manuals for specific model current ratings
  - Typical range: 15-30A @ 24V for small lifts (GS-1930, GS-2632)
  - Typical range: 30-60A @ 24V for medium lifts (GS-3246, GS-4047)
  - Typical range: 60-100A @ 24V for large lifts (GS-5390, GS-6868)

### Hydraulic Actuator Systems

- **Hydraulic pump motors:** Reference pump manufacturer datasheets
  - Typical range: 20-50A @ 24V for small pumps (1-2 HP)
  - Typical range: 50-100A @ 24V for medium pumps (3-5 HP)
  - Typical range: 100-200A @ 24V for large pumps (7.5-10 HP)
- **Actuator drivers/valves:** Reference driver/valve manufacturer datasheets
  - Typical range: 5-15A @ 24V per actuator driver
  - Typical range: 1-5A @ 24V per solenoid valve

### Shield Board Power

- Shield board + MCU: ~2-5A @ 24V (typically <1A, but allow headroom)

### Total Current Calculation

```
Total E-Stop Contactor Rating = 
    Scissor Lift Current (from manufacturer manual)
    + Hydraulic Pump Current (from pump datasheet)
    + (Actuator Driver Current × Number of Drivers)
    + (Solenoid Valve Current × Number of Valves)
    + Shield Board Current (~5A safety margin)
    + 25% Safety Margin (for inrush, peak loads, future expansion)
```

### Example Sizing

- Small system (1 small scissor lift + 1 small pump + 2 actuators): ~50-75A @ 24V → **100A contactor recommended**
- Medium system (1 medium scissor lift + 1 medium pump + 4 actuators): ~100-150A @ 24V → **150-200A contactor recommended**
- Large system (1 large scissor lift + 1 large pump + 6+ actuators): ~200-300A @ 24V → **300-400A contactor recommended**

### Contactor Selection

**What is a Contactor?**
A contactor is an electrically-controlled switch designed to handle high currents (typically 50A-400A or more). Think of it like a heavy-duty light switch, but instead of flipping it by hand, it's controlled by a small electrical signal (like the E-Stop switch). When the E-Stop switch is pressed, it sends a signal to the contactor's coil (a small electromagnet), which mechanically opens the contactor's main contacts, cutting all power to your system. Contactors are specifically designed for high-current applications and have built-in arc suppression to safely interrupt large currents without damage. For E-Stop applications, contactors are preferred over regular relays because they're rated for higher currents, have more robust mechanical construction, and are designed for safety-critical applications.

**Selection Criteria:**
- Use industrial-grade contactors rated for **DC operation** (DC contactors have different arc suppression than AC)
- Ensure contactor is rated for the full voltage range (12V-24V, up to 29.2V for fully charged 24V LiFePO4)
- Consider contactor with auxiliary contacts for status monitoring
- Verify contactor coil voltage matches E-Stop switch control voltage (typically 12V or 24V)
- Contactor must be **positive-opening** (mechanical opening, not dependent on spring return)

### References

- Skyjack operator manuals: [Skyjack Support](https://www.skyjack.com/en/support/)
- Genie operator manuals: [Genie Support](https://www.genielift.com/en/support/)
- Hydraulic pump datasheets: Consult pump manufacturer (Parker, Bosch Rexroth, etc.)
- Actuator driver datasheets: Consult driver manufacturer (Parker, Moog, etc.)

## Standards Compliance

**⚠️ Important Note on Regulatory Readiness:**
This safety document is **by no means a comprehensive reference on regulatory readiness**. Local officials and regulatory bodies may require any number of additional standards beyond those listed here, in addition to IPC standards. Always consult with qualified electrical engineers, safety consultants, and local regulatory authorities to ensure full compliance with all applicable standards in your jurisdiction.

**Referenced Standards:**
- **IPC-2221** (Generic Standard on Printed Board Design): Defines PCB design requirements including trace width, conductor spacing, thermal management, and material selection. For detailed IPC-2221 trace width and clearance calculators, see: [Using an IPC-2221 Calculator for High Voltage Design](https://resources.altium.com/p/using-an-ipc-2221-calculator-for-high-voltage-design)
- **IEC 60204-1** (Safety of machinery - Electrical equipment): Requires E-Stop systems to be Category 0 (immediate removal of power) or Category 1 (controlled stop then power removal). High-side switching is mandatory.
- **NFPA 79** (Electrical Standard for Industrial Machinery): Requires E-Stop circuits to be "fail-safe" (any single fault does not prevent E-Stop function). High-side switching with positive-opening contacts required.
- **OSHA 29 CFR 1910** (General Industry): Requires emergency stop devices on machinery where employee exposure to hazards exists.
- **ASTM F2291** (Amusement Rides and Devices): May apply if system is classified as an amusement ride in your jurisdiction.

## Installation Requirements

- E-Stop contactor must be rated for **125% of maximum continuous current** (per NFPA 79)
- Contactor must be **DC-rated** (not AC contactor used for DC - different arc suppression)
- Contactor must be **positive-opening** (mechanical opening, not dependent on spring return)
- E-Stop switch must be **readily accessible** and **clearly marked**
- E-Stop circuit must be **fail-safe** (single fault does not prevent operation)
- All E-Stop wiring must be **separate from control wiring** (dedicated safety circuit)

## Testing Requirements

### Initial Installation Testing

1. **Functional Test:**
   - Verify E-Stop switch immediately cuts all power when pressed
   - Verify E-Stop switch latches in "off" position until manually reset
   - Verify all systems (high-current loads + shield board) de-energize simultaneously
   - Verify E-Stop cannot be bypassed by any single fault

2. **Load Testing (Required for Compliance):**
   - **Test total load at the battery with accurate simulated max weight capacity to confirm continuous operation at max current within expected tolerance.**
   - Apply maximum rated load to all systems simultaneously:
     - Scissor lift at maximum platform capacity (reference manufacturer manual)
     - Hydraulic pump at maximum pressure/flow
     - All actuators at maximum extension/retraction force
     - All solenoids energized
   - Measure actual current draw at battery terminals using calibrated clamp meter or shunt
   - Verify continuous operation for minimum 30 minutes at max load
   - Verify E-Stop contactor current rating is ≥125% of measured max current
   - Document test results (current readings, duration, ambient temperature)

3. **Inrush Current Testing:**
   - Measure peak inrush current during system startup (all motors starting simultaneously)
   - Verify contactor can handle inrush without contact welding or degradation
   - Typical inrush: 3-5× continuous current for 100-500ms

4. **Voltage Drop Testing:**
   - Measure voltage drop across E-Stop contactor contacts at max load
   - Verify voltage drop <2% of supply voltage (per NFPA 79)
   - Excessive voltage drop indicates contactor undersizing or contact degradation

### Periodic Testing (Maintenance)

- **Daily:** Visual inspection of E-Stop switch (accessibility, clear marking, no damage)
- **Weekly:** Functional test (press E-Stop, verify all systems de-energize)
- **Monthly:** Load test at 50% capacity (verify contactor operation under load)
- **Annually:** Full load test (repeat initial installation testing)
- **After any maintenance:** Functional test before returning to service

## Documentation Requirements

- Maintain test records (date, tester name, results, any failures)
- Document contactor part number, current rating, and installation date
- Document all E-Stop circuit modifications
- Keep manufacturer datasheets for contactor and E-Stop switch on file

## Failure Modes and Troubleshooting

- **Contactor fails to open:** Check for contact welding (excessive current, undersized contactor)
- **Contactor opens but power remains:** Check for bypass wiring or secondary power sources
- **E-Stop switch doesn't latch:** Replace switch (mechanical failure)
- **Excessive voltage drop:** Check contactor contacts for pitting/corrosion, verify proper sizing

## PCB Power Trace Design (Universal Shield)

### 5V Power Rail to Aux Ports

**Configuration:**
- **Total capacity:** 4A across 8 aux ports (≥0.5A per port)
- **Distribution:** Single trace to all 8 aux ports
- **Trace width:** 1.2mm (1oz copper, 20°C temperature rise)
- **Copper weight:** 1oz (35µm) standard
- **Temperature rise:** 20°C (acceptable for this application)

**Design Rationale:**

1. **Single Trace Configuration:**
   - Simplified routing (single trace along board edge)
   - 4A capacity only reached in worst-case scenario (all 8 aux ports with ESP32-S3 at 100% duty cycle)
   - Realistic installations operate well below maximum capacity
   - Single trace is sufficient given actual load expectations

2. **Trace Width Selection:**
   - **1.2mm width:** Handles 4A with ~20°C rise (1oz copper, IPC-2221 standards)
   - Based on worst-case scenario (all 8 ports at maximum load)
   - Provides adequate margin for typical operation

3. **Temperature Rise Acceptance (20°C):**
   - 20°C rise is warm but not dangerous for PCB operation
   - 4A load only occurs if all 8 aux ports have ESP32-S3 pulling 100% duty cycle simultaneously
   - Realistic load expectations: extremely unlikely that any installation will operate at maximum capacity
   - Typical installations will operate at 20-50% of maximum capacity, resulting in <10°C rise
   - Integrated fan (optional MCU cooling fan) will provide airflow across entire board, wicking away heat

4. **Routing Strategy:**
   - 5V power trace routed along board edge
   - Takes priority over data I/O traces (power is critical path)
   - No vias required (single-layer routing)
   - Edge routing provides better heat dissipation (exposed to air/airflow)

**Current Capacity Verification:**
- **Maximum theoretical load:** 8× ESP32-S3 at full WiFi (200mA each) = 1.6A
- **Worst-case design load:** 8× ESP32-S3 at 100% duty cycle (500mA each) = 4A
- **Typical installation load:** 8× ESP32-S3 typical (80-120mA each) = 0.64-0.96A
- **Design capacity:** 4A (worst-case scenario, 4-6× safety margin over typical load)
- **Actual temperature rise at typical load:** <5°C (well within acceptable range)

**Fan Cooling Consideration:**
- Optional MCU cooling fan (40mm, 12/24V PWM) provides airflow across entire board
- Fan airflow will significantly reduce temperature rise of power traces
- Even at maximum load (4A), fan cooling should keep temperature rise well below 20°C

**Future Expansion:**
- If higher current capacity needed, can increase trace width to 1.2mm or upgrade to 2oz copper
- 2oz copper would allow 0.4-0.6mm trace width for same current capacity
- Current design provides significant headroom for future expansion

### Overcurrent Protection

**Fuse Protection (v1.0):**
- **4A fuse** installed on 5V aux power bus (protects all 8 aux ports)
- Fuse rating matches maximum design capacity (4A total)
- Protects against:
  - Excessive current draw from misbehaving devices
  - Short circuits in cables or connectors
  - Reverse current from miswired devices
- Fuse will trip if total current exceeds 4A, protecting PCB traces and buck converter

**Fuse Selection:**
- Fast-blow or slow-blow fuse acceptable (fast-blow provides better protection)
- Fuse should be rated for 5V DC operation
- Consider resettable fuse (polyfuse) for easier maintenance in field installations

### Power over Ethernet (PoE) Compatibility

**Standard PoE Devices:**
- **Standard PoE (IEEE 802.3af/at/bt) devices are NOT compatible** with Universal Shield aux ports
- Standard PoE requires:
  - 48V power (Universal Shield provides 5V)
  - PoE handshake/negotiation protocol (not implemented)
  - PoE controller chip (not present on Universal Shield)
- Standard PoE devices will not activate on 5V supply (safe, but won't work)

**Raspberry Pi and Jetson Nano:**
- **Standard devices (without PoE HATs) are SAFE** to connect
- Pins 4&5 are not connected internally on standard Ethernet jacks
- 5V power on pins 4&5 is safely ignored (no current flow, no damage)
- Standard devices will function normally via Ethernet (pins 1-3, 6)

**PoE HATs/Add-on Boards:**
- **PoE HATs are NOT supported and may be damaged**
- PoE HATs connect pins 4&5 to PoE controller chips
- PoE controllers expect 48V, not 5V
- Applying 5V to PoE controller may cause damage or malfunction
- **Do not use Raspberry Pi PoE HATs or Jetson PoE add-on boards on aux ports**

**Recommendations:**
- Document clearly that pins 4&5 provide 5V power, not PoE
- Label aux ports as "5V Power" not "PoE"
- Verify devices before connecting (check for PoE HATs/add-ons)
- Use standard Raspberry Pi/Jetson devices without PoE modifications

### Power Capacity and Copper Weight Selection

**Standard Universal Shield (1oz Copper):**
- **Safe for:** 4 or fewer powered Child ECUs drawing current from aux ports
- **Maximum safe load:** ~2A total (0.5A per port × 4 ports)
- **Temperature rise:** <10°C at typical loads, <20°C at maximum safe load
- **IPC-2221 compliance:** Meets standard for 2A @ 20°C with 1.2mm trace width

**High-Power Universal Shield (2oz Copper):**
- **Required for:** More than 4 powered Child ECUs drawing current from aux ports
- **Maximum capacity:** 4A total (0.5A per port × 8 ports)
- **Temperature rise:** <15-18°C at 4A load with 1.2mm trace width
- **IPC-2221 compliance:** Exceeds standard for 4A @ 20°C with 1.2mm trace width
- **Ordering:** Specify "high-power version" or "2oz copper" when ordering PCBs

**Fan Operation Requirements:**
- **Standard Shield (≤4 powered Child ECUs):** Fan operation optional but recommended
- **High-Power Shield (>4 powered Child ECUs):** **Fan operation REQUIRED** during operation
- Fan provides airflow across entire board, significantly reducing temperature rise
- Fan should be operational whenever more than 4 Child ECUs are drawing power simultaneously
- Fan failure should be treated as a maintenance issue requiring immediate attention

**Heat Testing Requirements:**
- **Before deployment:** Test Universal Shield under continuous maximum load with heat mapping sensors
- **Test conditions:**
  - All powered aux ports at maximum rated current (0.5A per port)
  - Continuous operation for minimum 30 minutes
  - Measure temperature rise at power traces using thermal imaging or thermocouples
  - Verify temperature rise stays within acceptable limits (<20°C for 1oz, <18°C for 2oz)
- **Documentation:** Record test results (ambient temperature, measured temperature rise, duration, number of powered ports)
- **Periodic testing:** Repeat heat testing annually or after any hardware modifications

**Installation Guidelines:**
- Count total number of Child ECUs that will draw power from aux ports
- If 4 or fewer: Standard Universal Shield (1oz copper) is sufficient
- If more than 4: Order High-Power Universal Shield (2oz copper)
- Ensure MCU cooling fan is installed and operational for High-Power installations
- Verify fan operation before deploying systems with >4 powered Child ECUs

## Additional Safety Considerations

- **QTY2 Up-to-code Fire Emergency Fire Extinguishers:** One at the Ops Tech Console and another near any hydraulic equipment.
- **Movable stairs:** Any system that causes players to be lifted into the air must have a physical means of egress in an e-stop emergency.
- **Hydraulically-actuated equipment should have multiple manual and auto e-stops** located at console and on device.
- **Theme park safety regulations vary by state** - take steps to abide by the same rules that apply to carnival equipment in your state.
- **The author of HoloCade disclaims any liability resulting in the use of this free software.**

- **QTY2 Up-to-code Fire Emergency Fire Extinguishers:** One at the Ops Tech Console and another near any hydraulic equipment.
- **Movable stairs:** Any system that causes players to be lifted into the air must have a physical means of egress in an e-stop emergency.
- **Hydraulically-actuated equipment should have multiple manual and auto e-stops** located at console and on device.
- **Theme park safety regulations vary by state** - take steps to abide by the same rules that apply to carnival equipment in your state.
- **The author of HoloCade disclaims any liability resulting in the use of this free software.**

