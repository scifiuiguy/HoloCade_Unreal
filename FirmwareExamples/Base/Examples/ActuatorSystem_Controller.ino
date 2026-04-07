/*
 * HoloCade Actuator System Controller - Standalone Example
 * 
 * Standalone embedded module for controlling 4-gang hydraulic actuator systems.
 * This example uses the ActuatorSystem_Controller.h header.
 * 
 * For use in combined systems (e.g., GunshipExperience), include the header directly.
 * 
 * Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.
 */

#include "HoloCade_Wireless_RX.h"
#include "../Templates/ActuatorSystem_Controller.h"

// WiFi credentials (change to match your LAN)
const char* ssid = "VR_Arcade_LAN";
const char* password = "your_password_here";

// Create controller instance
ActuatorSystemController actuatorController;

// =====================================
// Setup
// =====================================

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\nHoloCade Actuator System Controller");
  Serial.println("==================================\n");
  
  // Configure actuator system
  ActuatorSystemConfig config;
  config.valvePins[0] = 12;
  config.valvePins[1] = 13;
  config.valvePins[2] = 14;
  config.valvePins[3] = 15;
  config.sensorPins[0] = 32;
  config.sensorPins[1] = 33;
  config.sensorPins[2] = 34;
  config.sensorPins[3] = 35;
  config.lowerLimitPins[0] = 16;
  config.lowerLimitPins[1] = 17;
  config.lowerLimitPins[2] = 18;
  config.lowerLimitPins[3] = 19;
  config.upperLimitPins[0] = 20;
  config.upperLimitPins[1] = 21;
  config.upperLimitPins[2] = 22;
  config.upperLimitPins[3] = 23;
  
  config.maxPitchDeg = 10.0f;
  config.maxRollDeg = 10.0f;
  config.actuatorStrokeCm = 7.62f;
  config.platformWidthCm = 150.0f;
  config.platformLengthCm = 200.0f;
  
  config.kp = 2.0f;
  config.ki = 0.1f;
  config.kd = 0.5f;
  
  config.autoCalibrateMode = true;
  config.autoCalibrateTimeoutMs = 2000;
  
  // Initialize controller
  actuatorController.begin(config);
  
  // Initialize wireless communication
  HoloCade_Wireless_Init(ssid, password, 8888);
  
  Serial.println("\nActuator System Controller Ready!");
  Serial.println("Waiting for commands from game engine...\n");
  Serial.println("NOTICE: Actuators must be calibrated before use.");
  Serial.println("Send calibration command (Channel 2 = true) to enter calibration mode.\n");
}

// =====================================
// Main Loop
// =====================================

void loop() {
  // Process incoming HoloCade commands
  HoloCade_ProcessIncoming();
  
  // Update controller
  actuatorController.update();
  
  delay(10); // Control loop update rate (~100 Hz)
}

// =====================================
// HoloCade Command Handlers
// =====================================

void HoloCade_HandleFloat(uint8_t channel, float value) {
  actuatorController.handleFloatCommand(channel, value);
}

void HoloCade_HandleBool(uint8_t channel, bool value) {
  actuatorController.handleBoolCommand(channel, value);
}

