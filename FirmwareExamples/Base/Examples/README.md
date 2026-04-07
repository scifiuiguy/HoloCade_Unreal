# Base Examples

**Generic firmware examples demonstrating HoloCade EmbeddedSystems protocol communication.**

These examples are **platform-agnostic** in functionality but **platform-specific** in implementation. Each example demonstrates the same core features but uses platform-specific libraries and pin configurations.

---

## 📋 Available Examples

| File | Description | Supported Platforms |
|------|-------------|---------------------|
| **`ButtonMotor_Example.ino`** | Button & motor controller (platform-agnostic) | ESP32, ESP8266, Arduino+WiFi, STM32+WiFi, Raspberry Pi, Jetson Nano |
| **`ScissorLift_Controller.ino`** | Scissor lift control (CAN bus or GPIO mode) | ESP32, ESP8266, Arduino, STM32, Raspberry Pi, Jetson Nano |
| **`ActuatorSystem_Controller.ino`** | 4-actuator hydraulic system control | ESP32, ESP8266, Arduino, STM32, Raspberry Pi, Jetson Nano |

**Supported Platforms:**
- ✅ **ESP32** - Full GPIO support, built-in WiFi (recommended)
- ✅ **ESP8266** - Limited GPIO pins, built-in WiFi (lower cost)
- ✅ **Arduino + WiFi Shield** - Requires WiFi shield (ESP8266-based)
- ✅ **STM32 + WiFi Module** - Requires external WiFi module (ESP8266/ESP32-based)
- ✅ **Raspberry Pi** - Linux-based, uses standard sockets
- ✅ **Jetson Nano** - Linux-based, uses standard sockets

**Note:** The example (`ButtonMotor_Example.ino`) works on all platforms. Adjust GPIO pin assignments in the Configuration section to match your hardware. See comments in the example for ESP8266-specific pin recommendations.

---

## 🎯 Features Demonstrated

### **ButtonMotor_Example.ino**
- ✅ **4 buttons** - Digital input reading and transmission
- ✅ **6 vibration motors** - PWM output control via commands
- ✅ **Bidirectional communication** - Send button states, receive motor commands
- ✅ **Binary protocol** - Low-latency HoloCade protocol
- ✅ **CRC validation** - Packet integrity checking

### **ScissorLift_Controller.ino**
- ✅ **CAN Bus Mode** - Communicate with manufacturer ECUs (Genie/Skyjack)
- ✅ **Direct GPIO Mode** - Direct motor control for custom builds
- ✅ **Vertical Translation** - Lift up/down control
- ✅ **Forward/Reverse Drive** - Optional lateral movement (can be disabled)
- ✅ **Position Feedback** - GPIO analog input or CAN bus feedback
- ✅ **Calibration** - Automatic zero-point calibration
- ✅ **Safety Limits** - Hardware and software limits

### **ActuatorSystem_Controller.ino**
- ✅ **4-Actuator Control** - Independent control of 4 hydraulic actuators
- ✅ **Pitch/Roll Control** - Platform tilt control
- ✅ **Closed-Loop Control** - PID control with position sensors
- ✅ **Calibration** - Automatic calibration with limit switches
- ✅ **Safety Limits** - Hardware and software limits

---

## 🔧 Platform-Specific Notes

### **ESP32**
- Uses `WiFi.h` and `WiFiUdp.h` (built-in)
- GPIO pins: 2, 4, 5, 12, 13, 14, 18, 25, 26, 27
- Full PWM support on all pins
- Recommended for most projects

### **ESP8266**
- Uses `ESP8266WiFi.h` and `WiFiUdp.h`
- Limited GPIO pins (avoid GPIO 0, 15, 16 for some functions)
- GPIO pins: 2, 4, 5, 12, 13, 14, 16
- Lower cost, fewer pins than ESP32

### **Arduino + WiFi Shield**
- Uses shield-specific WiFi library (e.g., `WiFiShield.h`)
- Standard Arduino GPIO pins (varies by board)
- Requires compatible WiFi shield (ESP8266-based recommended)
- Check shield documentation for pin assignments

### **STM32 + WiFi Module**
- Uses STM32 WiFi library (module-specific)
- GPIO pins vary by STM32 board
- Requires external WiFi module (ESP8266 or ESP32-based)
- Module-specific setup required

### **Raspberry Pi**
- Uses Linux socket libraries
- GPIO via WiringPi or pigpio
- Full Linux networking stack
- More complex setup, more powerful

### **Jetson Nano**
- Uses Linux socket libraries
- GPIO via Jetson GPIO library
- Full Linux networking stack
- Most powerful, most complex

---

## 📝 Usage

1. **Open the example file** (`ButtonMotor_Example.ino`)
2. **Configure WiFi credentials**:
   ```cpp
   const char* ssid = "VR_Arcade_LAN";
   const char* password = "your_password_here";
   ```
3. **Set Unreal PC IP address**:
   ```cpp
   IPAddress unrealIP(192, 168, 1, 100);
   ```
4. **Adjust GPIO pins** if needed (check your hardware - see comments for ESP8266 pin recommendations)
5. **Upload to microcontroller**

---

## 🔌 Hardware Setup

### **Buttons**
- Connect buttons between GPIO pin and GND
- Use internal pull-up resistors (`INPUT_PULLUP`)
- Active LOW (pressed = LOW, released = HIGH)

### **Vibration Motors**
- Connect motors via NPN transistors or motor drivers
- Use PWM pins for intensity control
- Check motor voltage requirements (may need external power)

---

## 🐛 Troubleshooting

### **WiFi connection fails**
- Check SSID and password
- Verify 2.4GHz network (ESP32/ESP8266 don't support 5GHz)
- Check signal strength

### **No packets received**
- Verify Unreal PC IP address
- Check firewall allows UDP port 8888
- Ensure both devices on same network
- Use Wireshark to monitor network traffic

### **GPIO not working**
- Check pin assignments match your hardware
- Verify pin modes (INPUT_PULLUP, OUTPUT)
- Some pins have special functions (boot, flash) - avoid these

---

## 📚 Related Documentation

- **[Templates/README.md](../Templates/README.md)** - Using header templates
- **[EscapeRoom/README.md](../../EscapeRoom/README.md)** - Experience-specific examples
- **[EmbeddedSystems Module README](../../../Source/EmbeddedSystems/README.md)** - Full API documentation

---

## 📄 License

MIT License - Copyright (c) 2025 AJ Campbell

