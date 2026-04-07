# HoloCade Firmware Examples

**Firmware examples, templates, and documentation for HoloCade Embedded Systems integration.**

---

## 📁 Directory Structure

```
FirmwareExamples/
├── Base/                          # Generic examples and templates for all experiences
│   ├── Templates/                  # Reusable header templates
│   │   ├── HoloCade_Wireless_TX.h   # Wireless transmission template
│   │   ├── HoloCade_Wireless_RX.h   # Wireless reception template
│   │   ├── HoloCade_CAN.h           # CAN bus communication template
│   │   ├── ScissorLift_Controller.h  # Scissor lift control (CAN or GPIO)
│   │   └── ActuatorSystem_Controller.h  # Actuator system control
│   └── Examples/                   # Functionality-based examples
│       ├── ButtonMotor_Example.ino              # Main example (all platforms)
│       ├── ScissorLift_Controller.ino          # Scissor lift standalone
│       └── ActuatorSystem_Controller.ino       # Actuator system standalone
│
├── EscapeRoom/                     # Escape room specific examples
│   └── DoorLock/                   # Door lock control examples
│       └── DoorLock_Example.ino                 # Main example (all platforms)
│
└── GunshipExperience/              # Gunship experience specific examples
    ├── FourPlayerRig/               # Four-player scissor lift platform variant
    │   ├── GunshipExperience_ECU.ino   # Parent ECU for 4DOF motion platform (uses Universal Shield)
    │   ├── Gun_ECU.ino                  # Child ECU for per-station gun control
    │   └── README.md                    # Four-player rig firmware documentation
    └── SinglePlayerRig/            # Single-player custom chassis variant
        ├── GunshipExperience_ECU.ino   # Parent ECU for single-player motion platform (uses Driver Shield)
        ├── Gun_ECU.ino                  # Child ECU for gun control with servos
        └── README.md                    # Single-player rig firmware documentation
```

---

## 🎯 Quick Start

### **Using Templates**

1. **Copy template header** to your sketch directory:
   ```cpp
   // Copy from: FirmwareExamples/Base/Templates/HoloCade_Wireless_RX.h
   // To: YourSketch/HoloCade_Wireless_RX.h
   ```

2. **Include in your sketch**:
   ```cpp
   #include "HoloCade_Wireless_RX.h"
   
   void setup() {
     HoloCade_Wireless_Init("VR_Arcade_LAN", "password", 8888);
   }
   
   void loop() {
     HoloCade_ProcessIncoming();
   }
   ```

### **Using Examples**

1. **Choose your platform** (ESP32, ESP8266, Arduino+WiFi, etc.)
2. **Open the appropriate example**:
   - Use main example (`ButtonMotor_Example.ino` or `DoorLock_Example.ino`) for most platforms
   - Use ESP8266 variant if using ESP8266 (shows platform-specific pin config)
3. **Adjust GPIO pins** in Configuration section to match your hardware
4. **Configure WiFi credentials** and Unreal PC IP address
5. **Upload to your microcontroller**

---

## 📚 Documentation

- **[Base/Templates/README.md](Base/Templates/README.md)** - Template usage guide
- **[Base/Examples/README.md](Base/Examples/README.md)** - Base example documentation
- **[EscapeRoom/README.md](EscapeRoom/README.md)** - Escape room examples guide
- **[GunshipExperience/FourPlayerRig/README.md](GunshipExperience/FourPlayerRig/README.md)** - Four-player rig firmware documentation
- **[GunshipExperience/SinglePlayerRig/README.md](GunshipExperience/SinglePlayerRig/README.md)** - Single-player rig firmware documentation

---

## 🔧 Platform Support

All examples support multiple platforms. The main examples work on all platforms with minor GPIO pin adjustments.

| Platform | Wireless | Example File | Notes |
|----------|----------|--------------|-------|
| **ESP32** | ✅ Built-in | All examples | Full support, recommended |
| **ESP8266** | ✅ Built-in | All examples | Limited GPIO pins, adjust pin assignments |
| **ESP32-C3 (Pico-Click)** | ✅ Built-in | All examples | **v2.0:** Will replace ESP8266 as default for wireless buttons in clothing/props |
| **Arduino + WiFi Shield** | ✅ Via shield | All examples | Adjust GPIO pins, use shield library |
| **STM32 + WiFi Module** | ✅ Via module | All examples | Adjust GPIO pins, use module library |
| **Raspberry Pi** | ✅ Built-in | All examples | Adjust GPIO pins, use Linux sockets |
| **Jetson Nano** | ✅ Built-in | All examples | Adjust GPIO pins, use Linux sockets |

**Note:** All examples are platform-agnostic. Adjust GPIO pin assignments in the Configuration section to match your hardware. See comments in each example for platform-specific pin recommendations.

### **v2.0 Platform Migration: Pico-Click C3**

**Planned for v2.0:** The open-source [Pico-Click C3](https://github.com/pico-click) project (ESP32-C3 based) will be integrated as the default platform for hidden embedded wireless buttons in clothing and props, replacing the current ESP8266-based implementation.

**Advantages of ESP32-C3 over ESP8266:**
- **Performance:** 160 MHz RISC-V processor (vs. 80 MHz on ESP8266)
- **Connectivity:** Dual WiFi + Bluetooth 5.0 (vs. WiFi-only on ESP8266)
- **Power Efficiency:** 5 µA deep sleep (vs. 20 µA on ESP8266) - critical for battery-powered costume buttons
- **Memory:** 400 KB SRAM (vs. limited SRAM on ESP8266)
- **Form Factor:** Pico-Click C3 provides optimized form factor for embedding in clothing and props

Current ESP8266 examples will be migrated to Pico-Click C3 in v2.0. See main roadmap for details.

---

## 📝 Notes

### **FirmwareExamples Folder**

**Important:** The `FirmwareExamples/` folder is **not a special folder** in either engine. It's purely for **organization and documentation** of firmware examples and templates.

- ✅ **Safe to reorganize** - No engine dependencies
- ✅ **Safe to rename** - No code references
- ✅ **Documentation only** - Examples and templates for developers
- ✅ **Not included in builds** - Firmware files are not game engine assets

**Note:** We use `FirmwareExamples/` instead of `Resources/` to avoid Unity's special `Resources/` folder behavior (which includes all assets in builds). Firmware examples are not Unity assets and should not be included in game builds.

**Organization:** All embedded hardware firmware examples are located in `FirmwareExamples/`, organized by experience type. Game engine code references these examples in documentation but does not depend on their location.

---

## 🚀 Experience-Specific Examples

Examples are organized by experience type:

- **Base/** - Generic examples usable by any experience
  - `ScissorLift_Controller.ino` - Standalone scissor lift control (CAN bus or GPIO)
  - `ActuatorSystem_Controller.ino` - Standalone 4-actuator hydraulic control
  - `ButtonMotor_Example.ino` - Generic button & motor example
- **EscapeRoom/** - Escape room specific (door locks, props, sensors)
- **GunshipExperience/** - Gunship experience
  - **FourPlayerRig/** - Four-player scissor lift platform (parent ECU for 4DOF motion platform, child ECUs for per-station gun control)
  - **SinglePlayerRig/** - Single-player custom chassis (parent ECU for single-player motion platform, child ECU for gun control with servos)
- **AIFacemask/** - (Future) Live actor costume examples
- **MovingPlatform/** - (Future) Motion platform sensor examples

## 🔌 CAN Bus Support

The scissor lift controller supports **CAN bus communication** for manufacturer ECUs (e.g., Genie/Skyjack lifts):

- **CAN Bus Mode (default):** Sends joystick commands to manufacturer ECU via CAN bus
- **Direct GPIO Mode:** Direct motor control for custom builds or testing
- **Platform Support:** ESP32 (native TWAI), Arduino (MCP2515), STM32 (native), Linux (SocketCAN)

See `GunshipExperience/FourPlayerRig/README.md` for CAN bus configuration instructions.

---

## 📄 License

MIT License - Copyright (c) 2025 AJ Campbell

---

**Built for HoloCade**

