# Wireless DMX Child ECU - Design Specification

## Overview

Battery-powered ESP32 Child ECU that receives Art-Net/sACN lighting data wirelessly and outputs standard DMX to fixtures. Designed for scenarios where fixtures must be suspended in air without cables, requiring wireless communication and battery power.

## Architecture

```
Game Engine / Light Board
    ↓ (Art-Net/sACN over network)
Universal Shield (ground-based or server-side)
    ↓ (WiFi Art-Net/sACN)
ESP32 DMX Child ECU (battery-powered, attached to fixture)
    ↓ (DMX output via RS-485)
Suspended Fixture
```

## Use Case

**Scenario:** Installation has multiple Art-Net fixtures all controlled from game-engine console and light board, but one fixture needs to be suspended in air without cables.

**Solution:**
- Fixture goes battery-operated
- ESP32 Child ECU attached to fixture, powered from same battery
- ESP32 receives Art-Net/sACN wirelessly from Universal Shield
- Universal Shield can be:
  - **Ground-based (in play):** Receives Art-Net from network, broadcasts to WiFi Child ECU
  - **Server-side:** Provides network connectivity; Child ECU connects directly to network WiFi

## Hardware Design

### Core Components

| Component | Part Number | Purpose | Notes |
|-----------|-------------|---------|-------|
| MCU | ESP32-S3 or ESP32-WROOM | WiFi + Art-Net/sACN processing | Built-in WiFi, sufficient processing power |
| RS-485 Transceiver | MAX485, SN75176, or similar | DMX output driver | Converts UART to DMX-compatible RS-485 |
| DMX Connector | 3-pin or 5-pin XLR | Standard fixture interface | Industry standard DMX connector |
| Battery Management | LiPo/LiFePO4 BMS | Battery charging & protection | Depends on fixture runtime requirements |
| Power Regulation | 3.3V LDO | ESP32 power supply | May need 5V for RS-485 if required |
| Status LED | Standard LED | Power/status indication | Optional but recommended |
| Power Switch | Toggle or push-button | On/off control | Optional but recommended |

### Power Considerations

- **ESP32-S3 WiFi Active:** ~80-170mA
- **RS-485 Transceiver:** ~5-20mA
- **Total Active Current:** ~100-200mA
- **Deep Sleep (between updates):** ~10-150µA (if update rate allows)
- **Battery Capacity:** Depends on fixture runtime needs (could be hours to days)

### Communication

- **WiFi:** ESP32-S3 native 802.11 b/g/n
- **Protocol:** Art-Net (UDP port 6454) or sACN (UDP port 5568)
- **DMX Output:** Standard DMX512 (250kbps, 8 data bits, 2 stop bits, no parity)
- **DMX Refresh Rate:** Art-Net typically 30-44Hz; ESP32 can easily handle this

## Firmware Requirements

### Core Functionality

1. **WiFi Connection Management**
   - Connect to network (same network as Universal Shield or direct to network)
   - Handle reconnection on WiFi drop
   - Optional: WiFi mesh/repeater support for extended range

2. **Art-Net/sACN Receiver**
   - Listen for UDP packets on Art-Net port (6454) or sACN port (5568)
   - Parse Art-Net/sACN packets
   - Extract DMX universe data
   - Handle multiple universes if needed

3. **DMX Output Driver**
   - Convert DMX data to UART stream
   - Drive RS-485 transceiver
   - Maintain DMX timing (break, mark, start code, data)
   - Handle DMX refresh rate (typically 30-44Hz)

4. **Power Management** (Optional)
   - Deep sleep between Art-Net updates if update rate allows
   - Wake on WiFi activity or timer
   - Battery level monitoring (if BMS provides)

### Libraries & Resources

- **Art-Net Libraries:** Available on GitHub for ESP32
- **sACN Libraries:** Available on GitHub for ESP32
- **DMX Libraries:** ESP32 DMX output libraries available
- **Example Projects:** ESP8266 Art-Net to DMX projects exist (can be adapted)

## Design Considerations

### Advantages

- **Modular:** Dedicated Child ECU, doesn't burden Universal Shield
- **Flexible:** Universal Shield can be ground-based (in play) or server-side
- **Standard:** Fixture sees normal DMX, no special protocol needed
- **Scalable:** Multiple wireless fixtures = multiple Child ECUs
- **Battery-Friendly:** ESP32 can sleep between updates, extend runtime

### Challenges

1. **Power Consumption**
   - WiFi active mode draws significant current
   - Consider deep sleep between updates if update rate allows
   - Battery capacity must match fixture runtime needs

2. **WiFi Range**
   - ESP32-S3 has good range (~100m line-of-sight)
   - Consider mesh/repeater if fixture is very high or far
   - May need external antenna for extended range

3. **DMX Refresh Rate**
   - Art-Net typically 30-44Hz
   - ESP32 can easily handle this
   - Ensure UART timing is correct for DMX (250kbps)

4. **Battery Life**
   - Depends on update rate, WiFi power, and battery capacity
   - May need larger battery for longer runtime
   - Consider power-saving modes if possible

## Integration with Universal Shield

### Universal Shield Role

- **Option A (Ground-based):** Universal Shield in play, receives Art-Net from network, broadcasts to WiFi Child ECU
- **Option B (Server-side):** Universal Shield at server rack, just provides network connectivity; Child ECU connects directly to network WiFi

### UART Breakout on Universal Shield

The Universal Shield's UART breakout (GND, 3.3V, U0TXD, U0RXD) remains available for other debug/interface needs. The DMX Child ECU is a separate, dedicated module that doesn't require the Universal Shield's UART pins.

## Commercial Alternatives

### Existing Products

- **DMXKing eDMX WiFi** - Art-Net/sACN to DMX bridge over WiFi (~$100-200)
- **Enttec ODE Mk2** - Art-Net/sACN to DMX with WiFi option (~$200-300)
- **DMX-Workshop WiFi-DMX** - Art-Net to DMX bridge (~$100-150)
- **ADJ Airstream DMX Bridge** - WiFi DMX (proprietary, not Art-Net) (~$100-200)
- **CLF W-Bridge** - Wireless DMX (proprietary protocol) (~$100-150)

### DIY vs Commercial

**Commercial Advantages:**
- Proven reliability
- Manufacturer support
- May include additional features

**DIY Advantages:**
- Lower cost (~$10-20 vs $100-300)
- Custom features
- Integration with HoloCade ecosystem
- Learning/flexibility
- Battery-powered option (most commercial units require AC power)

## Implementation Status

**Status:** Planned for v2.0

**Priority:** Low (rare use case, but valuable when needed)

**Dependencies:**
- Universal Shield (already designed)
- Art-Net/sACN libraries for ESP32
- DMX output driver for ESP32

## References

- [Art-Net Protocol Specification](https://art-net.org.uk/)
- [sACN (E1.31) Protocol Specification](https://tsp.esta.org/tsp/documents/docs/ANSI_E1-31-2018.pdf)
- [DMX512-A Standard](https://tsp.esta.org/tsp/documents/docs/ANSI_E1-11-2017.pdf)
- ESP32 Art-Net libraries (GitHub)
- ESP32 DMX libraries (GitHub)




