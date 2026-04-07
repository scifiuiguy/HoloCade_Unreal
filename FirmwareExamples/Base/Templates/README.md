# HoloCade Communication Templates

**Standalone header templates for easy integration of wireless and CAN bus communication in microcontroller firmware sketches.**

---

## 📦 Overview

These header templates provide a simple, drop-in solution for bidirectional wireless communication between microcontrollers and Unreal Engine using the HoloCade EmbeddedSystems protocol.

### Available Templates

| Template | Purpose | Platforms |
|----------|---------|-----------|
| **`HoloCade_Wireless_TX.h`** | Transmit to game engine | ESP32, ESP8266, Arduino+WiFi, STM32+WiFi, RPi, Jetson |
| **`HoloCade_Wireless_RX.h`** | Receive from game engine | ESP32, ESP8266, Arduino+WiFi, STM32+WiFi, RPi, Jetson |
| **`HoloCade_CAN.h`** | CAN bus communication | ESP32, Arduino (MCP2515), STM32, Linux (SocketCAN) |
| **`ScissorLift_Controller.h`** | Scissor lift control | All platforms (CAN or GPIO mode) |
| **`ActuatorSystem_Controller.h`** | Actuator system control | All platforms |

### Supported Platforms

| Platform | Wireless | CAN Bus | Template Files |
|----------|----------|---------|----------------|
| **ESP32** | ✅ Built-in WiFi | ✅ Native TWAI | All templates |
| **ESP8266** | ✅ Built-in WiFi | ✅ MCP2515 | Wireless templates |
| **Arduino + WiFi Shield** | ✅ Via shield | ✅ MCP2515 | All templates |
| **STM32 + WiFi Module** | ✅ Via module | ✅ Native CAN | All templates |
| **Raspberry Pi** | ✅ Built-in WiFi | ✅ SocketCAN | All templates |
| **Jetson Nano** | ✅ Built-in WiFi | ✅ SocketCAN | All templates |
| **Arduino (no WiFi)** | ❌ | ✅ MCP2515 | CAN templates only |
| **STM32 (no WiFi)** | ❌ | ✅ Native CAN | CAN templates only |

---

## 🚀 Quick Start

### **Transmitting to Unreal (TX)**

```cpp
#include "HoloCade_Wireless_TX.h"

void setup() {
  // Initialize wireless communication
  HoloCade_Wireless_Init(
    "VR_Arcade_LAN",                    // WiFi SSID
    "your_password",                     // WiFi password
    IPAddress(192, 168, 1, 100),        // Unreal PC IP
    8888                                 // UDP port
  );
}

void loop() {
  // Send button press
  if (digitalRead(BUTTON_PIN) == LOW) {
    HoloCade_SendBool(0, true);
  }
  
  // Send sensor value
  float sensorValue = analogRead(SENSOR_PIN) / 1024.0f;
  HoloCade_SendFloat(1, sensorValue);
  
  delay(10);
}
```

### **Receiving from Unreal (RX)**

```cpp
#include "HoloCade_Wireless_RX.h"

// Implement handlers
void HoloCade_HandleBool(uint8_t channel, bool value) {
  if (channel == 0) {
    // Unlock door
    digitalWrite(LOCK_PIN, value ? HIGH : LOW);
  }
}

void HoloCade_HandleFloat(uint8_t channel, float value) {
  if (channel == 1) {
    // Set motor speed (0.0-1.0)
    analogWrite(MOTOR_PIN, (int)(value * 255));
  }
}

void setup() {
  // Initialize wireless communication
  HoloCade_Wireless_Init(
    "VR_Arcade_LAN",                    // WiFi SSID
    "your_password",                     // WiFi password
    8888                                 // UDP port
  );
}

void loop() {
  // Process incoming commands
  HoloCade_ProcessIncoming();
  
  delay(10);
}
```

---

## 🚀 CAN Bus Quick Start

### **Using CAN Bus Template**

```cpp
#include "HoloCade_CAN.h"

void setup() {
  // Initialize CAN bus (500 kbps, MCP2515 CS pin 10)
  HoloCade_CAN_Init(500000, 10);
  
  // Send joystick command to scissor lift ECU
  HoloCade_CAN_SendLiftJoystickCommand(0.5f, 0.0f, 0x180);
  // Vertical: 0.5 (up), Forward: 0.0 (neutral), CAN ID: 0x180
  
  // Send emergency stop
  HoloCade_CAN_SendLiftEmergencyStop(true, 0x200);
}

void loop() {
  // CAN bus communication happens here
  delay(10);
}
```

**Platform Notes:**
- **ESP32:** Uses native TWAI (no MCP2515 needed)
- **Arduino:** Requires MCP2515 CAN controller module
- **STM32:** Uses native CAN controller
- **Linux (RPi/Jetson):** Uses SocketCAN (interface: "can0")

**See `GunshipExperience/FourPlayerRig/README.md` for complete CAN bus configuration guide.**

---

## 📋 API Reference

### **TX Template (`HoloCade_Wireless_TX.h`)**

#### Initialization
```cpp
void HoloCade_Wireless_Init(const char* ssid, const char* password, IPAddress targetIP, uint16_t targetPort = 8888);
```

#### Transmission Functions
```cpp
void HoloCade_SendBool(uint8_t channel, bool value);
void HoloCade_SendInt32(uint8_t channel, int32_t value);
void HoloCade_SendFloat(uint8_t channel, float value);
void HoloCade_SendString(uint8_t channel, const char* str);
```

### **RX Template (`HoloCade_Wireless_RX.h`)**

#### Initialization
```cpp
void HoloCade_Wireless_Init(const char* ssid, const char* password, uint16_t localPort = 8888);
```

#### Reception Functions
```cpp
void HoloCade_ProcessIncoming();  // Call regularly in loop()
```

#### Handler Functions (Implement in your sketch)
```cpp
void HoloCade_HandleBool(uint8_t channel, bool value);
void HoloCade_HandleInt32(uint8_t channel, int32_t value);
void HoloCade_HandleFloat(uint8_t channel, float value);
void HoloCade_HandleString(uint8_t channel, const char* str, uint8_t length);
```

---

## 📊 Protocol Details

### **Packet Format**

```
[Marker:1] [Type:1] [Channel:1] [Payload:N] [CRC:1]
```

| Field | Size | Description |
|-------|------|-------------|
| Marker | 1 byte | Always `0xAA` (start of packet) |
| Type | 1 byte | Data type (0=Bool, 1=Int32, 2=Float, 3=String) |
| Channel | 1 byte | Channel/pin number (0-255) |
| Payload | N bytes | Data (variable length) |
| CRC | 1 byte | XOR checksum of all preceding bytes |

### **Data Types**

| Type | Value | Payload Size | Example |
|------|-------|--------------|---------|
| Bool | 0 | 1 byte | `true` / `false` |
| Int32 | 1 | 4 bytes | `42` |
| Float | 2 | 4 bytes | `3.14f` |
| String | 3 | 1-255 bytes | `"Hello"` |

---

## 🔒 Security

For production deployments, use the secured firmware templates with AES-128 encryption and HMAC authentication. See the EmbeddedSystems module documentation for secured firmware examples.

**Security Levels:**
- **None** - Development only (CRC checksum)
- **HMAC** - Authentication (prevents spoofing)
- **Encrypted** - Full confidentiality (AES-128 + HMAC)

---

## 🛠️ Platform-Specific Notes

### **ESP32 / ESP8266**
- Built-in WiFi support
- No additional hardware required
- Works out of the box

### **Arduino + WiFi Shield**
- Requires compatible WiFi shield (e.g., ESP8266-based)
- May need to adjust library includes based on shield model

### **STM32 + WiFi Module**
- Requires external WiFi module (e.g., ESP8266, ESP32)
- Use appropriate STM32 WiFi library for your module

### **Raspberry Pi / Jetson Nano**
- Uses standard Linux socket libraries
- May require additional WiFi configuration

---

## 📝 Integration Checklist

- [ ] Copy `HoloCade_Wireless_TX.h` and/or `HoloCade_Wireless_RX.h` to your sketch directory
- [ ] Configure WiFi credentials (SSID, password)
- [ ] Set Unreal PC IP address
- [ ] Implement handler functions (for RX)
- [ ] Call `HoloCade_ProcessIncoming()` in loop() (for RX)
- [ ] Test connection with Unreal Engine
- [ ] Configure security settings for production

---

## 🐛 Troubleshooting

### **"Platform not supported" error**
- Your platform doesn't have built-in wireless
- Use Serial templates instead (coming soon)

### **WiFi connection fails**
- Check SSID and password
- Verify WiFi network is 2.4GHz (ESP32 doesn't support 5GHz)
- Check signal strength

### **No packets received**
- Verify Unreal PC IP address is correct
- Check firewall allows UDP port 8888
- Ensure both devices are on same network
- Use Wireshark to monitor network traffic

### **CRC mismatch errors**
- Check for interference on WiFi network
- Verify both TX and RX use same protocol version
- Enable debug mode to inspect packets

---

## 📚 Additional Resources

- **[Base/Examples/README.md](../Examples/README.md)** - Complete example firmware
- **[GunshipExperience/FourPlayerRig/README.md](../../GunshipExperience/FourPlayerRig/README.md)** - CAN bus configuration guide
- **[EscapeRoom/README.md](../../EscapeRoom/README.md)** - Experience-specific examples
- **[EmbeddedSystems Module README](../../../Source/EmbeddedSystems/README.md)** - Full API documentation

---

## 🔌 CAN Bus Template (`HoloCade_CAN.h`)

### **Initialization**
```cpp
bool HoloCade_CAN_Init(uint32_t baudRate = 500000, int csPin = 10, const char* interface = "can0");
```

### **Send Commands**
```cpp
bool HoloCade_CAN_SendCommand(uint32_t canId, uint8_t* data, uint8_t dataLength);
void HoloCade_CAN_SendLiftJoystickCommand(float verticalCommand, float forwardCommand, uint32_t canIdBase = 0x180);
void HoloCade_CAN_SendLiftEmergencyStop(bool enable, uint32_t canIdBase = 0x200);
```

### **Platform Support**

| Platform | CAN Implementation | Notes |
|----------|-------------------|-------|
| **ESP32** | Native TWAI | GPIO 4 (TX), GPIO 5 (RX) by default |
| **Arduino** | MCP2515 via SPI | Requires MCP2515 module, specify CS pin |
| **STM32** | Native CAN | Uses STM32 CAN library |
| **Linux** | SocketCAN | Interface name: "can0" (configurable) |

**Important:** 
- CAN bus protocol documentation (including CAN IDs) is typically **proprietary** and not publicly available
- Default values (0x180, 0x200, 0x280) are **examples only** - replace with your manufacturer's actual CAN IDs
- You may need to:
  - Contact manufacturer support for protocol documentation
  - Use a CAN bus analyzer to reverse-engineer the protocol
  - Work with authorized dealers/service centers who have access to proprietary documentation

---

## 📄 License

MIT License - Copyright (c) 2025 AJ Campbell

---

**Built for HoloCade**

