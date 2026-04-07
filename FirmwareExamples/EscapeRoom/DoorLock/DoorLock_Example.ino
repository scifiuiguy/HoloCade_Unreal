/*
 * HoloCade Door Lock Example Firmware
 * 
 * Example firmware for controlling door locks/latches in escape room installations.
 * Demonstrates wireless communication with Unreal Engine using HoloCade protocol.
 * 
 * Functionality:
 * - Receives unlock/lock commands from Unreal Engine via wireless communication
 * - Controls solenoid locks or servo motors for door latches
 * - Reads door sensors (magnetic reed switches) to detect door state
 * - Provides status LED feedback for lock state
 * - Supports multiple doors (configurable, default: 4)
 * 
 * Supported Platforms:
 * - ESP32 (built-in WiFi) - GPIO pins: 2, 4, 5, 12, 13, 14, 18, 26, 27, 32, 33
 * - ESP8266 (built-in WiFi) - GPIO pins: 2, 4, 5, 12, 13, 14, 15, 16
 * - Arduino + WiFi Shield (ESP8266-based) - Standard Arduino GPIO pins
 * - STM32 + WiFi Module (ESP8266/ESP32-based) - STM32 GPIO pins
 * - Raspberry Pi (built-in WiFi) - GPIO via WiringPi or pigpio
 * - Jetson Nano (built-in WiFi) - GPIO via Jetson GPIO library
 * 
 * Hardware Requirements:
 * - Microcontroller with WiFi capability (see supported platforms above)
 * - Solenoid lock or servo motor for door latch (one per door)
 * - Optional: Door sensor (magnetic reed switch) to detect lock state
 * - Optional: LED indicator for lock status
 * 
 * Wiring (example for ESP32):
 * - Solenoid/Servo: GPIO 12 (PWM for servo, digital for solenoid)
 * - Door Sensor: GPIO 13 (INPUT_PULLUP)
 * - Status LED: GPIO 14 (OUTPUT)
 * 
 * Note: GPIO pin assignments vary by platform. Adjust pin numbers in the
 * Configuration section below to match your hardware.
 * 
 * This example uses the HoloCade_Wireless_RX.h template for receiving unlock commands.
 * Copy HoloCade_Wireless_RX.h from FirmwareExamples/Base/Templates/ to your sketch directory.
 * 
 * Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.
 */

#include "HoloCade_Wireless_RX.h"

// Forward declare TX functions (we'll implement them using the RX UDP object)
void HoloCade_SendBool_TX(uint8_t channel, bool value);
uint8_t HoloCade_CalculateCRC_TX(uint8_t* data, int length);

// =====================================
// Configuration
// =====================================

// WiFi credentials (change to match your LAN)
const char* ssid = "VR_Arcade_LAN";
const char* password = "your_password_here";

// Door lock configuration
// Note: GPIO pin assignments vary by platform. For ESP8266, use pins: {12, 13, 14, 15} for locks
// ESP8266 has fewer GPIO pins - avoid GPIO 0, 15, 16 for some functions
const int NUM_DOORS = 4;
const int lockPins[NUM_DOORS] = {12, 14, 27, 26};  // GPIO pins for locks (ESP32 example)
const int sensorPins[NUM_DOORS] = {13, 15, 32, 33}; // GPIO pins for door sensors (ESP32 example)
const int ledPins[NUM_DOORS] = {2, 4, 5, 18};       // GPIO pins for status LEDs (ESP32 example)

// Lock state tracking
bool doorUnlocked[NUM_DOORS] = {false, false, false, false};
bool lastSensorState[NUM_DOORS] = {false, false, false, false};

// =====================================
// Setup
// =====================================

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\nHoloCade Door Lock Controller");
  Serial.println("============================\n");
  
  // Configure lock pins (servo or solenoid control)
  for (int i = 0; i < NUM_DOORS; i++) {
    pinMode(lockPins[i], OUTPUT);
    digitalWrite(lockPins[i], LOW);  // Start locked
  }
  
  // Configure sensor pins (magnetic reed switches)
  for (int i = 0; i < NUM_DOORS; i++) {
    pinMode(sensorPins[i], INPUT_PULLUP);
    lastSensorState[i] = digitalRead(sensorPins[i]) == LOW;
  }
  
  // Configure LED pins
  for (int i = 0; i < NUM_DOORS; i++) {
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], LOW);  // Start with LED off (locked)
  }
  
  // Initialize wireless communication (RX for receiving commands)
  // This sets up WiFi and UDP listening
  HoloCade_Wireless_Init(ssid, password, 8888);
  
  Serial.println("\nDoor Lock Controller Ready!");
  Serial.println("Waiting for commands from Unreal Engine...\n");
}

// =====================================
// Main Loop
// =====================================

void loop() {
  // Process incoming commands from Unreal
  HoloCade_ProcessIncoming();
  
  // Read door sensors and report state changes
  readDoorSensors();
  
  delay(10); // ~100Hz polling
}

// =====================================
// HoloCade Command Handlers
// =====================================

void HoloCade_HandleBool(uint8_t channel, bool value) {
  if (channel >= NUM_DOORS) {
    Serial.printf("Invalid door channel: %d\n", channel);
    return;
  }
  
  // value = true means unlock, false means lock
  if (value) {
    unlockDoor(channel);
  } else {
    lockDoor(channel);
  }
}

void HoloCade_HandleFloat(uint8_t channel, float value) {
  // Float can be used for partial unlock (0.0 = locked, 1.0 = unlocked)
  // Or for servo position control
  if (channel >= NUM_DOORS) {
    return;
  }
  
  // Convert float to unlock state
  bool shouldUnlock = (value > 0.5f);
  
  if (shouldUnlock) {
    unlockDoor(channel);
  } else {
    lockDoor(channel);
  }
}

// =====================================
// Door Control Functions
// =====================================

void unlockDoor(int doorIndex) {
  if (doorIndex < 0 || doorIndex >= NUM_DOORS) return;
  
  // Activate lock mechanism (solenoid or servo)
  digitalWrite(lockPins[doorIndex], HIGH);
  
  // Update state
  doorUnlocked[doorIndex] = true;
  
  // Turn on status LED
  digitalWrite(ledPins[doorIndex], HIGH);
  
  Serial.printf("Door %d: UNLOCKED\n", doorIndex);
  
  // Send confirmation back to Unreal Engine
  // Send bool true on the same channel to confirm unlock
  HoloCade_SendBool_TX(doorIndex, true);
  Serial.printf("Door %d: Unlock confirmation sent to Unreal\n", doorIndex);
}

void lockDoor(int doorIndex) {
  if (doorIndex < 0 || doorIndex >= NUM_DOORS) return;
  
  // Deactivate lock mechanism
  digitalWrite(lockPins[doorIndex], LOW);
  
  // Update state
  doorUnlocked[doorIndex] = false;
  
  // Turn off status LED
  digitalWrite(ledPins[doorIndex], LOW);
  
  Serial.printf("Door %d: LOCKED\n", doorIndex);
  
  // Send confirmation back to Unreal Engine
  // Send bool false on the same channel to confirm lock
  HoloCade_SendBool_TX(doorIndex, false);
  Serial.printf("Door %d: Lock confirmation sent to Unreal\n", doorIndex);
}

// =====================================
// Sensor Reading
// =====================================

void readDoorSensors() {
  for (int i = 0; i < NUM_DOORS; i++) {
    // Read sensor (LOW = door closed, HIGH = door open)
    bool sensorState = (digitalRead(sensorPins[i]) == LOW);
    
    // Report state changes
    if (sensorState != lastSensorState[i]) {
      lastSensorState[i] = sensorState;
      
      if (sensorState) {
        Serial.printf("Door %d: CLOSED (sensor)\n", i);
      } else {
        Serial.printf("Door %d: OPEN (sensor)\n", i);
      }
    }
  }
}

// =====================================
// TX Functions (using RX UDP object)
// =====================================

// Unreal PC IP (change to your Unreal PC's IP)
IPAddress unrealIP(192, 168, 1, 100);
uint16_t unrealPort = 8888;

uint8_t HoloCade_CalculateCRC_TX(uint8_t* data, int length) {
  uint8_t crc = 0;
  for (int i = 0; i < length; i++) {
    crc ^= data[i];
  }
  return crc;
}

void HoloCade_SendBool_TX(uint8_t channel, bool value) {
  if (!HoloCade_Initialized) return;
  
  uint8_t packet[5];
  packet[0] = 0xAA;  // PACKET_START_MARKER
  packet[1] = 0;     // TYPE_BOOL
  packet[2] = channel;
  packet[3] = value ? 1 : 0;
  packet[4] = HoloCade_CalculateCRC_TX(packet, 4);
  
  // Use the RX UDP object to send (it's already initialized)
  HoloCade_UDP.beginPacket(unrealIP, unrealPort);
  HoloCade_UDP.write(packet, 5);
  HoloCade_UDP.endPacket();
}

