# CAN Bus Research & Design Guide

## Overview

The Universal Shield provides a **3-pin CAN connector (J20)** supporting both **2-wire** and **3-wire** CAN bus configurations. This flexibility allows connection to devices using either wiring scheme.

## CAN Bus Configurations

### 2-Wire CAN (Most Common)

**Configuration:**
- **CANH** (Pin 1) - Twisted pair high
- **CANL** (Pin 2) - Twisted pair low
- **CAN_GND** (Pin 3) - Leave unconnected or tie to chassis at ONE point only

**When to Use:**
- Standard CAN networks, most automotive applications
- Short to medium cable runs (< 10 meters)
- Systems with good chassis grounding

### 3-Wire CAN (Grounded)

**Configuration:**
- **CANH** (Pin 1) - Twisted pair high
- **CANL** (Pin 2) - Twisted pair low
- **CAN_GND** (Pin 3) - Dedicated ground wire (isolated from ADM3057E)

**When to Use:**
- Long cable runs (> 10 meters)
- Systems with multiple power sources
- Environments with significant ground potential differences
- High-noise industrial environments

### Mixed Networks

**Supporting Both Types:**
- CANH and CANL: Connect to ALL devices (required for all CAN devices)
- CAN_GND: Connect wire to 3-wire devices only, leave unconnected for 2-wire devices
- Ground CAN_GND at ONE point only (shield/battery negative)

## Connector Selection

### Pin Count Requirements

**CAN bus only requires 3 pins:**
1. CANH (Pin 1)
2. CANL (Pin 2)
3. CAN_GND (Pin 3)

**Recommended:** 3-pin connector (exact fit)
**Acceptable:** 4-pin or 6-pin connector (extra pins unused, mark as NC)

### Vibration-Resistant Connectors

**Standard screw terminals are NOT recommended for high-vibration applications** (motion platforms, lifts, actuators). Use:

**1. Spring Clamp Terminals (Recommended):**
- **Phoenix Contact MSTB 2.5/3-ST** - Spring clamp (cage-clamp), tool-free installation
- Constant pressure on wire, vibration-resistant
- Cost: ~$2-4 per connector

**2. Crimp-Style Automotive Connectors (Best for Severe Vibration):**
- **Deutsch DT Series** (DTM04-3P) - IP67 rated, automotive-grade, ~$5-10
- **TE Connectivity AMP Superseal** - IP67 rated, ~$3-7
- **Molex MX150-3** - 3-pin variant (exact fit), automotive-grade, ~$2-5
- **JST EH** - Wire-to-board, 2.5mm pitch, available in KiCAD, ~$0.50-2

**3. Standard Screw Terminals:**
- Use only for low-vibration applications (stationary, prototype)

**Molex MX150 Note:** Available in 2, 3, 4, 6, 9-pin variants. For CAN, use 3-pin (recommended) or 4/6-pin (extra pins unused).

## PCB Routing Requirements

### Always Route All Three Signals

**The PCB must always route all three CAN signals to the connector:**

| Signal | Source | Destination | Always Route? |
|--------|--------|-------------|---------------|
| CANH | ADM3057E CANH | J20 Pin 1 | ✅ Yes |
| CANL | ADM3057E CANL | J20 Pin 2 | ✅ Yes |
| CAN_GND | ADM3057E GNDISO | J20 Pin 3 | ✅ **Yes - Always route** |

**Why Route CAN_GND Even for 2-Wire Networks?**
- Flexibility: Same PCB supports both 2-wire and 3-wire networks
- No PCB variants needed
- Future-proofing: Upgrade from 2-wire to 3-wire without PCB changes
- Mixed networks: Support both device types on same bus

**PCB vs Harness:**
- **PCB Routing (Always):** Route CAN_GND trace from ADM3057E GNDISO to connector Pin 3
- **Harness Wiring (Depends on Network):**
  - 2-Wire: No wire attached to Pin 3 (pin exists but unused)
  - 3-Wire: Wire attached to Pin 3 (connects to CAN_GND)

**Example - Forklift Integration (2-Wire Network):**
```
PCB Design:
ADM3057E GNDISO ──[PCB Trace]──→ J20 Pin 3  ← Trace exists

Harness Wiring:
J20 Pin 3 ──[NO WIRE] ← Pin exists, but no wire attached
```

## Wiring Practices

### Grounding Rules (Critical!)

**Single Point Grounding:**
- Ground CAN_GND at **ONE location only** (typically at parent ECU/shield)
- **Never** ground CAN_GND at multiple points (creates ground loops)
- Ground loops cause noise, communication errors, and can damage transceivers

**For 2-Wire CAN:**
- Devices use chassis/common ground
- Shield's CAN_GND pin can be left unconnected OR tied to battery negative at shield only

**For 3-Wire CAN:**
- All devices connect CAN_GND to common wire
- Ground the common CAN_GND wire at ONE point (shield/battery negative)

### Cable Selection

**2-Wire CAN:**
- Twisted pair cable (CANH, CANL)
- 120Ω characteristic impedance
- Shielded cable recommended for high-noise environments
- Shield grounded at ONE end only (typically at ECU)

**3-Wire CAN:**
- 3-wire twisted pair cable (CANH, CANL, CAN_GND)
- 120Ω characteristic impedance
- Shielded cable recommended
- Shield grounded at ONE end only

**Recommended Cables:**
- Belden 3082A (2-wire, shielded)
- Belden 3105A (3-wire, shielded)
- Or equivalent CAN-rated twisted pair cables

### Termination

**Both Configurations:**
- 120Ω termination resistors at **both ends** of the CAN bus
- One at the parent ECU (shield), one at the last device
- Termination can be built into connector or external

**Stub Length:**
- Keep stub lengths (connections from main bus to nodes) as short as possible
- **Recommended:** < 0.3 meters at 1 Mbps
- Longer stubs cause signal reflections and communication errors

## Practical Examples

### Example 1: Pure 2-Wire CAN Network

```
Shield J20:
  Pin 1 (CANH) ────┬─── CANH ──── Device 1
                   ├─── CANH ──── Device 2
                   └─── CANH ──── Device 3
  
  Pin 2 (CANL) ────┬─── CANL ──── Device 1
                   ├─── CANL ──── Device 2
                   └─── CANL ──── Device 3
  
  Pin 3 (CAN_GND) ──── (Leave unconnected OR tie to battery negative at shield only)
```

**Grounding:** Each device's CAN transceiver ground connects to local ground/chassis. All devices share common ground through power supply/chassis.

### Example 2: Pure 3-Wire CAN Network

```
Shield J20:
  Pin 1 (CANH) ────┬─── CANH ──── Device 1
                   ├─── CANH ──── Device 2
                   └─── CANH ──── Device 3
  
  Pin 2 (CANL) ────┬─── CANL ──── Device 1
                   ├─── CANL ──── Device 2
                   └─── CANL ──── Device 3
  
  Pin 3 (CAN_GND) ────┬─── CAN_GND ──── Device 1
                      ├─── CAN_GND ──── Device 2
                      ├─── CAN_GND ──── Device 3
                      └─── Battery Negative (GROUND AT ONE POINT ONLY)
```

**Grounding:** All devices connect CAN_GND to common wire. CAN_GND wire grounded at shield (battery negative) - single point only.

### Example 3: Mixed Network (2-Wire + 3-Wire Devices)

```
Shield J20:
  Pin 1 (CANH) ────┬─── CANH ──── Device A (2-wire)
                   ├─── CANH ──── Device B (2-wire)
                   └─── CANH ──── Device C (3-wire)
  
  Pin 2 (CANL) ────┬─── CANL ──── Device A (2-wire)
                   ├─── CANL ──── Device B (2-wire)
                   └─── CANL ──── Device C (3-wire)
  
  Pin 3 (CAN_GND) ──── CAN_GND ──── Device C (3-wire)
                      └─── Battery Negative (GROUND AT ONE POINT ONLY)
  
  Device A & B (2-wire): CAN_GND not connected (use chassis ground)
```

## Troubleshooting

**Intermittent CAN Errors:**
- Cause: Ground loops (CAN_GND grounded at multiple points)
- Solution: Ensure CAN_GND grounded at ONE point only (shield)

**Communication Fails on Long Runs:**
- Cause: Missing CAN_GND wire (2-wire on long runs)
- Solution: Use 3-wire CAN with dedicated CAN_GND wire

**Noise/EMI Issues:**
- Cause: Unshielded cable or improper shield grounding
- Solution: Use shielded twisted pair, ground shield at ONE end only

**Signal Reflections:**
- Cause: Missing termination resistors or long stubs
- Solution: Add 120Ω resistors at both ends, keep stubs < 0.3m

## Recommendations for Universal Shield

**Connector:**
- **Primary:** Phoenix Contact MSTB 2.5/3-ST (3-pin, spring clamp) - Best balance of cost, ease of installation, vibration resistance
- **Alternative:** Deutsch DT or TE AMP Superseal (severe vibration, IP67)
- **Acceptable:** Molex MX150-3 (3-pin) or MX150-4/6 (extra pins unused)
- **Not Recommended:** Standard screw terminals for high-vibration applications

**PCB Design:**
- Always route CANH, CANL, and CAN_GND traces to connector
- Always include Pin 3 on connector (even if unused in 2-wire networks)
- Keep CAN_GND isolated from main ground plane (connect only to ADM3057E GNDISO and connector Pin 3)

**Wiring:**
- Use twisted pair cable (120Ω impedance)
- Shielded cable for high-noise environments
- 120Ω termination at both ends
- Single point grounding for CAN_GND (critical!)
- Keep stubs short (< 0.3m)

## Summary

✅ **Universal Shield supports both 2-wire and 3-wire CAN** via 3-pin connector
✅ **Always route CAN_GND trace on PCB** (flexibility for both network types)
✅ **Use vibration-resistant connectors** (spring clamp or crimp-style) for high-vibration applications
✅ **Single point grounding** for CAN_GND (critical for reliable operation)
✅ **3-pin connector recommended** (exact fit), 4/6-pin acceptable (extra pins unused)

