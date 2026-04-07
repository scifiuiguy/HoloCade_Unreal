# HoloCade Driver Shield

**Also known as:** LBDS, Driver Shield

## Overview

**Motor Control ECU for High-Current Actuator Applications:**

The HoloCade Driver Shield ("LBDS" or "Driver Shield") is a specialized ECU derived from the Universal Shield, designed for high-current motor control applications. It integrates a BTS7960 H-bridge motor driver with hall sensor feedback support, while maintaining the core connectivity features of the Universal Shield (Ethernet, CAN-FD, and aux ports).


<details>
<summary><strong>Driver Shield is a Motor Control ECU</strong></summary>

<div style="margin-left: 20px;">

The Driver Shield is based on the Universal Shield architecture but replaces 4 of the 8 aux ports with an integrated BTS7960 H-bridge motor driver. This enables direct control of high-current linear actuators (such as the PA-HD1-HALL) with integrated hall sensor feedback for position sensing. The shield maintains full compatibility with the Universal Shield's networking and communication features (Ethernet, CAN-FD) while adding dedicated motor control capabilities.

</div>

</details>

<details>
<summary><strong>Key Features</strong></summary>

<div style="margin-left: 20px;">

- Multi-platform support: ESP32-S3 (native) & STM32/Arduino (via personality adapters)
- 4× CAT5 aux ports for child ECUs (Raspberry Pi, Jetson, ESP32, STM32, Arduino)
- Integrated BTS7960 H-bridge motor driver (30A per half-bridge, 60A total)
- Hall sensor feedback support (2× ADC channels for position sensing)
- Isolated CAN-FD transceiver for high-current device control
- 100 Mbps Ethernet connectivity
- 12V/24V input with 5V power distribution (≥5A total capacity)
- 24V motor power rail (direct from input, bypasses buck converter)
- Designed for potting and long-term installations

</div>

</details>

<details>
<summary><strong>High-Level Component Diagram</strong></summary>

<div style="margin-left: 20px;">

```
          ┌────────────────────────────────────────┐
          |                                        │
    ┌─────▲───────┐    ┌───────┐ +5V               │
    │ Barrel Jack │───→│LM2576 │────┐       ┌──────▼───────┐
    │  12V/24V    │    │ Buck  │    │       │ 12/24V Fan   │
    └─────────────┘    └───────┘    │       └───────┬──────┘
         │                │+5V     │               ▼
         │                │        │  ┌────────────────────────┐
         │                │        └─→│         ESP32-S3       │
         │                │           │      (Motherboard)     │
         │                │           └──▲──────────────▲──────┘
         │                │              │              │
         │                │         ┌────▼────┐    ┌────▼────┐
         │                │         │LAN8720A │    │ADM3057E │
         │                │         │Ethernet │    │ CAN-FD  │
         │                │         └────▲────┘    └────▲────┘
         │                │              │              │
         │                │              │         ┌────▼─────┐
         │                │              │         │CAN 3-pin │
         │                │              │         └──────────┘         
         │                │              │
         │                │    ┌─────────▼───────────────┐   
         │                │    │  Aux Ports: J5 J6 J7 J8 │   
         │                │    │  (4× RJ45: Ethernet)    │   
         │                │    └─────────────────────────┘   
         │                │
         │                └──→ 5V to MCU VIN
         │
         └──→ 24V Motor Power Rail
                │
                └──→ Motor Driver Sheet
                      ├─→ BTS7960 H-Bridge (×2)
                      ├─→ 74HC244 Buffer IC
                      ├─→ MBR3045PT Dual-Channel Schottky Diode
                      └─→ Molex 6-pin Actuator Connector (PA-HD1-HALL)
                                              
```

</div>

</details>

<details>
<summary><strong>Project Files</strong></summary>

<div style="margin-left: 20px;">

- **HoloCade_Driver_Shield.kicad_pro** - KiCAD project file
- **HoloCade_Driver_Shield.kicad_sch** - Main schematic file (Sheet 1)
- **motor-driver.kicad_sch** - Motor driver hierarchical sheet (Sheet 2)
- **HoloCade_Driver_Shield.kicad_pcb** - PCB layout file

**Note:** The Driver Shield uses a hierarchical schematic design with the motor driver components on a separate sheet for better organization.

</div>

</details>

<details>
<summary><strong>Power Architecture</strong></summary>

<div style="margin-left: 20px;">

**Hybrid Approach:**
```
12V/24V Input (Barrel Jack)
    ↓
    ├─→ Buck Converter (LM2576: 12V/24V → 5V, 60V max input)
    │       ↓
    │       ├─→ 5V to MCU VIN (via adapter)
    │       │       ↓
    │       │   MCU Onboard Regulator (5V → 3.3V)
    │       │       ↓
    │       │   MCU 3.3V Output (via adapter)
    │       │       └─→ Shield Logic Components (LAN8720A, LEDs)
    │       │
    │       └─→ 5V directly to Aux Port Pins 4 (≥5.0A total across 4 ports)
    │
    └─→ 24V Motor Power Rail (direct passthrough, bypasses buck converter)
            ↓
        Motor Driver Sheet
            ├─→ BTS7960 H-Bridge (×2) - VS pins
            └─→ Actuator Power (MOTOR_A_OUT, MOTOR_B_OUT)
```

**Key Points:**
- Buck converter (LM2576HVS-5) handles 12V/24V input (up to 29.2V for fully charged 24V LiFePO4)
- Aux port power: 5V directly from buck converter (≥5.0A total capacity across 4 ports)
- Motor power: 24V direct passthrough from barrel jack (bypasses buck converter)
- Shield logic: 3.3V from ESP32-S3 pins 1-2 (3.3V_OUT)
- High-current MCUs (ESP32-S3, STM32F407): Adapter routes MCU 3.3V directly
- Low-current MCUs (Arduino): Adapter includes 3.3V regulator (AMS1117-3.3)

</div>

</details>

<details>
<summary><strong>Components</strong></summary>

<div style="margin-left: 20px;">

### Primary Components (Sheet 1 - Shared with Universal Shield)

**Primary ICs (Sheet 1):**
- **U1** - LM2576HVS-5 (TO-220/TO-263) - Buck converter (12V/24V → 5V, ≥5A, 60V max input)
- **U2-U5** - MTJ-883X1 (RJ45 aux ports J5-J8, references U2, U3, U4, U5 in schematic)
- **U6** - ESP32-S3-2x22 (MCU interface, 44-pin header)
- **U7** - ADM3057ExRW - Isolated CAN-FD transceiver (3kV isolation)
- **U8** - LAN8720A (QFN-32) - 100 Mbps Ethernet PHY
  - Management: MDC/MDIO to MCU via adapter
  - Clock: External 25MHz crystal (3225 SMD recommended, Y1)
  - Reset: ESP32-S3 EN pin (GPIO3) or separate GPIO

**3.3V Power:** ESP32-S3 pins 1-2 (3.3V_OUT) provide 3.3V for shield logic components. No separate 3.3V LDO required for ESP32-S3.
- VIO: +3.3V (logic supply) - decoupled with C14 (100nF)
- VCC: +5V (isoPower supply) - decoupled with C15 (100nF) and C16 (4.7µF tantalum)
- VISOOUT → VISOIN (Kelvin jumper required on PCB layout)
- VISOIN/VISOOUT: Isolated supply - decoupled with C17 (100nF) and C18 (4.7µF tantalum)
- CAN termination: 120Ω resistor (R12) switchable via SW1 (DIP switch) across CANH/CANL

**LEDs (3 total):**
- Power LED (Red): +3.3V → R4 (270Ω) → D1 → GND
- Ethernet Link LED (Green): LAN8720A LED1 → R5 (270Ω) → D2 → GND
- Ethernet Speed LED (Green): LAN8720A LED2 → R6 (270Ω) → D3 → GND

**Resistors (Sheet 1 - standard components):**

| Reference | Value | Purpose | Notes |
|-----------|-------|---------|-------|
| R1 | 10 kΩ | Polarity protection MOSFET gate pull-down | Pulls gate of IRF9540N P-channel MOSFET to GND for reverse polarity protection |
| R2 | 1 kΩ | Buck converter voltage divider (upper) | Forms feedback divider with R3 for LM2576 output regulation |
| R3 | 326Ω | Buck converter voltage divider (lower) | Forms feedback divider with R2 for LM2576 5V output regulation |
| R4 | 270Ω | Power LED current limiting | Limits current through red power indicator LED (D1) |
| R5 | 270Ω | Ethernet Link LED current limiting | Limits current through green LAN LED1 (D2, link status) |
| R6 | 270Ω | Ethernet Speed LED current limiting | Limits current through green LAN LED2 (D3, speed indicator) |
| R7 | 49.9Ω (49R9) | LAN8720A RBIAS pull-down | Sets bias current for LAN8720A internal reference |
| R8 | 10 kΩ | ESP32-S3 RESET_N pull-up | Pull-up resistor for LAN8720A RESET_N pin (active LOW) |
| R9 | 4.7 kΩ | LAN8720A MDIO pull-up | Pull-up resistor for ESP32-S3 GPIO18 (MDIO) management interface |
| R10 | 100Ω | Fan FET gate resistor | Current limiting resistor for IRLZ44N MOSFET gate drive (GPIO42) |
| R11 | 10 kΩ | Fan FET gate pull-down | Pull-down resistor for fan MOSFET gate (prevents floating when GPIO42 is high-Z) |
| R12 | 120Ω | CAN bus termination resistor | Switchable termination resistor across CANH/CANL (via SW1) |

**Capacitors (Sheet 1 - standard components):**

| Reference | Value | Type | Purpose | Notes |
|-----------|-------|------|---------|-------|
| C1 | 100nF | Non-polarized | Barrel jack output decoupling | High-frequency noise filtering on power input |
| C2 | 10µF | Polarized | Barrel jack output decoupling | Bulk capacitance for power input smoothing |
| C3 | 100nF | Non-polarized | Buck converter input decoupling | High-frequency noise filtering on LM2576 input |
| C4 | 100nF | Non-polarized | Buck converter output decoupling | High-frequency noise filtering on 5V output |
| C5 | 10µF | Polarized | Buck converter output decoupling | Bulk capacitance for 5V output smoothing |
| C6 | 10µF | Polarized | ESP32-S3 3.3V output decoupling | Bulk capacitance for ESP32 3.3V_OUT smoothing |
| C7 | 100nF | Non-polarized | ESP32-S3 3.3V output decoupling | High-frequency noise filtering on ESP32 3.3V_OUT |
| C8 | 100nF | Non-polarized | LAN8720A VDD2 decoupling | Power supply decoupling for LAN8720A VDD2 pin |
| C9 | 100nF | Non-polarized | LAN8720A VDDCR decoupling | Power supply decoupling for LAN8720A VDDCR pin |
| C10 | 100nF | Non-polarized | LAN8720A VDD1A decoupling | Power supply decoupling for LAN8720A VDD1A pin |
| C11 | 100nF | Non-polarized | LAN8720A VDDIO decoupling | Power supply decoupling for LAN8720A VDDIO pin |
| C12 | 10pF | Non-polarized | 25MHz crystal load capacitor (XTAL1) | Crystal oscillator load capacitor for XTAL1 |
| C13 | 10pF | Non-polarized | 25MHz crystal load capacitor (XTAL2) | Crystal oscillator load capacitor for XTAL2 |
| C14 | 100nF | Non-polarized | ADM3057E VIO decoupling | High-frequency noise filtering on CAN transceiver logic supply |
| C15 | 100nF | Non-polarized | ADM3057E VCC decoupling | High-frequency noise filtering on CAN transceiver isoPower supply |
| C16 | 4.7µF | Polarized (Tantalum) | ADM3057E VCC bulk decoupling | Bulk capacitance for CAN transceiver isoPower supply smoothing |
| C17 | 100nF | Non-polarized | ADM3057E VISO decoupling | High-frequency noise filtering on CAN transceiver isolated supply (VISOIN/VISOOUT) |
| C18 | 4.7µF | Polarized (Tantalum) | ADM3057E VISO bulk decoupling | Bulk capacitance for CAN transceiver isolated supply smoothing |

**Diodes (Sheet 1 - standard components):**

| Reference | Type | Value | Purpose | Notes |
|-----------|------|-------|---------|-------|
| D1 | LED | LED | Red status LED for 3V power | Power indicator LED, current limited by R4 (270Ω) |
| D2 | LED | LED | Green status LED for LAN LED1 pin | Ethernet link status indicator, driven by LAN8720A LED1, current limited by R5 (270Ω) |
| D3 | LED | LED | Green status LED for LAN LED2 pin | Ethernet speed indicator, driven by LAN8720A LED2, current limited by R6 (270Ω) |
| D4 | Schottky Diode | SS14 | Flyback diode for MCU fan protection | Protects MCU from inductive kickback when fan MOSFET switches off (cathode on +VIN, anode on FAN_SW) |
| D5 | Zener Diode | BZX84C12 | Polarity protection MOSFET gate protection | 12V zener diode protects IRF9540N gate from overvoltage (cathode to gate, anode to source) |
| D6-D8 | Schottky Diode | MBR735 | Aux port power protection | Reverse polarity protection for aux port power pins (J5-J7) |
| D9 | Schottky Diode | SS14 | Aux port power protection | Reverse polarity protection for aux port power pin (J8) |

**MOSFETs (Sheet 1 - standard components):**

| Reference | Type | Part Number | Purpose | Notes |
|-----------|------|-------------|---------|-------|
| Q1 | P-channel MOSFET | IRF9540N | Reverse polarity protection | High-side switch at barrel jack input. Source to +24_12V, Drain to buck converter input. Gate pulled to GND via R1 (10kΩ) with zener protection (D5). Prevents damage from inverted power supply connections. |
| Q2 | N-channel MOSFET | IRLZ44N | Fan control switch | Low-side switch for 12/24V fan. Drain to FAN_SW, Source to GND. Gate driven by ESP32-S3 GPIO42 via R10 (100Ω). Gate pull-down via R11 (10kΩ). Flyback diode D4 protects from inductive kickback. |

**Switches (Sheet 1 - standard components):**

| Reference | Type | Part Number | Purpose | Notes |
|-----------|------|-------------|---------|-------|
| SW1 | DIP Switch | SW_DIP_x01 | CAN bus termination control | Single-pole switch to enable/disable 120Ω termination resistor (R12) across CANH/CANL. Required for proper CAN bus operation (only end nodes should have termination). |
| SW2-SW5 | DIP Switch | SW_DIP_x01 | Aux port power switches | Power enable switches for aux ports J5-J8 (J5_PWR_SW, J6_PWR_SW, J7_PWR_SW, J8_PWR_SW) |

### Motor Driver Components (Sheet 2 - Hierarchical Motor Driver Sheet)

**Motor Driver ICs:**
- **BTS7960B** (×2) - Half-bridge motor driver (30A per IC, 60A total in H-bridge configuration)
  - Package: TO-263-7 (DPAK127P1490X440-8N)
  - VS: 24V motor power rail (pins 1, 7, 8 - thermal tab)
  - OUT: Motor output (pin 4/5)
  - IN: PWM control input (pin 2)
  - INH: Enable input (pin 3)
  - IS: Current sense output (pin 6)
  - SR: Slew rate control (pin 5, unused, tied to GND via pull-down)

**Buffer IC:**
- **74HC244** - Octal buffer/line driver (TSSOP-20 package)
  - Purpose: Signal isolation and drive strength between ESP32-S3 and BTS7960
  - Supply: 3.3V (from ESP32-S3 3.3V_OUT)
  - Inputs: ESP32-S3 GPIO pins (MOTOR_A_EN, MOTOR_A_IN, MOTOR_B_EN, MOTOR_B_IN)
  - Outputs: BTS7960 IN and INH pins

**Schottky Diodes:**
- **D10** - MBR3045PT - Dual common-cathode Schottky diode (30A per diode, 15A per anode)
  - Package: TO-220-3
  - Purpose: Flyback protection for motor driver (dissipates inductive kickback)
  - Configuration: Common cathode to motor power rail, anodes to MOTOR_A_OUT and MOTOR_B_OUT
  - Note: Only one MBR3045PT is used (not two separate diodes) as it contains two diodes in a single package

**Motor Driver Resistors:**

| Reference | Value | Purpose | Notes |
|-----------|-------|---------|-------|
| R13-R16 | 10 kΩ | BTS7960 INH pull-down | Pull-down resistors for enable pins (INH) on both BTS7960 ICs |
| R17-R18 | 2.2 kΩ | Current sense voltage divider (upper) | Forms voltage divider with R19-R20 to scale 0-5V current sense to 0-3.3V for ESP32 ADC |
| R19-R20 | 220Ω | Current sense voltage divider (lower) | Forms voltage divider with R17-R18 for ESP32 ADC compatibility |
| R21-R22 | 10 kΩ | Hall sensor pull-up | Pull-up resistors for hall sensor data lines (HALL1, HALL2) |

**Motor Driver Capacitors:**

| Reference | Value | Type | Purpose | Notes |
|-----------|-------|------|---------|-------|
| C19 | 100nF | Non-polarized | BTS7960 decoupling (U9) | High-frequency noise filtering on VS pin of first BTS7960 IC |
| C20 | 47µF | Polarized | Motor power bulk capacitance (U9) | Bulk capacitance for 24V motor power rail smoothing near first BTS7960 IC |
| C21 | 100nF | Non-polarized | BTS7960 decoupling (U10) | High-frequency noise filtering on VS pin of second BTS7960 IC |
| C22 | 47µF | Polarized | Motor power bulk capacitance (U10) | Bulk capacitance for 24V motor power rail smoothing near second BTS7960 IC |

### Connectors (Sheet 1)

**Power:**
- **Schematic Symbol:** Kycon KLDX-0202-A (horizontal mount, 2.0mm center pin) - Used in schematic because footprint is available in KiCad built-in libraries. Pin pattern is identical to vertical mount variant.
- **BOM Part (Manufacturing):** Kycon KLDVX-0202-A (vertical mount, 2.0mm center pin, ~15-20mm shell height) - **Use this for actual PCB assembly.** Vertical mount is required for potting compatibility. Barrel orientation is 90° from mount plane (vertical) vs. parallel (horizontal) for KLDX variant.
- **Note:** The KLDX-0202-A symbol can be used in schematic, but order KLDVX-0202-A for manufacturing. The barrel jack can be ordered separately and excluded from BOM during manufacture if necessary.
- 12V/24V header: Passthrough for motors/solenoids
- 5V header: To MCU VIN (via adapter)

### Optional MCU Cooling Fan

- **Mechanical**:
  - Add a large through-hole window directly under the MCU socket so airflow can pass through the board when a fan is mounted below the module.
  - Flank the 44-pin MCU header with four plated mounting holes (M2/M2.5) laid out on a 40×40 mm square to accept standard standoffs for 40 mm fans. The same pattern works for top- or bottom-mounted fans; leave clearance for the ESP32 heat shield.
- **Electrical**:
  - Add a **JST-XH 2-pin** header near the MCU slot labelled `FAN_PWR` (Pin 1 = +VIN 12/24 V, Pin 2 = FAN_SW).
  - Route Pin 1 to the raw 12 V/24 V rail immediately after the barrel jack (ahead of the LM2576 buck) with the same fuse/polyswitch protection as the rest of the VIN net.
  - Route Pin 2 to the drain of **IRLZ44N** N-channel MOSFET (Q2) so the MCU can switch the low side of the fan; tie the source to GND and add a 10 kΩ gate pull-down (R11) plus 100 Ω gate resistor (R10).
  - Place a flyback diode (SS14, D4) across the fan connector (cathode on +VIN, anode on FAN_SW) to clamp inductive kick when the MOSFET turns off.
  - Provide a 0.1 µF/50 V snubber capacitor near the connector if the harness run is long (>0.5 m).
  - Connect ESP32-S3 `GPIO42` (Pad 26) to the MOSFET gate via the adapter header. This GPIO is reserved for `FAN_CTRL` and supports high-current drive. Other MCUs can remap as needed.
  - Label the net `FAN_CTRL` so firmware can PWM the fan if desired; recommend a 25 kHz PWM to stay out of the audible band.

**Ethernet:**
- **Adam Tech MTJ-883X1** (4× aux ports) - RJ45 jack, vertical mount, 16.38mm shell height
- **Aux Ports:** J5, J6, J7, J8 (schematic references U2, U3, U4, U5)
- **Note:** Pins 7 and 8 on all aux ports have been remapped compared to Universal Shield:
  - Pin 7 (ADC): Now uses GPIO1 (J6) and GPIO7 (J7) instead of GPIO4 and GPIO5
  - Pin 8 (PWM): Now uses GPIO4 (J5) and GPIO5 (J6) instead of GPIO1 and GPIO2
  - This remapping improves routing on the Driver Shield PCB and frees GPIO4/5 for motor driver current sense

**CAN Bus:**
- **JST-XH 3-pin connector** (vibration-resistant alternatives: Phoenix Contact MSTB, Deutsch DT, Molex MX150, or JST EH)
- Pin 1: CANH
- Pin 2: CANL
- Pin 3: CAN_GND (always routed on PCB, even if unused in 2-wire networks)

**MCU Interface:**
- **44-pin 2×22 stacking female header** - ESP32-S3 socket/through-hole footprint

**Motor Actuator Connector (Sheet 2 - Motor Driver Sheet):**
- **J5** - Molex 1724480006 - 6-pin connector for PA-HD1-HALL linear actuator
- Pin 1: +24V (motor power positive)
- Pin 2: 24V GND (motor power ground)
- Pin 3: +5V (hall sensor power)
- Pin 4: 5V GND (hall sensor ground)
- Pin 5: HALL1 (hall sensor data 1, to ESP32-S3 GPIO9 via voltage divider)
- Pin 6: HALL2 (hall sensor data 2, to ESP32-S3 GPIO10 via voltage divider)

**UART Debug Breakout (4-pin header):** ✅ **AVAILABLE**
- **Status:** Implemented and available on Driver Shield
- **Purpose:** Serial debug interface for low-level debugging, firmware programming, and module interfacing
- **Pinout:**
  - Pin 1: GND (Ground)
  - Pin 2: +3.3V (Power supply for debug modules)
  - Pin 3: U0TXD (GPIO43, UART0 TX - serial transmit)
  - Pin 4: U0RXD (GPIO44, UART0 RX - serial receive)

### Enclosure

**Standard Project Box:**
- **Polycase LP-70F** - ABS plastic enclosure designed for the Driver Shield
- **External dimensions:** 5.5 × 4.2 × 1.7 inches (139.7 × 106.7 × 43.2 mm)
- **Internal dimensions:** 5.25 × 4.0 × 1.6 inches (133.4 × 101.6 × 40.6 mm)
- **Features:**
  - Molded-on flanges for surface mounting
  - PCB mounting bosses in the base
  - PCB template available from Polycase for design reference
- **Product page:** [Polycase LP-70F](https://www.polycase.com/lp-70F)
- **Mounting holes:** PCB corner mounting holes are sized for standard case mounting bosses (typically M2/M2.5/M3 threaded inserts)
- **Mounting hole spacing:** 4.13" × 2.88" (mounting boss centers)
- **PCB mounting:** PCB feet/standoffs elevate the board 0.125" (1/8") above the case floor

**Potting Design:**
- **Connector orientation:** All pluggable elements (RJ45 connectors, barrel jack, headers, Molex connector) are oriented vertically (perpendicular to PCB plane) to facilitate potting and prevent compound from entering connector cavities during encapsulation.
- **Potting process:** Potting involves filling the enclosure with a protective compound that encapsulates the PCB and components, providing environmental protection (moisture, dust, vibration), electrical insulation, and thermal management. The compound cures to form a solid, protective barrier around the electronics. For the Driver Shield, potting is recommended for long-term installations in harsh environments (outdoor, industrial, automotive applications).
- **Recommended potting compound:** Two-part polyurethane potting compound (e.g., MG Chemicals 832TC, Smooth-On Smooth-Cast 300, or similar) is recommended. Polyurethane offers good thermal conductivity, flexibility to accommodate thermal expansion, and resistance to moisture and chemicals. Avoid rigid epoxies for this application as they can crack under thermal cycling. Select a compound with a working time (pot life) of 15-30 minutes to allow proper mixing and pouring, and ensure it's rated for the operating temperature range of your installation.
- **Potting compound volume:** To submerge the top of the PCB by at least 1/16" (0.0625"), approximately **4.5-5.0 cubic inches** of potting compound is required. This accounts for the case interior (5.25" × 4.0"), PCB standoff height (0.125"), PCB thickness (~0.062"), and minimum coverage depth (0.0625"), minus the volume displaced by the PCB and components. Order slightly more than calculated (10-20% extra) to account for mixing waste and ensure complete coverage.
- **Connector:** Standard 2.54mm pitch (0.1") header, recommended 4-pin male header for breadboard/prototyping
- **Common Uses:**
  - Serial debug console (USB-to-serial adapters: FTDI, CP2102, CH340, etc.)
  - Serial terminal interfaces (PuTTY, minicom, screen, etc.)
  - Logic analyzers for signal debugging
  - Firmware programming/bootloaders
  - Interfacing with UART-based modules:
    - Bluetooth modules (HC-05, HC-06, ESP32-BLE, etc.)
    - WiFi modules (ESP8266, some ESP32 variants)
    - GPS modules (NEO-6M, NEO-8M, etc.)
    - LoRa modules (SX1278, SX1262)
    - RFID readers (MFRC522, PN532)
    - Barcode scanners
    - Serial LCD displays
    - Motor controllers
    - Industrial sensors
    - Many other embedded modules requiring point-to-point serial communication
- **Note:** UART is one of the most versatile communication protocols for embedded systems, supporting a wide variety of components and debug tools. While the 4 Ethernet ports provide network-level debugging, the UART breakout provides low-level serial debugging and direct module interfacing.

</div>

</details>

<details>
<summary><strong>Pin Mappings</strong></summary>

<div style="margin-left: 20px;">

### ESP32-S3-WROOM-1 Complete 44-Pin Pinout (2×22 Header)

**Pin Numbering:** Left column (pins 1-22) top to bottom, right column (pins 23-44) top to bottom.

| Pin | GPIO | Function | Notes |
|-----|------|----------|-------|
| 1 | - | 3.3V_OUT | 3.3V power output (from onboard regulator) |
| 2 | - | 3.3V_OUT | 3.3V power output (from onboard regulator) |
| 3 | GPIO3 | EN | Enable (active HIGH, reset when LOW) |
| 4 | GPIO4 | GPIO4 / ADC1_CH3 | Motor A Current Sense (MOTOR_A_IS) |
| 5 | GPIO5 | GPIO5 / ADC1_CH4 | Motor B Current Sense (MOTOR_B_IS) |
| 6 | GPIO6 | GPIO6 / ADC1_CH5 | Aux Port 8 ADC |
| 7 | GPIO7 | GPIO7 / ADC1_CH6 | Aux Port 7 ADC |
| 8 | GPIO15 | GPIO15 / LEDC_CH3 | Aux Port 8 PWM |
| 9 | GPIO16 | GPIO16 / LEDC_CH4 | Aux Port 5 PWM |
| 10 | GPIO17 | GPIO17 | LAN8720A MDC (Ethernet management clock) |
| 11 | GPIO18 | GPIO18 | LAN8720A MDIO (Ethernet management data) |
| 12 | GPIO8 | GPIO8 / ADC1_CH7 | Aux Port 5 ADC |
| 13 | GPIO3 | GPIO3 | General purpose I/O (also EN function on pin 3) |
| 14 | GPIO46 | GPIO46 | General purpose I/O (E-Stop interrupt capable) |
| 15 | GPIO9 | GPIO9 / ADC1_CH8 | Hall Sensor 1 (HALL1) |
| 16 | GPIO10 | GPIO10 / ADC1_CH9 | Hall Sensor 2 (HALL2) |
| 17 | GPIO11 | GPIO11 / LEDC_CH4 | Motor A PWM (MOTOR_A_IN) |
| 18 | GPIO12 | GPIO12 / LEDC_CH0 | Motor A Enable (MOTOR_A_EN) |
| 19 | GPIO13 | GPIO13 / LEDC_CH1 | Motor B PWM (MOTOR_B_IN) |
| 20 | GPIO14 | GPIO14 / LEDC_CH2 | Motor B Enable (MOTOR_B_EN) |
| 21 | - | VIN | 5V power input (to onboard regulator) |
| 22 | - | GND | Ground |
| 23 | - | GND | Ground |
| 24 | GPIO43 | U0TXD | UART0 TX (GPIO43) |
| 25 | - | NC | No Connect |
| 26 | GPIO1 | GPIO1 / ADC1_CH0 | Aux Port 6 ADC (remapped from GPIO4) |
| 27 | GPIO2 | GPIO2 / LEDC_CH6 | Aux Port 7 PWM (remapped from GPIO5) |
| 28 | GPIO42 | GPIO42 / Pad 26 | General purpose I/O (reserved for FAN_CTRL per README) |
| 29 | GPIO41 | GPIO41 / Pad 29 | CAN RXD (ADM3057E) |
| 30 | GPIO40 | GPIO40 / Pad 30 | CAN TXD (ADM3057E) |
| 31 | GPIO39 | GPIO39 | LAN8720A TXD0 (Ethernet transmit data 0) |
| 32 | GPIO38 | GPIO38 | General purpose I/O |
| 33 | GPIO37 | GPIO37 | General purpose I/O |
| 34 | GPIO36 | GPIO36 | General purpose I/O |
| 35 | GPIO35 | GPIO35 | General purpose I/O |
| 36 | GPIO0 | GPIO0 / ADC1_CH0 | Boot strap pin (unused, kept free to avoid boot mode conflicts) |
| 37 | GPIO45 | GPIO45 | General purpose I/O (E-Stop interrupt capable) |
| 38 | GPIO46 | GPIO46 | General purpose I/O (E-Stop interrupt capable) |
| 39 | GPIO47 | GPIO47 | General purpose I/O (E-Stop interrupt capable) |
| 40 | GPIO21 | GPIO21 / LEDC_CH7 | Aux Port 6 PWM |
| 41 | GPIO20 | GPIO20 | General purpose I/O |
| 42 | GPIO44 | U0RXD | UART0 RX (GPIO44) |
| 43 | - | GND | Ground |
| 44 | - | GND | Ground |

**Note:** This pinout is based on standard ESP32-S3-WROOM-1 dev board layouts. Verify against your specific board's datasheet. GPIO assignments for Aux Ports, Motor Driver, Ethernet, and CAN match the mappings documented below.

### ESP32-S3 Electrical Type Map (KiCad Pin Properties)

**Use this map to set the "Electrical type" property for each pin in your ESP32-S3 symbol:**

| Pin | GPIO | Electrical Type | Reason |
|-----|------|----------------|--------|
| 1 | - | Power output | 3.3V_OUT (power source) |
| 2 | - | Power output | 3.3V_OUT (power source) |
| 3 | GPIO3 | Input | EN (enable/reset input) |
| 4 | GPIO4 | Input | ADC pin (analog input) - Motor A Current Sense |
| 5 | GPIO5 | Input | ADC pin (analog input) - Motor B Current Sense |
| 6 | GPIO6 | Input | ADC pin (analog input) - Aux Port 8 ADC |
| 7 | GPIO7 | Input | ADC pin (analog input) - Aux Port 7 ADC |
| 8 | GPIO15 | Output | PWM output (LEDC_CH3) - Aux Port 8 PWM |
| 9 | GPIO16 | Output | PWM output (LEDC_CH4) - Aux Port 5 PWM |
| 10 | GPIO17 | Bidirectional | MDC (I2C-like, bidirectional) |
| 11 | GPIO18 | Bidirectional | MDIO (I2C-like, bidirectional) |
| 12 | GPIO8 | Input | ADC pin (analog input) - Aux Port 5 ADC |
| 13 | GPIO3 | Bidirectional | General purpose I/O (also EN function) |
| 14 | GPIO46 | Bidirectional | General purpose I/O (E-Stop interrupt capable) |
| 15 | GPIO9 | Input | ADC pin (analog input) - Hall Sensor 1 |
| 16 | GPIO10 | Input | ADC pin (analog input) - Hall Sensor 2 |
| 17 | GPIO11 | Output | PWM output (LEDC_CH4) - Motor A PWM |
| 18 | GPIO12 | Output | Digital output - Motor A Enable |
| 19 | GPIO13 | Output | PWM output (LEDC_CH1) - Motor B PWM |
| 20 | GPIO14 | Output | Digital output - Motor B Enable |
| 21 | - | Power input | VIN (5V power input) |
| 22 | - | Power input | GND (ground) |
| 23 | - | Power input | GND (ground) |
| 24 | GPIO43 | Output | UART0 TX (serial transmit) |
| 25 | - | Unspecified | NC (No Connect) |
| 26 | GPIO1 | Input | ADC pin (analog input) - Aux Port 6 ADC |
| 27 | GPIO2 | Output | PWM output (LEDC_CH6) - Aux Port 7 PWM |
| 28 | GPIO42 | Bidirectional | General purpose I/O (reserved for FAN_CTRL) |
| 29 | GPIO41 | Bidirectional | CAN RXD (bidirectional CAN receive) |
| 30 | GPIO40 | Bidirectional | CAN TXD (bidirectional CAN transmit) |
| 31 | GPIO39 | Output | LAN8720A TXD0 (Ethernet transmit data 0) |
| 32 | GPIO38 | Bidirectional | General purpose I/O |
| 33 | GPIO37 | Bidirectional | General purpose I/O |
| 34 | GPIO36 | Bidirectional | General purpose I/O |
| 35 | GPIO35 | Bidirectional | General purpose I/O |
| 36 | GPIO0 | Input | General purpose I/O (boot strap pin, unused) |
| 37 | GPIO45 | Bidirectional | General purpose I/O (E-Stop interrupt capable) |
| 38 | GPIO46 | Bidirectional | General purpose I/O (E-Stop interrupt capable) |
| 39 | GPIO47 | Bidirectional | General purpose I/O (E-Stop interrupt capable) |
| 40 | GPIO21 | Output | PWM output (LEDC_CH7) - Aux Port 6 PWM |
| 41 | GPIO20 | Bidirectional | General purpose I/O |
| 42 | GPIO44 | Input | UART0 RX (serial receive) |
| 43 | - | Power input | GND (ground) |
| 44 | - | Power input | GND (ground) |

**Summary:**
- **Power output**: Pins 1, 2 (3.3V_OUT)
- **Power input**: Pin 21 (VIN), Pins 22, 23, 43, 44 (GND)
- **Input**: Pins 3 (EN), 4-7, 12, 15-16, 26 (ADC pins), 42 (UART0 RX)
- **Output**: Pins 8-9, 17-20, 24 (UART0 TX), 27, 31, 40 (PWM/digital outputs)
- **Bidirectional**: Pins 10-11 (MDC/MDIO), 13-14, 28-30 (CAN), 32-35, 37-39 (general GPIOs)
- **Unspecified**: Pin 25 (NC)
- **Unused**: Pin 36 (GPIO0 - boot strap pin, kept free to avoid boot mode conflicts)

### ESP32-S3 Aux Port ADC/PWM (J5-J8 only)

| Aux Port | ADC GPIO | ADC Pin | ADC Channel | PWM GPIO | PWM Pin | PWM LEDC Channel |
|----------|----------|---------|-------------|----------|---------|------------------|
| J5 | GPIO8 | Pin 12 | ADC1_CH7 | GPIO16 | Pin 9 | LEDC_CH4 |
| J6 | GPIO1 | Pin 26 | ADC1_CH0 | GPIO21 | Pin 40 | LEDC_CH7 |
| J7 | GPIO7 | Pin 7 | ADC1_CH6 | GPIO2 | Pin 27 | LEDC_CH6 |
| J8 | GPIO6 | Pin 6 | ADC1_CH5 | GPIO15 | Pin 8 | LEDC_CH3 |

**Note:** Pins 7 and 8 on all aux ports have been remapped compared to Universal Shield:
- **Pin 7 (ADC):** Now uses GPIO1 (J6) and GPIO7 (J7) instead of GPIO4 and GPIO5
- **Pin 8 (PWM):** Now uses GPIO4 (J5) and GPIO5 (J6) instead of GPIO1 and GPIO2
- This remapping improves PCB routing and frees GPIO4/5 for motor driver current sense

**KiCAD Net Labels:**
- ADC: `AUX5_ADC`, `AUX6_ADC`, `AUX7_ADC`, `AUX8_ADC`
- PWM: `AUX5_PWM`, `AUX6_PWM`, `AUX7_PWM`, `AUX8_PWM`

### Motor Driver Pin Mappings

**BTS7960 H-Bridge Control:**

| Signal | ESP32-S3 GPIO | ESP32-S3 Pin | Function | Notes |
|--------|---------------|--------------|----------|-------|
| MOTOR_A_EN | GPIO12 | Pin 18 | Motor A Enable | Digital output, via 74HC244 buffer |
| MOTOR_A_IN | GPIO11 | Pin 17 | Motor A PWM | PWM output (LEDC_CH4), via 74HC244 buffer |
| MOTOR_A_IS | GPIO4 | Pin 4 | Motor A Current Sense | ADC input (ADC1_CH3), via voltage divider (2.2kΩ/220Ω) |
| MOTOR_B_EN | GPIO14 | Pin 20 | Motor B Enable | Digital output, via 74HC244 buffer |
| MOTOR_B_IN | GPIO13 | Pin 19 | Motor B PWM | PWM output (LEDC_CH1), via 74HC244 buffer |
| MOTOR_B_IS | GPIO5 | Pin 5 | Motor B Current Sense | ADC input (ADC1_CH4), via voltage divider (2.2kΩ/220Ω) |

**Hall Sensor Feedback:**

| Signal | ESP32-S3 GPIO | ESP32-S3 Pin | Function | Notes |
|--------|---------------|--------------|----------|-------|
| HALL1 | GPIO9 | Pin 15 | Hall Sensor 1 | ADC input (ADC1_CH8), via voltage divider (10kΩ pull-up, 2.2kΩ/220Ω divider) |
| HALL2 | GPIO10 | Pin 16 | Hall Sensor 2 | ADC input (ADC1_CH9), via voltage divider (10kΩ pull-up, 2.2kΩ/220Ω divider) |

**Molex 6-Pin Actuator Connector (J5):**

| Pin | Signal | Connection | Function | Notes |
|-----|--------|------------|----------|-------|
| 1 | MOTOR_PWR | +24V rail | Motor power positive | 24V motor power supply (direct from barrel jack, bypasses buck converter) |
| 2 | MOTOR_GND | GND | Motor power ground | Motor power return path |
| 3 | HALL_PWR | +5V | Hall sensor power | 5V supply for hall sensor electronics (from buck converter 5V output) |
| 4 | HALL_GND | GND | Hall sensor ground | Hall sensor power return path |
| 5 | HALL1 | ESP32-S3 GPIO9 | Hall Sensor 1 data | ADC input (ADC1_CH8), via voltage divider (10kΩ pull-up, 2.2kΩ/220Ω divider) |
| 6 | HALL2 | ESP32-S3 GPIO10 | Hall Sensor 2 data | ADC input (ADC1_CH9), via voltage divider (10kΩ pull-up, 2.2kΩ/220Ω divider) |

**Note:** Pin mapping is based on the PA-HD1-HALL linear actuator datasheet. The connector (Molex 1724480006) is a 6-pin connector that matches the actuator's wiring harness. Motor power (pins 1-2) is controlled via the BTS7960 H-bridge outputs (MOTOR_A_OUT, MOTOR_B_OUT), while hall sensor feedback (pins 5-6) provides position feedback to the ESP32-S3 ADC inputs.

**74HC244 Buffer IC Pin Mapping:**

| 74HC244 Pin | Signal | Connection | Notes |
|-------------|--------|------------|-------|
| 1A0 | MOTOR_A_EN | ESP32-S3 GPIO12 | Enable signal for Motor A |
| 1Y0 | BTS_A_EN | BTS7960 INH (Motor A) | Buffered enable output |
| 1A1 | MOTOR_A_IN | ESP32-S3 GPIO11 | PWM signal for Motor A |
| 1Y1 | BTS_A_IN | BTS7960 IN (Motor A) | Buffered PWM output |
| 1A2 | MOTOR_B_EN | ESP32-S3 GPIO14 | Enable signal for Motor B |
| 1Y2 | BTS_B_EN | BTS7960 INH (Motor B) | Buffered enable output |
| 1A3 | MOTOR_B_IN | ESP32-S3 GPIO13 | PWM signal for Motor B |
| 1Y3 | BTS_B_IN | BTS7960 IN (Motor B) | Buffered PWM output |
| VCC | +3.3V | ESP32-S3 3.3V_OUT | Buffer IC power supply |
| GND | GND | Common ground | Buffer IC ground |

**Note:** The 74HC244 provides signal isolation and increased drive strength between the ESP32-S3 and BTS7960 ICs, ensuring reliable control signal transmission.

### LAN8720A Ethernet PHY

**MII/RMII Interface (Ethernet Data):**

| LAN8720A Pin | ESP32-S3 GPIO | ESP32-S3 Pin | Function | Notes |
|--------------|---------------|--------------|----------|-------|
| TXD0 | GPIO39 | Pin 31 | Transmit Data 0 | RMII transmit data bit 0 |
| TXD1 | GPIO37 | Pin 33 | Transmit Data 1 | RMII transmit data bit 1 |
| TXEN | GPIO20 | Pin 41 | Transmit Enable | RMII transmit enable |
| RXD0 | GPIO35 | Pin 35 | Receive Data 0 | RMII receive data bit 0 |
| RXD1 | GPIO36 | Pin 34 | Receive Data 1 | RMII receive data bit 1 |
| RXER | - | - | Receive Error | No Connect (NC) |
| CRS_DV | GPIO38 | Pin 32 | Carrier Sense / Data Valid | RMII carrier sense and data valid |
| REF_CLK | - | - | Reference Clock | No Connect (NC) - LAN8720A uses external 25MHz crystal |

**Management Interface (SMI):**

| LAN8720A Pin | ESP32-S3 GPIO | ESP32-S3 Pin | Function | Notes |
|--------------|---------------|--------------|----------|-------|
| MDC | GPIO17 | Pin 10 | Management Data Clock | SMI clock signal |
| MDIO | GPIO18 | Pin 11 | Management Data I/O | SMI bidirectional data |

**Control Signals:**

| LAN8720A Pin | ESP32-S3 GPIO | ESP32-S3 Pin | Function | Notes |
|--------------|---------------|--------------|----------|-------|
| RESET_N | EN (GPIO3) | Pin 3 | Reset (active LOW) | Reset control, active LOW |
| INT | - | - | Interrupt | No Connect (NC) |

**Clock:**

| LAN8720A Pin | Connection | Notes |
|--------------|------------|-------|
| XTAL1 | External 25MHz crystal (3225 SMD recommended) | Crystal connection |
| XTAL2 | External 25MHz crystal (3225 SMD recommended) | Crystal connection |
| - | 10pF decoupling capacitors | One capacitor from XTAL1 to GND, one from XTAL2 to GND |
| Exposed pad (VSS) | Ground pour + thermal vias | Stitch the VQFN thermal pad to the 3.3 V ground plane during PCB layout for heat spreading and signal reference |

**Ethernet Differential Signals (to RJ45 Connectors):**

| LAN8720A Pin | Destination | Notes |
|--------------|-------------|-------|
| TXP/TXN | All 4 aux ports pins 1-2 | Transmit differential pair |
| RXP/RXN | All 4 aux ports pins 3, 6 | Receive differential pair |

### ADM3057E CAN Transceiver

| ADM3057E Pin | ESP32-S3 Connection | Notes |
|--------------|---------------------|-------|
| TXD | GPIO40 (Pad 30) | CAN transmit |
| RXD | GPIO41 (Pad 29) | CAN receive |
| STBY | +3.3V (tie high) | Normal operation |
| SILENT | GND (tie low) | Normal operation |
| RS | GND (tie low) | Normal mode |
| VIO | +3.3V | Logic supply |
| VCC | +5V | isoPower supply |
| VISOOUT | Jump to VISOIN | Required |
| VISOIN | Jump to VISOOUT | Required |
| CANH | CAN connector Pin 1 | Bus high |
| CANL | CAN connector Pin 2 | Bus low |
| GNDISO | CAN connector Pin 3 | Isolated ground |

**Note:** GPIO4/5 are used for motor driver current sense, so CAN uses GPIO40/41 (same as Universal Shield).

</div>

</details>

<details>
<summary><strong>Aux Port Pinout (CAT5 T568B)</strong></summary>

<div style="margin-left: 20px;">

| RJ45 Pin | T568B Color | Signal | Connection |
|----------|-------------|--------|------------|
| 1 | Orange/White | Ethernet TX+ | LAN8720A TXP |
| 2 | Orange | Ethernet TX- | LAN8720A TXN |
| 3 | Green/White | Ethernet RX+ | LAN8720A RXP |
| 4 | Blue | **+5V Power** | Buck converter 5V (direct, ≥5.0A total) |
| 5 | Blue/White | **GND** | Common ground |
| 6 | Green | Ethernet RX- | LAN8720A RXN |
| 7 | Brown/White | **ADC** | MCU ADC pin via adapter (parent ECU only) |
| 8 | Brown | **PWM** | MCU PWM pin via adapter (parent ECU only) |

**Cable Requirements:**
- Standard straight-through Cat5e/Cat6 (T568B both ends)
- **All 8 wires required** - Do not use 4-wire cables (some budget/flat Ethernet cables only include wires for pins 1-2, 3-6)
- Pins 4-5 are required for power (5V and GND) when connecting powered child devices
- Pins 7-8 are required for ADC/PWM functionality
- 24-26 AWG, UTP or STP
- Tested up to 50m (100m for light loads)
- **Note:** While 100 Mbps Ethernet only requires 4 wires (pins 1-3, 6), Driver Shield aux ports use all 8 wires for power and I/O functionality

**Usage:**
- **Child ECUs (Pi/Jetson):** Connect via Ethernet (pins 1-3, 6). Use device's native ADC/PWM. Communication via UDP binary protocol.
- **Parent ECU I/O:** Access ADC/PWM on pins 7-8 for direct hardware control (ESP32/STM32/Arduino native).

**Pin Remapping Note:**
- Pins 7 and 8 on all aux ports (J5-J8) have been remapped compared to Universal Shield:
  - **Pin 7 (ADC):** J5 uses GPIO8, J6 uses GPIO1, J7 uses GPIO7, J8 uses GPIO6
  - **Pin 8 (PWM):** J5 uses GPIO16, J6 uses GPIO21, J7 uses GPIO2, J8 uses GPIO15
  - This remapping improves PCB routing and frees GPIO4/5 for motor driver current sense

<details>
<summary><strong>Power Safety</strong></summary>

<div style="margin-left: 20px;">

**Overcurrent Protection:**
- ⚠️ **PENDING:** Driver Shield requires a **4A fuse** on the aux power bus (5V rail to all 4 aux ports) - See Next Steps (v2.0 Features)
- Fuse will protect against excessive current draw from misbehaving devices or short circuits
- Fuse rating should match maximum design capacity (4A total across 4 ports)

**Per-Port Power Control:** ✅ **AVAILABLE**
- **DIP Switches:** One DIP switch per aux port (4 total) allows manual enable/disable of 5V power output on each port
- **Backfeed Protection:** Schottky diodes (MBR745) on each aux port's 5V output prevent reverse current flow
- **Power Path:** Buck converter → DIP switch → Schottky diode → Connector pin 4
- **Benefits:**
  - Prevents parallel power supply issues when connecting Driver Shield-to-Universal Shield (turn off 5V on child shield)
  - Power management: Only power child devices that need it
  - Troubleshooting: Isolate power issues per port
  - Flexible installations: Mix powered and unpowered child devices
- **Switch Labeling:** Each switch labeled (e.g., "J5 5V", "J6 5V", etc.)

**Power over Ethernet (PoE) Compatibility:**
- **Standard PoE devices (IEEE 802.3af/at/bt) are NOT compatible** with aux port power
- Standard PoE requires 48V and handshake protocol (not provided by Driver Shield)
- **Standard Raspberry Pi and Jetson Nano (without PoE HATs) are safe** - pins 4&5 are not connected internally, so 5V power is safely ignored
- **PoE HATs/add-on boards are NOT supported** - PoE controllers expect 48V and may be damaged by 5V
- **Do not use PoE-enabled devices or PoE HATs on aux ports**

**Power Safety Considerations:**
- Maximum current per port: 1.0A (4A total ÷ 4 ports)
- Devices drawing excessive current will trip the 4A fuse
- Always verify device power requirements before connecting to aux ports
- For detailed power safety information, see [E-Stop & Safety Considerations](../../ESTOP_&_Safety_Considerations.md#pcb-power-trace-design-universal-shield)

</div>

</details>

</div>

</details>

<details>
<summary><strong>Bill of Materials</strong></summary>

<div style="margin-left: 20px;">

### Driver Shield (≈ $28-32 @ 1k pcs)

| Qty | Component | Part Number | Purpose | Unit Cost |
|-----|-----------|-------------|---------|-----------|
| 1 | Ethernet PHY | LAN8720A | 100 Mbps Ethernet | $1.50 |
| 4 | Ethernet connector (aux) | Adam Tech MTJ-883X1 | Aux ports J5-J8 (any can serve as router interface) | $0.53 × 4 |
| 1 | DC barrel jack | Kycon KLDVX-0202-A | 12-24V input, vertical mount (2.0mm center pin) | $0.60 |
| **Note:** Schematic uses KLDX-0202-A symbol (horizontal mount, available in KiCad libraries), but BOM lists KLDVX-0202-A (vertical mount) for actual manufacturing. Pin patterns are identical; only barrel orientation differs. Can be ordered separately and excluded from BOM during manufacture if needed. |
| 1 | Buck converter | LM2576HVS-5 | 12V/24V → 5V (60V max) | $3.00 |
| - | 3.3V LDO | N/A (ESP32-S3 provides 3.3V via pins 1-2) | Shield logic power from MCU | $0.00 |
| 1 | CAN transceiver | ADM3057E | Isolated CAN-FD | $5.50 |
| 1 | MCU header | 44-pin 2×22 stacking | ESP32-S3 interface | $1.20 |
| 2 | Motor driver IC | BTS7960B | Half-bridge motor driver (30A per IC) | $2.50 × 2 |
| 1 | Buffer IC | 74HC244 | Octal buffer/line driver (TSSOP-20) | $0.50 |
| 2 | Schottky diode | MBR3045PT | Flyback protection (30A dual common-cathode) | $1.20 × 2 |
| 1 | Actuator connector | Molex 1724480006 | 6-pin connector for PA-HD1-HALL actuator | $0.80 |
| - | Misc (caps, resistors, LEDs, protection) | - | Standard components + motor driver components | ~$4.00 |
| 1 (opt) | 40 mm fan | Sunon HA40101V4-1000U-A99 | Optional MCU cooling fan (12 V PWM) | $3.50 |

### Personality Adapters (Parent ECU Only)

| Platform | Adapter | Extra Components | Cost Adder | Total Cost |
|----------|---------|------------------|------------|------------|
| ESP32-S3 | Direct | None (native ADC/PWM) | $0 | ~$1 |
| STM32 | Direct | None (native ADC/PWM) | $0 | ~$1-2 |
| Arduino | 5V→3.3V | 6× TXS0108E (level shifters) | ~$7 | ~$8-9 |

**Note:** Raspberry Pi and Jetson Nano are **child ECUs** (connect via Ethernet aux ports), not adapters.

</div>

</details>

<details>
<summary><strong>Platform Support</strong></summary>

<div style="margin-left: 20px;">

**Parent ECU (via adapters):**
- ESP32-S3 (socket/through-hole) - Native ADC/PWM
- ESP32-WROOM-32 - Native ADC/PWM
- STM32 (Black Pill, Nucleo, etc.) - Native ADC/PWM
- Arduino Uno/Mega - Requires level shifters (TXS0108E)

**Child ECUs (via Ethernet aux ports):**
- Raspberry Pi 4/5 - Native GPIO/ADC/PWM, UDP communication
- Jetson Nano/Xavier NX - Native GPIO/ADC/PWM, UDP communication
- Any Ethernet-capable device - Device-dependent I/O, UDP communication

</div>

</details>

<details>
<summary><strong>Design Specifications</strong></summary>

<div style="margin-left: 20px;">

- **Aux Ports:** 4× CAT5 RJ45 (Ethernet + 5V power + 1× ADC + 1× PWM per port)
- **Motor Driver:** BTS7960 H-bridge (30A per half-bridge, 60A total)
- **Motor Power:** 24V direct passthrough from barrel jack (bypasses buck converter)
- **Hall Sensor Support:** 2× ADC channels for position feedback
- **Ethernet:** 100 Mbps full-duplex (LAN8720A)
- **Power Input:** 12V/24V (up to 29.2V for fully charged 24V LiFePO4)
- **Aux Port Power:** 5V directly from buck converter (≥5.0A total across 4 ports)
- **Shield Logic Power:** 3.3V from ESP32-S3 3.3V_OUT (≥2A capacity)
- **CAN Bus:** Isolated CAN-FD (ADM3057E, 3kV isolation)
- **Connectors:** All vertical mount with tall shells (16.38mm) for potting
- **Schematic Organization:** Hierarchical design with motor driver on separate sheet

</div>

</details>

<details>
<summary><strong>External Systems</strong></summary>

<div style="margin-left: 20px;">

### E-Stop Systems

**Note:** E-Stop (Emergency Stop) systems are implemented externally, upstream from the shield board. The E-Stop contactor/relay must be located as close to the battery/power supply as possible, immediately after the main circuit breaker, before the barrel jack. This ensures all power (high-current loads + shield board) is cut at the source when E-Stop is activated.

For complete E-Stop system design, sizing, compliance, and testing guidance, see: **[E-Stop & Safety Considerations](../ESTOP_&_Safety_Considerations.md)**

</div>

</details>

<details>
<summary><strong>KiCAD Workflow Notes</strong></summary>

<div style="margin-left: 20px;">

### Hierarchical Schematic Design

**Motor Driver Sheet:**
- The Driver Shield uses a hierarchical schematic design with motor driver components on a separate sheet (`motor-driver.kicad_sch`)
- The motor driver sheet is connected to the main sheet via hierarchical pins/labels
- All motor driver components (BTS7960, 74HC244, MBR3045PT, resistors, capacitors, Molex connector) are located on Sheet 2
- Power rails (24V, 5V, 3.3V, GND) are passed through hierarchical labels

### Component Re-indexing

**Note:** All components have been re-indexed compared to Universal Shield. Component references may differ from Universal Shield documentation. Always refer to the schematic for current reference assignments.

**Current Component Reference Assignments (Sheet 1):**
- **U1** - LM2576HVS-5 (Buck converter)
- **U2-U5** - MTJ-883X1 (Aux ports J5-J8, references U2, U3, U4, U5 in schematic)
- **U6** - ESP32-S3-2x22 (MCU interface, 44-pin header)
- **U7** - ADM3057ExRW (CAN transceiver)
- **U8** - LAN8720A (Ethernet PHY)

**Motor Driver Component References (Sheet 2):**
- **U9-U10** - BTS7960B (Motor driver ICs)
- **U11** - 74HC244 (Buffer IC)
- **D10** - MBR3045PT (Schottky diode - dual common-cathode package)
- **J5** - Molex 1724480006 (Actuator connector, 6-pin)
- **R13-R22** - Motor driver resistors (pull-downs, voltage dividers, pull-ups)
- **C19-C22** - Motor driver capacitors (decoupling and bulk capacitance)
- Note: All component references have been re-indexed compared to Universal Shield

### ESP32 Module Placement for Net Assignment

**Important:** If a component in the schematic is planned to be replaced with headers/vias on the PCB (like the ESP32-S3 module), it **must still be placed on the PCB first** before deletion to ensure correct net assignments.

**Problem:**
- Components marked as `(power)` symbols or with `#` prefix in their Reference (e.g., `"#U02"`) are treated as "virtual" components
- Virtual components are **not placed on the PCB** when using "Update PCB from Schematic"
- This causes all nets connected to that component's pins to be incorrectly assigned or missing

**Solution Workflow:**
1. **Remove virtual component markers** from the schematic:
   - Remove `(power)` attribute from the symbol definition in the schematic
   - Remove `#` prefix from the Reference property (e.g., change `"#U02"` to `"U02"`)
2. **Update PCB from Schematic** (Tools → Update PCB from Schematic, or F8)
   - The component will now appear in the PCB with correct net assignments
3. **Delete the component from the PCB** after nets are assigned
   - Replace with headers, vias, or other footprints as needed
   - The net connections will remain correct

**Why This Matters:**
- KiCAD uses the component's footprint and pin assignments to create the netlist
- Without the component on the PCB, nets connected to its pins cannot be properly assigned
- This causes cascading net assignment errors throughout the board

</div>

</details>

<details>
<summary><strong>Research Folder</strong></summary>

<div style="margin-left: 20px;">

PCB R&D, analysis documents, and troubleshooting guides are in the `Research/` subfolder:
- Power architecture analysis and options
- Component finding/creation guides
- KiCAD setup and troubleshooting
- CAN bus wiring and connector research
- Connector selection guides
- Motor driver component research (BTS7960, MBR3045PT, Molex connectors)

</div>

</details>
