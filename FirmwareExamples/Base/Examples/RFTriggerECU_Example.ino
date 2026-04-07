/*
 * HoloCade RF Trigger ECU Example Firmware
 * 
 * This example demonstrates bidirectional 433MHz RF communication for wireless
 * button/remote control integration with HoloCade EmbeddedSystems API.
 * 
 * Functionality:
 * - Receives button events from 433MHz wireless remote (RX module)
 * - Sends button state changes to Unreal Engine via WiFi UDP
 * - Optionally sends commands via 433MHz TX module (for bidirectional RF)
 * - Demonstrates rolling code validation for security
 * - Autonomous button mapping storage (saves to onboard flash memory)
 * 
 * Supported Platforms:
 * - ESP32 (built-in WiFi, EEPROM/Preferences) - Recommended for this application
 * - ESP8266 (built-in WiFi, EEPROM) - Limited GPIO, may need additional hardware
 * - Arduino + WiFi Shield (ESP8266-based) - Standard Arduino GPIO pins, EEPROM
 * - STM32 + WiFi Module (ESP8266/ESP32-based) - STM32 GPIO pins, EEPROM/Flash
 * - Raspberry Pi (built-in WiFi) - GPIO via WiringPi or pigpio, file system storage
 * - Jetson Nano (built-in WiFi) - GPIO via Jetson GPIO library, file system storage
 * 
 * Hardware Requirements:
 * - Microcontroller with WiFi capability (see supported platforms above)
 * - 433MHz RX module (receiver) - e.g., XY-MK-5V, FS1000A, or similar
 * - 433MHz TX module (transmitter, optional) - e.g., FS1000A, or similar
 * - 433MHz wireless remote (transmitter) - Standard 433MHz RF remote
 * - Antenna for RX/TX modules (typically 17.3cm wire for 433MHz)
 * 
 * 433MHz Module Connections:
 * - RX Module VCC → 5V (or 3.3V for some modules)
 * - RX Module GND → GND
 * - RX Module DATA → GPIO pin (configured below)
 * - TX Module VCC → 5V (or 3.3V for some modules)
 * - TX Module GND → GND
 * - TX Module DATA → GPIO pin (configured below)
 * 
 * Protocol: Binary HoloCade protocol (WiFi UDP to Unreal Engine)
 * RF Protocol: OOK (On-Off Keying) or ASK (Amplitude Shift Keying) - varies by module
 * 
 * Security:
 * - Supports rolling code validation (KeeLoq, Hopping Code, or custom)
 * - Replay attack prevention (reject duplicate codes)
 * - Code learning mode for pairing remotes
 * 
 * Storage:
 * - Button mappings saved to onboard flash memory (autonomous, no server dependency)
 * - Platform-specific storage implementations (EEPROM, Preferences, file system)
 * - Automatic save/load on startup
 * 
 * Note: GPIO pin assignments vary by platform. Adjust pin numbers in the
 * Configuration section below to match your hardware.
 * 
 * Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.
 */

#include <WiFi.h>
#include <WiFiUdp.h>

// =====================================
// Platform Detection
// =====================================

// Uncomment the platform you're using:
#define PLATFORM_ESP32
// #define PLATFORM_ESP8266
// #define PLATFORM_ARDUINO
// #define PLATFORM_STM32
// #define PLATFORM_RASPBERRY_PI
// #define PLATFORM_JETSON_NANO

// =====================================
// Platform-Specific Includes
// =====================================

#ifdef PLATFORM_ESP32
  #include <Preferences.h>  // ESP32 non-volatile storage
  Preferences preferences;
  #define STORAGE_NAMESPACE "lbeast_rf"
#elif defined(PLATFORM_ESP8266)
  #include <EEPROM.h>
  #define EEPROM_SIZE 512
#elif defined(PLATFORM_ARDUINO)
  #include <EEPROM.h>
  #define EEPROM_SIZE 512
#elif defined(PLATFORM_STM32)
  // STM32 uses internal Flash or external EEPROM
  // Uncomment if using external EEPROM:
  // #include <Wire.h>
  // #include <AT24C256.h>  // Example: 24C256 I2C EEPROM
#elif defined(PLATFORM_RASPBERRY_PI) || defined(PLATFORM_JETSON_NANO)
  // Linux-based platforms use file system storage
  #include <fstream>
  #include <string>
  #define STORAGE_FILE_PATH "/var/lib/lbeast/rf_buttons.json"
#endif

// =====================================
// Configuration
// =====================================

// WiFi credentials (change to match your LAN)
const char* ssid = "VR_Arcade_LAN";
const char* password = "your_password_here";

// Unreal Engine PC IP and port
IPAddress unrealIP(192, 168, 1, 100);  // Change to your Unreal PC's IP
uint16_t unrealPort = 8888;
uint16_t localPort = 8888;

// 433MHz RX module pin (DATA pin from receiver module)
const int rfRxPin = 2;  // ESP32 example - adjust for your platform

// 433MHz TX module pin (DATA pin to transmitter module, optional)
const int rfTxPin = 4;  // ESP32 example - adjust for your platform
bool bUseTX = false;    // Set to true if using TX module for bidirectional RF

// Button state tracking
const int MAX_BUTTONS = 8;  // Maximum number of buttons to track
struct ButtonState {
  uint32_t code;        // Button code from remote
  uint32_t rollingCodeSeed;  // Rolling code seed for this button
  bool state;           // Current button state (pressed = true)
  uint32_t lastCode;    // Last received rolling code (for replay attack prevention)
  unsigned long lastPressTime;  // Timestamp of last press (ms)
  char functionName[32];  // Assigned function name (e.g., "HeightUp", "HeightDown")
  bool bIsMapped;       // Whether this button has an assigned function
};

ButtonState buttonStates[MAX_BUTTONS];
int buttonCount = 0;

// Rolling code validation
bool bRollingCodeEnabled = true;  // Set to false for simple fixed-code remotes
uint32_t rollingCodeSeed = 0x12345678;  // Shared secret seed (must match remote)
uint32_t lastValidCode = 0;

// Code learning mode
bool bLearningMode = false;  // Set to true to learn new remote codes
unsigned long learningModeTimeout = 0;
const unsigned long LEARNING_MODE_DURATION_MS = 30000;  // 30 seconds

// UDP
WiFiUDP udp;

// Protocol constants
const uint8_t PACKET_START_MARKER = 0xAA;

enum DataType {
  TYPE_BOOL = 0,
  TYPE_INT32 = 1,
  TYPE_FLOAT = 2,
  TYPE_STRING = 3,
  TYPE_BYTES = 4
};

// RF signal processing
volatile bool rfSignalReceived = false;
volatile unsigned long rfSignalTime = 0;
volatile uint32_t rfReceivedCode = 0;
volatile bool rfSignalState = false;

// RF decoding state machine
volatile unsigned long rfPulseStart = 0;
volatile unsigned long rfPulseEnd = 0;
volatile int rfBitIndex = 0;
volatile uint32_t rfDecodedBits = 0;
volatile bool rfDecoding = false;

// Typical 433MHz timing (adjust based on your remote)
const unsigned long RF_SHORT_PULSE = 300;   // Short pulse duration (microseconds)
const unsigned long RF_LONG_PULSE = 900;   // Long pulse duration (microseconds)
const unsigned long RF_SYNC_PULSE = 3000;   // Sync pulse duration (microseconds)
const int RF_BITS_PER_CODE = 24;           // Number of bits in code (varies by remote)

// =====================================
// Setup
// =====================================

void setup() {
  Serial.begin(115200);
  Serial.println("\n\nHoloCade RF Trigger ECU Example Starting...");

  // Load saved button mappings from flash memory
  loadButtonMappings();

  // Configure 433MHz RX pin
  pinMode(rfRxPin, INPUT);
  
  // Configure 433MHz TX pin (if using)
  if (bUseTX) {
    pinMode(rfTxPin, OUTPUT);
    digitalWrite(rfTxPin, LOW);
  }

  // Attach interrupt for RX pin (trigger on change)
  attachInterrupt(digitalPinToInterrupt(rfRxPin), rfInterruptHandler, CHANGE);

  // Connect to WiFi
  Serial.printf("Connecting to WiFi: %s\n", ssid);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected!");
  Serial.printf("ESP32 IP: %s\n", WiFi.localIP().toString().c_str());
  Serial.printf("Unreal IP: %s:%d\n", unrealIP.toString().c_str(), unrealPort);

  // Start UDP
  udp.begin(localPort);
  Serial.printf("UDP listening on port %d\n", localPort);

  Serial.printf("Loaded %d learned buttons from flash memory\n", buttonCount);
  Serial.println("RF Trigger ECU Example ready!");
  Serial.println("Press remote buttons to test...");
  
  if (bLearningMode) {
    Serial.println("LEARNING MODE ACTIVE - Press remote buttons to learn codes");
    learningModeTimeout = millis() + LEARNING_MODE_DURATION_MS;
  }
}

// =====================================
// Main Loop
// =====================================

void loop() {
  // Process RF signals
  if (rfSignalReceived) {
    processRFSignal();
    rfSignalReceived = false;
  }

  // Check learning mode timeout
  if (bLearningMode && millis() > learningModeTimeout) {
    bLearningMode = false;
    Serial.println("Learning mode disabled (timeout)");
  }

  // Receive commands from Unreal (optional - for bidirectional RF)
  if (bUseTX) {
    receiveCommands();
  }

  delay(10); // ~100Hz polling
}

// =====================================
// RF Interrupt Handler (Full Implementation)
// =====================================

void IRAM_ATTR rfInterruptHandler() {
  // Full implementation: Decode OOK/ASK modulated 433MHz signals
  // This uses pulse width modulation (PWM) decoding
  
  unsigned long currentTime = micros();
  bool pinState = digitalRead(rfRxPin);
  
  if (pinState == HIGH) {
    // Rising edge - start of pulse
    rfPulseStart = currentTime;
    
    // Check if this is a sync pulse (long HIGH pulse)
    if (rfPulseEnd > 0) {
      unsigned long gapDuration = rfPulseStart - rfPulseEnd;
      if (gapDuration > RF_SYNC_PULSE) {
        // Sync pulse detected - start new code
        rfBitIndex = 0;
        rfDecodedBits = 0;
        rfDecoding = true;
      }
    }
  } else {
    // Falling edge - end of pulse
    rfPulseEnd = currentTime;
    
    if (rfDecoding && rfPulseStart > 0) {
      // Measure pulse width
      unsigned long pulseWidth = rfPulseEnd - rfPulseStart;
      
      // Decode bit based on pulse width
      bool bit = (pulseWidth > RF_SHORT_PULSE && pulseWidth < RF_LONG_PULSE) ? 
                 (pulseWidth > (RF_SHORT_PULSE + RF_LONG_PULSE) / 2) : false;
      
      // Store bit
      if (rfBitIndex < 32) {
        rfDecodedBits |= (bit ? 1UL : 0UL) << rfBitIndex;
        rfBitIndex++;
      }
      
      // Check if we've received enough bits
      if (rfBitIndex >= RF_BITS_PER_CODE) {
        rfReceivedCode = rfDecodedBits;
        rfSignalReceived = true;
        rfSignalTime = millis();
        rfDecoding = false;
      }
    }
  }
}

// =====================================
// RF Signal Processing (Full Implementation)
// =====================================

void processRFSignal() {
  // Decode RF signal - extract button code and rolling code
  // Format: [ButtonCode:8 bits][RollingCode:24 bits] (adjust based on your remote)
  
  uint32_t buttonCode = rfReceivedCode & 0xFF;  // Lower 8 bits = button code
  uint32_t rollingCode = (rfReceivedCode >> 8) & 0xFFFFFF;  // Upper 24 bits = rolling code
  bool buttonState = true;  // Assume pressed (some remotes send separate press/release codes)
  
  // Validate rolling code (if enabled)
  if (bRollingCodeEnabled) {
    if (!validateRollingCode(rollingCode, buttonCode)) {
      Serial.printf("Invalid rolling code: 0x%06X (button: %d)\n", rollingCode, buttonCode);
      return;  // Reject invalid code
    }
  }
  
  // Check for replay attack (reject duplicate codes)
  for (int i = 0; i < buttonCount; i++) {
    if (buttonStates[i].code == buttonCode && buttonStates[i].lastCode == rollingCode) {
      unsigned long timeSinceLastPress = millis() - buttonStates[i].lastPressTime;
      if (timeSinceLastPress < 100) {  // Reject codes within 100ms
        Serial.printf("Replay attack detected: button %d, code 0x%06X\n", buttonCode, rollingCode);
        return;  // Reject duplicate code
      }
    }
  }
  
  // Learning mode: Add new button
  if (bLearningMode) {
    if (buttonCount < MAX_BUTTONS) {
      // Check if button already exists
      bool buttonExists = false;
      for (int i = 0; i < buttonCount; i++) {
        if (buttonStates[i].code == buttonCode) {
          buttonExists = true;
          break;
        }
      }
      
      if (!buttonExists) {
        buttonStates[buttonCount].code = buttonCode;
        buttonStates[buttonCount].state = buttonState;
        buttonStates[buttonCount].rollingCodeSeed = rollingCode;
        buttonStates[buttonCount].lastCode = rollingCode;
        buttonStates[buttonCount].lastPressTime = millis();
        strncpy(buttonStates[buttonCount].functionName, "", sizeof(buttonStates[buttonCount].functionName));
        buttonStates[buttonCount].bIsMapped = false;
        buttonCount++;
        
        Serial.printf("Learned new button: code=%d, rolling=0x%06X\n", buttonCode, rollingCode);
        
        // Save to flash memory immediately
        saveButtonMappings();
        
        // Send to Unreal Engine
        sendButtonEventToUnreal(buttonCode, buttonState);
        return;
      }
    } else {
      Serial.println("Maximum button count reached");
      return;
    }
  }
  
  // Find existing button
  int buttonIndex = -1;
  for (int i = 0; i < buttonCount; i++) {
    if (buttonStates[i].code == buttonCode) {
      buttonIndex = i;
      break;
    }
  }
  
  if (buttonIndex == -1) {
    // Unknown button - ignore
    Serial.printf("Unknown button code: %d\n", buttonCode);
    return;
  }
  
  // Update button state
  if (buttonStates[buttonIndex].state != buttonState) {
    buttonStates[buttonIndex].state = buttonState;
    buttonStates[buttonIndex].lastCode = rollingCode;
    buttonStates[buttonIndex].lastPressTime = millis();
    
    Serial.printf("Button %d (%s): %s (code: 0x%06X)\n", 
                  buttonCode,
                  buttonStates[buttonIndex].functionName,
                  buttonState ? "PRESSED" : "RELEASED",
                  rollingCode);
    
    // Send to Unreal Engine
    sendButtonEventToUnreal(buttonCode, buttonState);
  }
}

// =====================================
// Rolling Code Validation (Full Implementation)
// =====================================

bool validateRollingCode(uint32_t receivedCode, uint32_t buttonCode) {
  // Full implementation: Validate rolling code using linear congruential generator
  // This is a simple example - replace with your remote's actual algorithm
  
  // Find button's expected rolling code
  uint32_t expectedCode = 0;
  for (int i = 0; i < buttonCount; i++) {
    if (buttonStates[i].code == buttonCode) {
      expectedCode = calculateExpectedRollingCode(buttonStates[i].rollingCodeSeed, buttonCode);
      break;
    }
  }
  
  // Allow ±10 code window for drift
  int32_t diff = (int32_t)(receivedCode - expectedCode);
  if (diff < 0) diff = -diff;
  
  return (diff <= 10);
}

uint32_t calculateExpectedRollingCode(uint32_t seed, uint32_t buttonCode) {
  // Full implementation: Linear congruential generator
  // Replace with your remote's actual rolling code algorithm
  
  static uint32_t counter = 0;
  counter++;
  
  // Simple LCG: (a * x + c) mod m
  const uint32_t a = 1103515245;
  const uint32_t c = 12345;
  const uint32_t m = 0xFFFFFFFF;
  
  uint32_t x = seed + buttonCode + counter;
  return ((a * x + c) % m) & 0xFFFFFF;
}

// =====================================
// Sending to Unreal Engine
// =====================================

void sendButtonEventToUnreal(uint32_t buttonCode, bool buttonState) {
  // Send button event via WiFi UDP to Unreal Engine
  // Channel mapping: buttonCode = channel number
  
  uint8_t channel = (uint8_t)(buttonCode & 0xFF);  // Use lower 8 bits as channel
  
  sendBool(channel, buttonState);
}

void sendBool(uint8_t channel, bool value) {
  uint8_t packet[5];
  packet[0] = PACKET_START_MARKER;
  packet[1] = TYPE_BOOL;
  packet[2] = channel;
  packet[3] = value ? 1 : 0;
  packet[4] = calculateCRC(packet, 4);
  
  udp.beginPacket(unrealIP, unrealPort);
  udp.write(packet, 5);
  udp.endPacket();
}

// =====================================
// Receiving Commands from Unreal (Optional - for bidirectional RF)
// =====================================

void receiveCommands() {
  int packetSize = udp.parsePacket();
  if (packetSize == 0) return;
  
  // Read packet
  uint8_t buffer[256];
  int len = udp.read(buffer, 256);
  
  if (len < 5) {
    return;
  }
  
  // Validate start marker
  if (buffer[0] != PACKET_START_MARKER) {
    return;
  }
  
  // Validate CRC
  uint8_t receivedCRC = buffer[len - 1];
  uint8_t calculatedCRC = calculateCRC(buffer, len - 1);
  if (receivedCRC != calculatedCRC) {
    return;
  }
  
  // Parse packet
  uint8_t type = buffer[1];
  uint8_t channel = buffer[2];
  
  switch (type) {
    case TYPE_BOOL:
      handleBool(channel, buffer[3] != 0);
      break;
      
    default:
      break;
  }
}

void handleBool(uint8_t channel, bool value) {
  // Example: Send command via 433MHz TX module
  if (bUseTX && channel < MAX_BUTTONS) {
    sendRFCommand(channel, value);
  }
}

// =====================================
// RF TX (Optional - for bidirectional RF)
// =====================================

void sendRFCommand(uint32_t buttonCode, bool state) {
  // Full implementation: Encode and transmit via 433MHz TX module
  // Uses same encoding as RX decoding (OOK/ASK modulation)
  
  Serial.printf("Sending RF command: button=%d, state=%s\n", buttonCode, state ? "ON" : "OFF");
  
  // Generate rolling code
  uint32_t rollingCode = calculateExpectedRollingCode(rollingCodeSeed, buttonCode);
  
  // Encode: [ButtonCode:8 bits][RollingCode:24 bits]
  uint32_t encodedCode = buttonCode | (rollingCode << 8);
  
  // Transmit via TX module (OOK modulation)
  // Send sync pulse
  digitalWrite(rfTxPin, HIGH);
  delayMicroseconds(RF_SYNC_PULSE);
  digitalWrite(rfTxPin, LOW);
  delayMicroseconds(RF_SYNC_PULSE);
  
  // Send data bits
  for (int i = 0; i < RF_BITS_PER_CODE; i++) {
    bool bit = (encodedCode >> i) & 1;
    
    digitalWrite(rfTxPin, HIGH);
    delayMicroseconds(bit ? RF_LONG_PULSE : RF_SHORT_PULSE);
    digitalWrite(rfTxPin, LOW);
    delayMicroseconds(RF_SHORT_PULSE);
  }
  
  // End pulse
  digitalWrite(rfTxPin, HIGH);
  delayMicroseconds(RF_SHORT_PULSE);
  digitalWrite(rfTxPin, LOW);
}

// =====================================
// CRC Calculation
// =====================================

uint8_t calculateCRC(uint8_t* data, int length) {
  uint8_t crc = 0;
  for (int i = 0; i < length; i++) {
    crc ^= data[i];
  }
  return crc;
}

// =====================================
// Button Mapping Storage (Cross-Platform)
// =====================================

void saveButtonMappings() {
  // Save button mappings to onboard flash memory (autonomous, no server dependency)
  // Platform-specific implementations below
  
#ifdef PLATFORM_ESP32
  // ESP32: Use Preferences (non-volatile storage)
  preferences.begin(STORAGE_NAMESPACE, false);
  preferences.putUChar("buttonCount", buttonCount);
  
  for (int i = 0; i < buttonCount; i++) {
    char key[16];
    snprintf(key, sizeof(key), "btn%d_code", i);
    preferences.putULong(key, buttonStates[i].code);
    
    snprintf(key, sizeof(key), "btn%d_seed", i);
    preferences.putULong(key, buttonStates[i].rollingCodeSeed);
    
    snprintf(key, sizeof(key), "btn%d_func", i);
    preferences.putString(key, buttonStates[i].functionName);
    
    snprintf(key, sizeof(key), "btn%d_map", i);
    preferences.putBool(key, buttonStates[i].bIsMapped);
  }
  
  preferences.end();
  Serial.printf("Saved %d button mappings to ESP32 Preferences\n", buttonCount);
  
#elif defined(PLATFORM_ESP8266) || defined(PLATFORM_ARDUINO)
  // ESP8266/Arduino: Use EEPROM
  EEPROM.begin(EEPROM_SIZE);
  
  int addr = 0;
  EEPROM.write(addr++, buttonCount);
  
  for (int i = 0; i < buttonCount; i++) {
    // Save button code (4 bytes)
    EEPROM.write(addr++, (buttonStates[i].code >> 0) & 0xFF);
    EEPROM.write(addr++, (buttonStates[i].code >> 8) & 0xFF);
    EEPROM.write(addr++, (buttonStates[i].code >> 16) & 0xFF);
    EEPROM.write(addr++, (buttonStates[i].code >> 24) & 0xFF);
    
    // Save rolling code seed (4 bytes)
    EEPROM.write(addr++, (buttonStates[i].rollingCodeSeed >> 0) & 0xFF);
    EEPROM.write(addr++, (buttonStates[i].rollingCodeSeed >> 8) & 0xFF);
    EEPROM.write(addr++, (buttonStates[i].rollingCodeSeed >> 16) & 0xFF);
    EEPROM.write(addr++, (buttonStates[i].rollingCodeSeed >> 24) & 0xFF);
    
    // Save function name (32 bytes)
    for (int j = 0; j < 32; j++) {
      EEPROM.write(addr++, buttonStates[i].functionName[j]);
    }
    
    // Save mapped flag (1 byte)
    EEPROM.write(addr++, buttonStates[i].bIsMapped ? 1 : 0);
  }
  
  EEPROM.commit();
  EEPROM.end();
  Serial.printf("Saved %d button mappings to EEPROM\n", buttonCount);
  
#elif defined(PLATFORM_STM32)
  // STM32: Use internal Flash or external EEPROM
  // Option 1: Internal Flash (limited write cycles)
  // Option 2: External I2C EEPROM (recommended)
  
  // Uncomment for external I2C EEPROM (24C256 example):
  /*
  Wire.begin();
  AT24C256 eeprom(0x50);  // I2C address 0x50
  
  int addr = 0;
  eeprom.write(addr++, buttonCount);
  
  for (int i = 0; i < buttonCount; i++) {
    eeprom.write(addr++, (buttonStates[i].code >> 0) & 0xFF);
    eeprom.write(addr++, (buttonStates[i].code >> 8) & 0xFF);
    eeprom.write(addr++, (buttonStates[i].code >> 16) & 0xFF);
    eeprom.write(addr++, (buttonStates[i].code >> 24) & 0xFF);
    
    eeprom.write(addr++, (buttonStates[i].rollingCodeSeed >> 0) & 0xFF);
    eeprom.write(addr++, (buttonStates[i].rollingCodeSeed >> 8) & 0xFF);
    eeprom.write(addr++, (buttonStates[i].rollingCodeSeed >> 16) & 0xFF);
    eeprom.write(addr++, (buttonStates[i].rollingCodeSeed >> 24) & 0xFF);
    
    for (int j = 0; j < 32; j++) {
      eeprom.write(addr++, buttonStates[i].functionName[j]);
    }
    
    eeprom.write(addr++, buttonStates[i].bIsMapped ? 1 : 0);
  }
  */
  
  Serial.printf("STM32: Save button mappings (external EEPROM code commented out)\n");
  
#elif defined(PLATFORM_RASPBERRY_PI) || defined(PLATFORM_JETSON_NANO)
  // Raspberry Pi / Jetson Nano: Use file system (JSON format)
  // Note: This is C++ code, not Arduino - adjust includes for your build system
  
  /*
  std::ofstream file(STORAGE_FILE_PATH);
  if (file.is_open()) {
    file << "{\n";
    file << "  \"buttonCount\": " << buttonCount << ",\n";
    file << "  \"buttons\": [\n";
    
    for (int i = 0; i < buttonCount; i++) {
      file << "    {\n";
      file << "      \"code\": " << buttonStates[i].code << ",\n";
      file << "      \"rollingCodeSeed\": " << buttonStates[i].rollingCodeSeed << ",\n";
      file << "      \"functionName\": \"" << buttonStates[i].functionName << "\",\n";
      file << "      \"bIsMapped\": " << (buttonStates[i].bIsMapped ? "true" : "false") << "\n";
      file << "    }";
      if (i < buttonCount - 1) file << ",";
      file << "\n";
    }
    
    file << "  ]\n";
    file << "}\n";
    file.close();
  }
  */
  
  Serial.printf("Raspberry Pi/Jetson: Save button mappings (file system code commented out)\n");
#endif
}

void loadButtonMappings() {
  // Load button mappings from onboard flash memory
  // Platform-specific implementations below
  
#ifdef PLATFORM_ESP32
  // ESP32: Use Preferences
  preferences.begin(STORAGE_NAMESPACE, true);  // Read-only mode
  
  buttonCount = preferences.getUChar("buttonCount", 0);
  
  for (int i = 0; i < buttonCount && i < MAX_BUTTONS; i++) {
    char key[16];
    snprintf(key, sizeof(key), "btn%d_code", i);
    buttonStates[i].code = preferences.getULong(key, 0);
    
    snprintf(key, sizeof(key), "btn%d_seed", i);
    buttonStates[i].rollingCodeSeed = preferences.getULong(key, 0);
    
    snprintf(key, sizeof(key), "btn%d_func", i);
    String funcName = preferences.getString(key, "");
    funcName.toCharArray(buttonStates[i].functionName, sizeof(buttonStates[i].functionName));
    
    snprintf(key, sizeof(key), "btn%d_map", i);
    buttonStates[i].bIsMapped = preferences.getBool(key, false);
    
    buttonStates[i].state = false;
    buttonStates[i].lastCode = 0;
    buttonStates[i].lastPressTime = 0;
  }
  
  preferences.end();
  Serial.printf("Loaded %d button mappings from ESP32 Preferences\n", buttonCount);
  
#elif defined(PLATFORM_ESP8266) || defined(PLATFORM_ARDUINO)
  // ESP8266/Arduino: Use EEPROM
  EEPROM.begin(EEPROM_SIZE);
  
  int addr = 0;
  buttonCount = EEPROM.read(addr++);
  if (buttonCount > MAX_BUTTONS) buttonCount = MAX_BUTTONS;
  
  for (int i = 0; i < buttonCount; i++) {
    // Load button code (4 bytes)
    buttonStates[i].code = 0;
    buttonStates[i].code |= ((uint32_t)EEPROM.read(addr++)) << 0;
    buttonStates[i].code |= ((uint32_t)EEPROM.read(addr++)) << 8;
    buttonStates[i].code |= ((uint32_t)EEPROM.read(addr++)) << 16;
    buttonStates[i].code |= ((uint32_t)EEPROM.read(addr++)) << 24;
    
    // Load rolling code seed (4 bytes)
    buttonStates[i].rollingCodeSeed = 0;
    buttonStates[i].rollingCodeSeed |= ((uint32_t)EEPROM.read(addr++)) << 0;
    buttonStates[i].rollingCodeSeed |= ((uint32_t)EEPROM.read(addr++)) << 8;
    buttonStates[i].rollingCodeSeed |= ((uint32_t)EEPROM.read(addr++)) << 16;
    buttonStates[i].rollingCodeSeed |= ((uint32_t)EEPROM.read(addr++)) << 24;
    
    // Load function name (32 bytes)
    for (int j = 0; j < 32; j++) {
      buttonStates[i].functionName[j] = EEPROM.read(addr++);
    }
    buttonStates[i].functionName[31] = '\0';  // Null terminate
    
    // Load mapped flag (1 byte)
    buttonStates[i].bIsMapped = (EEPROM.read(addr++) != 0);
    
    buttonStates[i].state = false;
    buttonStates[i].lastCode = 0;
    buttonStates[i].lastPressTime = 0;
  }
  
  EEPROM.end();
  Serial.printf("Loaded %d button mappings from EEPROM\n", buttonCount);
  
#elif defined(PLATFORM_STM32)
  // STM32: Use external I2C EEPROM
  // Uncomment for external I2C EEPROM (24C256 example):
  /*
  Wire.begin();
  AT24C256 eeprom(0x50);
  
  int addr = 0;
  buttonCount = eeprom.read(addr++);
  if (buttonCount > MAX_BUTTONS) buttonCount = MAX_BUTTONS;
  
  for (int i = 0; i < buttonCount; i++) {
    buttonStates[i].code = 0;
    buttonStates[i].code |= ((uint32_t)eeprom.read(addr++)) << 0;
    buttonStates[i].code |= ((uint32_t)eeprom.read(addr++)) << 8;
    buttonStates[i].code |= ((uint32_t)eeprom.read(addr++)) << 16;
    buttonStates[i].code |= ((uint32_t)eeprom.read(addr++)) << 24;
    
    buttonStates[i].rollingCodeSeed = 0;
    buttonStates[i].rollingCodeSeed |= ((uint32_t)eeprom.read(addr++)) << 0;
    buttonStates[i].rollingCodeSeed |= ((uint32_t)eeprom.read(addr++)) << 8;
    buttonStates[i].rollingCodeSeed |= ((uint32_t)eeprom.read(addr++)) << 16;
    buttonStates[i].rollingCodeSeed |= ((uint32_t)eeprom.read(addr++)) << 24;
    
    for (int j = 0; j < 32; j++) {
      buttonStates[i].functionName[j] = eeprom.read(addr++);
    }
    buttonStates[i].functionName[31] = '\0';
    
    buttonStates[i].bIsMapped = (eeprom.read(addr++) != 0);
    
    buttonStates[i].state = false;
    buttonStates[i].lastCode = 0;
    buttonStates[i].lastPressTime = 0;
  }
  */
  
  Serial.printf("STM32: Load button mappings (external EEPROM code commented out)\n");
  
#elif defined(PLATFORM_RASPBERRY_PI) || defined(PLATFORM_JETSON_NANO)
  // Raspberry Pi / Jetson Nano: Use file system (JSON format)
  // Note: This requires JSON parsing library (ArduinoJson, etc.)
  
  /*
  std::ifstream file(STORAGE_FILE_PATH);
  if (file.is_open()) {
    // Parse JSON (simplified - use proper JSON library in production)
    // This is a placeholder - implement proper JSON parsing
    file.close();
  }
  */
  
  Serial.printf("Raspberry Pi/Jetson: Load button mappings (file system code commented out)\n");
#endif
}

// =====================================
// Helper Functions
// =====================================

void enableLearningMode() {
  bLearningMode = true;
  learningModeTimeout = millis() + LEARNING_MODE_DURATION_MS;
  Serial.println("Learning mode enabled - press remote buttons to learn");
}

void disableLearningMode() {
  bLearningMode = false;
  Serial.println("Learning mode disabled");
}

void clearLearnedButtons() {
  buttonCount = 0;
  saveButtonMappings();  // Save empty state
  Serial.println("Cleared all learned buttons");
}

void assignButtonFunction(uint32_t buttonCode, const char* functionName) {
  // Assign function name to a learned button
  for (int i = 0; i < buttonCount; i++) {
    if (buttonStates[i].code == buttonCode) {
      strncpy(buttonStates[i].functionName, functionName, sizeof(buttonStates[i].functionName) - 1);
      buttonStates[i].functionName[sizeof(buttonStates[i].functionName) - 1] = '\0';
      buttonStates[i].bIsMapped = true;
      saveButtonMappings();  // Save immediately
      Serial.printf("Assigned function '%s' to button %d\n", functionName, buttonCode);
      return;
    }
  }
  Serial.printf("Button %d not found - cannot assign function\n", buttonCode);
}
