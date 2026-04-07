/*
 * HoloCade Button & Motor Example Firmware
 * 
 * This example demonstrates bidirectional communication between
 * Unreal Engine (HoloCade SDK) and a microcontroller over WiFi (UDP).
 * 
 * Functionality:
 * - Reads 4 tactile buttons and sends state changes to Unreal
 * - Receives motor control commands from Unreal and drives 6 vibration motors
 * - Demonstrates basic input/output integration with HoloCade EmbeddedSystems API
 * 
 * Supported Platforms:
 * - ESP32 (built-in WiFi) - GPIO pins: 2, 4, 5, 12, 13, 14, 18, 25, 26, 27
 * - ESP8266 (built-in WiFi) - GPIO pins: 2, 4, 5, 12, 13, 14, 16
 * - Arduino + WiFi Shield (ESP8266-based) - Standard Arduino GPIO pins
 * - STM32 + WiFi Module (ESP8266/ESP32-based) - STM32 GPIO pins
 * - Raspberry Pi (built-in WiFi) - GPIO via WiringPi or pigpio
 * - Jetson Nano (built-in WiFi) - GPIO via Jetson GPIO library
 * 
 * Hardware Requirements:
 * - Microcontroller with WiFi capability (see supported platforms above)
 * - 4 tactile buttons connected to GPIO pins (with pull-up resistors)
 * - 6 vibration motors connected to GPIO pins (with driver transistors)
 * 
 * Protocol: Binary (matches HoloCade EmbeddedDeviceController)
 * Packet Format: [0xAA][Type][Channel][Payload...][CRC]
 * 
 * Note: GPIO pin assignments vary by platform. Adjust pin numbers in the
 * Configuration section below to match your hardware.
 * 
 * Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.
 */

#include <WiFi.h>
#include <WiFiUdp.h>

// =====================================
// Configuration
// =====================================

// WiFi credentials (change to match your LAN)
const char* ssid = "VR_Arcade_LAN";
const char* password = "your_password_here";

// Unreal PC IP and port
IPAddress unrealIP(192, 168, 1, 100);  // Change to your Unreal PC's IP
uint16_t unrealPort = 8888;
uint16_t localPort = 8888;

// Button pins (INPUT_PULLUP)
// Note: GPIO pin assignments vary by platform. For ESP8266, use pins: {2, 4, 5, 16}
// ESP8266 has fewer GPIO pins - avoid GPIO 0, 15, 16 for some functions
const int buttonPins[4] = {2, 4, 5, 18};  // ESP32 example
bool buttonStates[4] = {false, false, false, false};
bool lastButtonStates[4] = {false, false, false, false};

// Vibration motor pins (PWM)
// Note: For ESP8266, use pins: {12, 13, 14, 15, 0, 1}
const int motorPins[6] = {12, 13, 14, 25, 26, 27};  // ESP32 example

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

// =====================================
// Setup
// =====================================

void setup() {
  Serial.begin(115200);
  Serial.println("\n\nHoloCade Button & Motor Example Starting...");

  // Configure button pins
  for (int i = 0; i < 4; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
  }

  // Configure motor pins
  for (int i = 0; i < 6; i++) {
    pinMode(motorPins[i], OUTPUT);
    digitalWrite(motorPins[i], LOW);
  }

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

  Serial.println("Button & Motor Example ready!");
}

// =====================================
// Main Loop
// =====================================

void loop() {
  // Read button states and send to Unreal
  readButtons();

  // Receive commands from Unreal
  receiveCommands();

  delay(10); // ~100Hz polling
}

// =====================================
// Button Reading
// =====================================

void readButtons() {
  for (int i = 0; i < 4; i++) {
    // Read button (active LOW due to pull-up)
    buttonStates[i] = (digitalRead(buttonPins[i]) == LOW);

    // Send on state change
    if (buttonStates[i] != lastButtonStates[i]) {
      sendBool(i, buttonStates[i]);
      Serial.printf("Button %d: %s\n", i, buttonStates[i] ? "PRESSED" : "RELEASED");
      lastButtonStates[i] = buttonStates[i];
    }
  }
}

// =====================================
// Command Receiving
// =====================================

void receiveCommands() {
  int packetSize = udp.parsePacket();
  if (packetSize == 0) return;

  // Read packet
  uint8_t buffer[256];
  int len = udp.read(buffer, 256);

  if (len < 5) {
    Serial.println("Packet too small");
    return;
  }

  // Validate start marker
  if (buffer[0] != PACKET_START_MARKER) {
    Serial.printf("Invalid start marker: 0x%02X\n", buffer[0]);
    return;
  }

  // Validate CRC
  uint8_t receivedCRC = buffer[len - 1];
  uint8_t calculatedCRC = calculateCRC(buffer, len - 1);
  if (receivedCRC != calculatedCRC) {
    Serial.println("CRC mismatch");
    return;
  }

  // Parse packet
  uint8_t type = buffer[1];
  uint8_t channel = buffer[2];

  switch (type) {
    case TYPE_BOOL:
      handleBool(channel, buffer[3] != 0);
      break;

    case TYPE_INT32:
      handleInt32(channel, *(int32_t*)&buffer[3]);
      break;

    case TYPE_FLOAT:
      handleFloat(channel, *(float*)&buffer[3]);
      break;

    case TYPE_STRING:
      handleString(channel, &buffer[4], buffer[3]);
      break;

    default:
      Serial.printf("Unknown type: %d\n", type);
      break;
  }
}

// =====================================
// Command Handlers
// =====================================

void handleBool(uint8_t channel, bool value) {
  Serial.printf("Received Bool - Ch:%d Val:%s\n", channel, value ? "true" : "false");
  // Example: Control LED based on bool
  if (channel < 6) {
    digitalWrite(motorPins[channel], value ? HIGH : LOW);
  }
}

void handleInt32(uint8_t channel, int32_t value) {
  Serial.printf("Received Int32 - Ch:%d Val:%d\n", channel, value);
  // Example: Set PWM duty cycle based on int (0-255)
  if (channel < 6) {
    analogWrite(motorPins[channel], constrain(value, 0, 255));
  }
}

void handleFloat(uint8_t channel, float value) {
  Serial.printf("Received Float - Ch:%d Val:%.3f\n", channel, value);
  // Example: Set motor intensity (0.0-1.0)
  if (channel < 6) {
    int pwm = (int)(value * 255.0f);
    analogWrite(motorPins[channel], constrain(pwm, 0, 255));
  }
}

void handleString(uint8_t channel, uint8_t* data, uint8_t length) {
  // Convert to null-terminated string
  char str[256];
  memcpy(str, data, length);
  str[length] = '\0';
  Serial.printf("Received String - Ch:%d Val:%s\n", channel, str);
}

// =====================================
// Sending to Unreal
// =====================================

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

void sendInt32(uint8_t channel, int32_t value) {
  uint8_t packet[8];
  packet[0] = PACKET_START_MARKER;
  packet[1] = TYPE_INT32;
  packet[2] = channel;
  packet[3] = (value) & 0xFF;
  packet[4] = (value >> 8) & 0xFF;
  packet[5] = (value >> 16) & 0xFF;
  packet[6] = (value >> 24) & 0xFF;
  packet[7] = calculateCRC(packet, 7);

  udp.beginPacket(unrealIP, unrealPort);
  udp.write(packet, 8);
  udp.endPacket();
}

void sendFloat(uint8_t channel, float value) {
  uint8_t packet[8];
  packet[0] = PACKET_START_MARKER;
  packet[1] = TYPE_FLOAT;
  packet[2] = channel;
  
  // Reinterpret float as uint32 for byte-by-byte transmission
  uint32_t intValue = *(uint32_t*)&value;
  packet[3] = (intValue) & 0xFF;
  packet[4] = (intValue >> 8) & 0xFF;
  packet[5] = (intValue >> 16) & 0xFF;
  packet[6] = (intValue >> 24) & 0xFF;
  packet[7] = calculateCRC(packet, 7);

  udp.beginPacket(unrealIP, unrealPort);
  udp.write(packet, 8);
  udp.endPacket();
}

void sendString(uint8_t channel, const char* str) {
  uint8_t len = strlen(str);
  if (len > 255) len = 255;

  uint8_t packet[256];
  packet[0] = PACKET_START_MARKER;
  packet[1] = TYPE_STRING;
  packet[2] = channel;
  packet[3] = len;
  memcpy(&packet[4], str, len);
  packet[4 + len] = calculateCRC(packet, 4 + len);

  udp.beginPacket(unrealIP, unrealPort);
  udp.write(packet, 5 + len);
  udp.endPacket();
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

