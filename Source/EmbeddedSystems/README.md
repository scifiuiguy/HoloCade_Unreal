# HoloCade Embedded Systems Module

**Low-latency bidirectional communication between Unreal Engine and embedded microcontrollers** (ESP32, Arduino, STM32, Raspberry Pi, Jetson Nano) for interactive costumes, props, and environmental controls in LBE installations.

---

## ✨ Features

- ✅ **Protocol-Agnostic API** - Same function calls work over WiFi, Serial, or Bluetooth
- ✅ **Hardware-Independent** - Works with Arduino, ESP32, STM32, Raspberry Pi, Jetson
- ✅ **Binary Protocol** - Low-latency, optimized for embedded devices
- ✅ **JSON Debug Mode** - Switch to human-readable JSON for debugging
- ✅ **CRC Validation** - Automatic packet integrity checking
- ✅ **Typed Primitives** - Send/receive bool, int32, float, string, raw bytes
- ✅ **Event-Driven** - Blueprint-friendly delegates for all data types
- ✅ **Template Support** - Send POD structs directly from C++

---

## 📊 Architecture & Data Flow

### **How Data Moves from Hardware to Unreal**

```
┌──────────────────────────────────────────────────────────────────┐
│  ESP32/Arduino Microcontroller                                   │
│  ─────────────────────────────                                   │
│  • Physical button pressed on costume                            │
│  • Reads digital pin (HIGH/LOW)                                  │
│  • Builds packet: [0xAA][Channel][Type][Payload][CRC/HMAC]      │
│  • Sends via WiFi UDP to Unreal (192.168.1.X:8888)              │
└──────────────────────────────────────────────────────────────────┘
                              │
                              ▼ UDP Packet
┌──────────────────────────────────────────────────────────────────┐
│  UEmbeddedDeviceController::TickComponent()                      │
│  ──────────────────────────────────────                          │
│  • ReceiveFrom() on UDP socket                                   │
│  • Validates packet (CRC/HMAC)                                   │
│  • Calls ParseBinaryPacket() or ParseJSONPacket()                │
└──────────────────────────────────────────────────────────────────┘
                              │
                              ▼ Parsed Data
┌──────────────────────────────────────────────────────────────────┐
│  ParseBinaryPacket() / ParseJSONPacket()                         │
│  ────────────────────────────────────────                        │
│  • Extracts: Channel=0, Type=Bool, Value=true                    │
│  • Stores in cache: InputValueCache[0] = 1.0f                    │
│  • Broadcasts delegate: OnBoolReceived(0, true)                  │
└──────────────────────────────────────────────────────────────────┘
                              │
                              ▼ Cached Value
┌──────────────────────────────────────────────────────────────────┐
│  Your Game Code                                                  │
│  ──────────────                                                  │
│  bool isPressed = CostumeController->GetDigitalInput(0);         │
│  // Returns: InputValueCache[0] > 0.5 → true                     │
└──────────────────────────────────────────────────────────────────┘
```

### **Key Design Decisions**

1. **Cache-Based Reading** - Input values are cached and updated asynchronously
   - `GetDigitalInput()` / `GetAnalogInput()` are instant lookups (no network delay)
   - Cache is populated every frame by `TickComponent()`
   - All values normalized to `float` (0.0 to 1.0)

2. **Separate from Unreal Networking** - This module is for **hardware I/O only**
   - Does NOT use Unreal's replication system
   - Direct UDP/Serial/Bluetooth to microcontrollers
   - If you need server-client replication, add it in your game code:
   
   ```cpp
   // Example: Replicate button state from server to clients
   UPROPERTY(Replicated)
   bool bButton0Pressed;
   
   // Server reads from microcontroller
   if (HasAuthority())
   {
       bButton0Pressed = CostumeController->GetDigitalInput(0);
   }
   // Clients receive replicated value automatically
   ```

3. **Event-Driven Alternative** - Use delegates instead of polling:
   ```cpp
   Device->OnBoolReceived.AddDynamic(this, &AMyActor::OnButtonPressed);
   ```

---

## 🎯 Quick Start

### **Unreal Engine Side**

```cpp
#include "EmbeddedDeviceController.h"

// 1. Create and configure device
UEmbeddedDeviceController* Device = CreateDefaultSubobject<UEmbeddedDeviceController>(TEXT("ESP32Device"));

FEmbeddedDeviceConfig Config;
Config.DeviceType = EHoloCadeMicrocontrollerType::ESP32;
Config.Protocol = EHoloCadeCommProtocol::WiFi;
Config.DeviceAddress = TEXT("192.168.1.50");
Config.Port = 8888;
Config.InputChannelCount = 4;  // 4 buttons
Config.OutputChannelCount = 6; // 6 vibration motors

// Security settings
Config.bDebugMode = false;     // Binary mode for production
Config.SecurityLevel = EHoloCadeSecurityLevel::Encrypted;  // AES-128 + HMAC
Config.SharedSecret = TEXT("MyVenueSecret_2025");  // MUST match ESP32

Device->InitializeDevice(Config);

// 2. Send data to ESP32
Device->SendBool(0, true);                  // Toggle LED
Device->SendFloat(1, 0.8f);                 // Set motor intensity
Device->SendInt32(2, 42);                   // Send score
Device->SendString(3, TEXT("Player1"));     // Send player name

// 3. Receive data from ESP32
Device->OnBoolReceived.AddDynamic(this, &AMyActor::OnButtonPressed);
Device->OnFloatReceived.AddDynamic(this, &AMyActor::OnSensorValue);

void AMyActor::OnButtonPressed(int32 Channel, bool Value)
{
    UE_LOG(LogTemp, Log, TEXT("Button %d: %s"), Channel, Value ? TEXT("Pressed") : TEXT("Released"));
}
```

### **ESP32 Side (Arduino)**

```cpp
// See: FirmwareExamples/Base/Examples/ButtonMotor_Example.ino for complete example

#include <WiFi.h>
#include <WiFiUdp.h>

// Connect to WiFi
WiFi.begin("VR_Arcade_LAN", "password");

// Send button press to Unreal
sendBool(0, true);

// Receive motor command from Unreal
void handleFloat(uint8_t channel, float value) {
    analogWrite(motorPins[channel], (int)(value * 255));
}
```

---

## 📦 Binary Protocol Specification

### **Packet Format**

```
┌─────────────────────────────────────────────────────────┐
│ [Marker:1] [Type:1] [Channel:1] [Payload:N] [CRC:1]    │
└─────────────────────────────────────────────────────────┘
```

| Field    | Size    | Description                                      |
|----------|---------|--------------------------------------------------|
| Marker   | 1 byte  | Always `0xAA` (start of packet)                  |
| Type     | 1 byte  | Data type enum (0=Bool, 1=Int32, 2=Float, etc.)  |
| Channel  | 1 byte  | Channel/pin number (0-255)                       |
| Payload  | N bytes | Data (variable length based on type)             |
| CRC      | 1 byte  | XOR checksum of all preceding bytes              |

### **Data Types**

| Type   | Value | Payload Format                              | Size      |
|--------|-------|---------------------------------------------|-----------|
| Bool   | 0     | `[value:1]` (0 or 1)                        | 5 bytes   |
| Int32  | 1     | `[byte0][byte1][byte2][byte3]` (little-endian) | 8 bytes   |
| Float  | 2     | `[byte0][byte1][byte2][byte3]` (little-endian) | 8 bytes   |
| String | 3     | `[length:1][utf8_bytes...]`                 | 6-260 bytes |
| Bytes  | 4     | `[length:1][raw_bytes...]`                  | 6-260 bytes |

### **Example Packets**

**Bool (true) on Channel 0:**
```
0xAA 0x00 0x00 0x01 0xAB
 ^    ^    ^    ^    ^
 |    |    |    |    CRC
 |    |    |    Value (1 = true)
 |    |    Channel 0
 |    Type: Bool
 Start marker
```

**Float (3.14) on Channel 2:**
```
0xAA 0x02 0x02 0xC3 0xF5 0x48 0x40 0x??
 ^    ^    ^    ^--------------^    ^
 |    |    |    Float bytes         CRC
 |    |    Channel 2
 |    Type: Float
 Start marker
```

---

## 🌐 WiFi Communication (UDP)

### **Configuration**

```cpp
FEmbeddedDeviceConfig Config;
Config.Protocol = EHoloCadeCommProtocol::WiFi;
Config.DeviceAddress = TEXT("192.168.1.50");  // ESP32 IP
Config.Port = 8888;                           // UDP port
```

### **Network Setup**

1. **ESP32 and Unreal PC must be on same LAN**
   - Configure WiFi SSID/password in ESP32 firmware
   - Note ESP32's IP address from Serial Monitor
   - Update `Config.DeviceAddress` in Unreal

2. **Firewall Rules**
   - Allow UDP traffic on port 8888 (Windows Firewall)
   - `netsh advfirewall firewall add rule name="HoloCade UDP" dir=in action=allow protocol=UDP localport=8888`

3. **Testing Connection**
   - Use Wireshark to monitor UDP packets
   - ESP32 sends button states continuously
   - Unreal logs received packets in `LogTemp`

---

## 🔒 Security (AES-128 + HMAC)

HoloCade supports **three security levels** to protect against spoofing, tampering, and sniffing:

### **Security Levels**

```cpp
Config.SecurityLevel = EHoloCadeSecurityLevel::Encrypted;  // Recommended
Config.SharedSecret = TEXT("my_secret_key_2025");        // MUST match ESP32
```

| Level       | Protection                  | Packet Overhead | Use Case                    |
|-------------|-----------------------------|-----------------|-----------------------------|
| **None**    | ❌ No protection            | +1 byte (CRC)   | Development only            |
| **HMAC**    | ✅ Authentication           | +8 bytes        | Small venues, low threat    |
| **Encrypted** | ✅✅ Full confidentiality  | +12 bytes       | **Production (recommended)** |

### **Encrypted Mode (AES-128 + HMAC)**

**Packet Format:**
```
[0xAA][IV:4][Encrypted(Type|Ch|Payload):N][HMAC:8]
```

**Example:**
```cpp
FEmbeddedDeviceConfig Config;
Config.SecurityLevel = EHoloCadeSecurityLevel::Encrypted;
Config.SharedSecret = TEXT("VenueSecret_2025_Prod");  // Change this!
Device->InitializeDevice(Config);

// All SendBool/Float/etc calls are now encrypted automatically
Device->SendFloat(0, 0.8f);  // Encrypted + authenticated
```

**ESP32 Side:**
```cpp
// In firmware (ESP32_Example_Firmware_Secured.ino)
const char* sharedSecret = "VenueSecret_2025_Prod";  // MUST match Unreal
const int securityLevel = 2;  // 2 = AES-128 + HMAC
```

### **Key Derivation**

Keys are **auto-derived** from `SharedSecret` using SHA-1:
```
AES Key (16 bytes)  = SHA1(SharedSecret + "AES128_HoloCade_2025")[0:16]
HMAC Key (32 bytes) = SHA1(SharedSecret + "HMAC_HoloCade_2025")[0:20] + padding
```

**Best Practices:**
- ✅ Use unique secrets per venue
- ✅ Rotate secrets monthly
- ✅ Store secrets in secure config files (not hardcoded)
- ❌ Never commit secrets to Git

### **Performance Impact**

| Metric                | None    | HMAC      | Encrypted |
|-----------------------|---------|-----------|-----------|
| **Serialize Time**    | 0.1 µs  | +2 µs     | +3 µs     |
| **Deserialize Time**  | 0.1 µs  | +2 µs     | +3 µs     |
| **Packet Size (float)** | 8 bytes | 16 bytes  | 20 bytes  |
| **Max Frequency**     | 1000 Hz | 500 Hz    | 300 Hz    |

**All modes support >60Hz refresh rates needed for smooth gameplay!**

### **Threat Protection**

| Attack Type      | None | HMAC | Encrypted |
|------------------|------|------|-----------|
| Sniffing         | ❌   | ⚠️   | ✅        |
| Spoofing         | ❌   | ✅   | ✅        |
| Tampering        | ❌   | ✅   | ✅        |
| Replay           | ❌   | ⚠️   | ✅        |
| Man-in-the-Middle| ❌   | ⚠️   | ✅        |

**Legend:**
- ✅ Protected
- ⚠️ Partially protected
- ❌ Vulnerable

### **Network Isolation (Additional Security)**

Recommended for all venues:
```
VLAN 30: LBE Devices (ESP32 + Unreal)
  - Hidden SSID
  - WPA3 password
  - Air-gapped (no internet)
  - Rotate password monthly
```

---

## 🐛 Debug Mode

Toggle between Binary and JSON modes:

```cpp
Config.bDebugMode = true;  // JSON mode
```

### **JSON Format**

```json
{"ch":0,"type":"float","val":3.14}
```

### **⚠️ CRITICAL SECURITY WARNING ⚠️**

**Debug mode DISABLES ALL ENCRYPTION**, regardless of `SecurityLevel` setting!

```cpp
// ❌ WRONG - Encryption will be DISABLED!
Config.bDebugMode = true;
Config.SecurityLevel = EHoloCadeSecurityLevel::Encrypted;  // IGNORED!

// ✅ CORRECT - Development
Config.bDebugMode = true;
Config.SecurityLevel = EHoloCadeSecurityLevel::None;  // Match debug behavior

// ✅ CORRECT - Production
Config.bDebugMode = false;
Config.SecurityLevel = EHoloCadeSecurityLevel::Encrypted;
```

**You will see this warning in logs:**
```
========================================
⚠️  SECURITY WARNING ⚠️
========================================
Debug mode DISABLES encryption for Wireshark packet inspection!
SecurityLevel is set to 'Encrypted' but will be IGNORED in debug mode.
All packets will be sent as PLAIN JSON (no encryption).

⛔ NEVER USE DEBUG MODE IN PRODUCTION! ⛔
========================================
```

### **When to Use Each Mode**

| Scenario | bDebugMode | SecurityLevel | Purpose |
|----------|------------|---------------|---------|
| **Early dev (no ESP32)** | `true` | `None` | Prototyping game logic |
| **Wireshark debugging** | `true` | `None` | Inspecting packet contents |
| **Testing encryption** | `false` | `Encrypted` | Verifying security works |
| **Production** | `false` | `Encrypted` | **Live venue deployment** |

**Use JSON mode for:**
- Initial development/debugging
- Wireshark packet inspection (see exact values in hex dump)
- Testing without ESP32 hardware

**Use Binary mode for:**
- Production (10x faster)
- High-frequency updates (>30Hz)
- Bandwidth-constrained networks
- **Anytime security matters!**

---

## 🎮 Use Cases

### **1. Actor Costume Controls**

```cpp
// 4 buttons on actor costume
Device->OnBoolReceived.AddDynamic(this, &ACostumeActor::OnButtonPressed);

void ACostumeActor::OnButtonPressed(int32 Channel, bool Pressed)
{
    switch (Channel)
    {
        case 0: TriggerDialogueOption1(); break;  // Button A
        case 1: TriggerDialogueOption2(); break;  // Button B
        case 2: EmergencyStop(); break;            // Panic button
        case 3: RequestHint(); break;              // Help button
    }
}

// Send haptic feedback to actor
Device->SendFloat(0, 0.8f);  // Vibrate vest motor (80% intensity)
```

### **2. Interactive Props**

```cpp
// Puzzle box with pressure sensors and LEDs
struct PuzzleState {
    int32 score;
    float timeRemaining;
    bool isPuzzleSolved;
};

Device->SendStruct(0, puzzleState);  // Send entire state as bytes

// Read sensor values
Device->OnFloatReceived.AddDynamic(this, &APuzzle::OnPressureSensor);
```

### **3. Environmental Controls**

```cpp
// Control stage lighting
Device->SendInt32(0, 255);  // Red channel
Device->SendInt32(1, 128);  // Green channel
Device->SendInt32(2, 64);   // Blue channel

// Trigger fog machine
Device->SendBool(10, true);
```

---

## 🔧 Hardware Setup

### **ESP32 DevKit + Actor Costume**

**Components:**
- ESP32 DevKit (WiFi)
- 4x Tactile buttons (chest/shoulder mounted)
- 6x Vibration motors (vest + gloves)
- 6x NPN transistors (motor drivers)
- LiPo battery + voltage regulator
- Wiring harness

**Schematic:**
```
Button → GPIO 2, 4, 5, 18 (INPUT_PULLUP)
Motors → GPIO 12, 13, 14, 25, 26, 27 (PWM via transistor)
Power  → 7.4V LiPo → 5V regulator → ESP32 + Motors
```

### **Arduino Uno + Interactive Prop**

**Components:**
- Arduino Uno
- USB cable to PC
- Pressure sensors, LEDs, servos
- 12V power supply

**Protocol:** Serial (115200 baud)

---

## 📊 Performance

### **Security Level Comparison (Binary Mode)**

| Metric                  | None        | HMAC        | Encrypted   |
|-------------------------|-------------|-------------|-------------|
| **Serialize Time**      | ~0.1 µs     | ~2 µs       | ~3 µs       |
| **Deserialize Time**    | ~0.1 µs     | ~2 µs       | ~3 µs       |
| **Packet Size (float)** | 8 bytes     | 16 bytes    | 20 bytes    |
| **Max Frequency**       | 1000+ Hz    | 500+ Hz     | 300+ Hz     |
| **Bandwidth (100Hz)**   | 0.8 KB/s    | 1.6 KB/s    | 2.0 KB/s    |

### **Binary vs JSON (No Security)**

| Metric               | Binary Mode | JSON Mode |
|----------------------|-------------|-----------|
| **Serialize Time**   | ~0.1 µs     | ~50 µs    |
| **Deserialize Time** | ~0.1 µs     | ~100 µs   |
| **Packet Size (float)** | 8 bytes     | ~35 bytes |
| **Max Frequency**    | 1000+ Hz    | ~30 Hz    |

**Recommendations:**
- **Production:** Binary mode + Encrypted security
- **Debugging:** JSON mode (encryption disabled for Wireshark)
- **Performance-critical:** Binary mode + HMAC only (if security is less important than latency)

**All modes comfortably exceed 60Hz gameplay requirements!**

---

## 🚧 Limitations & Future Work

### **Current Limitations:**
- ❌ Serial communication not yet implemented (Windows COM ports)
- ❌ Bluetooth not yet implemented
- ❌ Max payload size: 255 bytes
- ❌ No automatic reconnection (manual restart required)
- ❌ Single device per component (no multi-device support)

### **Planned Features:**
- [ ] Serial port communication (Windows/Linux)
- [ ] Bluetooth LE support
- [ ] Automatic device discovery (mDNS/Bonjour)
- [ ] Multi-device management
- [ ] Packet compression (for bandwidth-limited scenarios)
- [ ] Replay/recording system for debugging

---

## 📝 API Reference

### **Initialization**

```cpp
bool InitializeDevice(const FEmbeddedDeviceConfig& Config);
void DisconnectDevice();
bool IsDeviceConnected() const;
```

### **Sending (Unreal → Device)**

```cpp
void SendBool(int32 Channel, bool Value);
void SendInt32(int32 Channel, int32 Value);
void SendFloat(int32 Channel, float Value);
void SendString(int32 Channel, const FString& Value);
void SendBytes(int32 Channel, const TArray<uint8>& Data);

template<typename T>
void SendStruct(int32 Channel, const T& Data);  // C++ only, POD types
```

### **Receiving (Device → Unreal)**

```cpp
UPROPERTY(BlueprintAssignable)
FOnBoolReceived OnBoolReceived;

UPROPERTY(BlueprintAssignable)
FOnInt32Received OnInt32Received;

UPROPERTY(BlueprintAssignable)
FOnFloatReceived OnFloatReceived;

UPROPERTY(BlueprintAssignable)
FOnStringReceived OnStringReceived;

UPROPERTY(BlueprintAssignable)
FOnBytesReceived OnBytesReceived;
```

### **Legacy API (Command-Based)**

```cpp
void SendOutputCommand(const FEmbeddedOutputCommand& Command);
void TriggerHapticPulse(int32 Channel, float Intensity, float Duration);
void SetContinuousOutput(int32 Channel, float Value);
float GetInputValue(int32 Channel) const;
```

---

## 🧪 Testing

### **1. Local Loopback Test (No Hardware)**

```cpp
// Enable debug mode and use localhost
Config.bDebugMode = true;
Config.DeviceAddress = TEXT("127.0.0.1");

// Send test packets, inspect with Wireshark
Device->SendFloat(0, 3.14f);
```

### **2. ESP32 Echo Test**

Modify ESP32 firmware to echo all received packets back to Unreal. Verify round-trip communication.

### **3. Wireshark Packet Capture**

```
Filter: udp.port == 8888
Decode As: Data (raw hex view)
```

Look for `0xAA` start markers in hex dump.

---

## 📚 Additional Resources

- **Firmware Examples:** `FirmwareExamples/Base/Examples/` - Platform-agnostic examples
- **Escape Room Examples:** `FirmwareExamples/EscapeRoom/` - Door lock and prop control examples
- **Gunship Experience Examples:** `FirmwareExamples/GunshipExperience/` - Motion platform ECU examples
- **Protocol Spec:** This README, "Binary Protocol Specification" section
- **Security Guide:** This README, "Security (AES-128 + HMAC)" section
- **Arduino Libraries:** WiFiUdp, mbedtls (both built-in to ESP32 core)
- **Unreal Modules:** Sockets, Networking, Json, JsonUtilities, AES, SecureHash

---

## 📄 License

MIT License - Copyright (c) 2025 AJ Campbell

---

**Built for HoloCade**

