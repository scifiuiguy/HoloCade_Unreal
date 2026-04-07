# Escape Room Examples

**Escape room specific firmware examples for door locks, props, and sensors.**

These examples demonstrate how to integrate embedded systems into escape room installations using the HoloCade EmbeddedSystems API.

---

## 📁 Examples

### **Door Lock Control**

Located in `DoorLock/` directory:

| File | Description | Supported Platforms |
|------|-------------|---------------------|
| **`DoorLock_Example.ino`** | Door lock controller (platform-agnostic) | ESP32, ESP8266, Arduino+WiFi, STM32+WiFi, Raspberry Pi, Jetson Nano |

**Supported Platforms:**
- ✅ **ESP32** - Full GPIO support, built-in WiFi (recommended)
- ✅ **ESP8266** - Limited GPIO pins, built-in WiFi (lower cost)
- ✅ **Arduino + WiFi Shield** - Requires WiFi shield (ESP8266-based)
- ✅ **STM32 + WiFi Module** - Requires external WiFi module (ESP8266/ESP32-based)
- ✅ **Raspberry Pi** - Linux-based, uses standard sockets
- ✅ **Jetson Nano** - Linux-based, uses standard sockets

**Note:** The example (`DoorLock_Example.ino`) works on all platforms. Adjust GPIO pin assignments in the Configuration section to match your hardware. See comments in the example for ESP8266-specific pin recommendations.

---

## 🎯 Features

### **Door Lock Controller**

- ✅ **Multi-door support** - Control up to 4 doors (configurable)
- ✅ **Solenoid/Servo control** - Works with both lock types
- ✅ **Door sensors** - Magnetic reed switches for state detection
- ✅ **Status LEDs** - Visual feedback for lock state
- ✅ **Wireless commands** - Receive unlock/lock commands from Unreal
- ✅ **State reporting** - Report door open/closed state

---

## 🔌 Hardware Setup

### **Solenoid Lock**
```
ESP32 GPIO 12 ──[Transistor]── Solenoid Lock ── 12V Power Supply
```

### **Servo Motor Lock**
```
ESP32 GPIO 12 ── Servo Signal (PWM)
Servo Power ── 5V Power Supply
Servo Ground ── GND
```

### **Door Sensor (Magnetic Reed Switch)**
```
ESP32 GPIO 13 ──[Pull-up Resistor]── Reed Switch ── GND
```

### **Status LED**
```
ESP32 GPIO 14 ──[220Ω Resistor]── LED ── GND
```

---

## 💻 Usage in Unreal Engine

```cpp
// In EscapeRoomExperience
AEscapeRoomExperience* EscapeRoom = GetWorld()->SpawnActor<AEscapeRoomExperience>();

// Unlock door 0
EscapeRoom->UnlockDoor(0);

// Check if door is unlocked
bool isUnlocked = EscapeRoom->IsDoorUnlocked(0);

// Lock door 0
EscapeRoom->LockDoor(0);
```

---

## 📝 Configuration

### **WiFi Setup**
```cpp
const char* ssid = "VR_Arcade_LAN";
const char* password = "your_password_here";
```

### **Door Count**
```cpp
const int NUM_DOORS = 4;  // Adjust based on your installation
```

### **GPIO Pins**
```cpp
// ESP32 example
const int lockPins[NUM_DOORS] = {12, 14, 27, 26};
const int sensorPins[NUM_DOORS] = {13, 15, 32, 33};
const int ledPins[NUM_DOORS] = {2, 4, 5, 18};
```

---

## 🔧 Platform-Specific Notes

### **Platform Notes**

All platforms use the same core functionality. Differences are primarily in:
- GPIO pin assignments (adjust in Configuration section)
- WiFi library includes (platform-specific)
- Serial output methods (platform-specific)

**ESP32** (Recommended):
- Full GPIO support
- Built-in WiFi
- Use `DoorLock_Example.ino` with ESP32 pin assignments

**ESP8266**:
- Limited GPIO pins (avoid GPIO 0, 15, 16 for some functions)
- Lower cost option
- Use `DoorLock_Example.ino` with ESP8266-compatible pin assignments (see comments in example)

**Other Platforms**:
- Use `DoorLock_Example.ino` as base
- Adjust GPIO pins in Configuration section
- Use platform-specific WiFi libraries
- See platform documentation for GPIO setup

---

## 🚀 Integration with Narrative State Machine

The `EscapeRoomExperience` automatically unlocks doors based on narrative state progression:

```cpp
// In EscapeRoomExperience::OnNarrativeStateChanged()
if (NewState == FName("Puzzle1")) {
    UnlockDoor(0);  // Unlock first door when entering Puzzle1
}
else if (NewState == FName("Puzzle2")) {
    UnlockDoor(1);  // Unlock second door when entering Puzzle2
}
```

---

## 📚 Related Documentation

- **[Base/Templates/README.md](../../Base/Templates/README.md)** - Using header templates
- **[Base/Examples/README.md](../../Base/Examples/README.md)** - Base example documentation
- **[EscapeRoomExperience.h](../../../Source/HoloCadeExperiences/Public/EscapeRoomExperience.h)** - Unreal API

---

## 📄 License

MIT License - Copyright (c) 2025 AJ Campbell

