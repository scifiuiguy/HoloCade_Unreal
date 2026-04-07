/*
 * HoloCade Flight Sim Experience ECU
 * 
 * Embedded control unit for FlightSimExperience 2DOF gyroscope system.
 * Controls two servo motors for continuous pitch and roll rotation.
 * 
 * This ECU receives motion commands from Unreal/Unity FlightSimExperience
 * and controls servo motors for continuous rotation (no limit switches).
 * 
 * Functionality:
 * - Receives gyroscope rotation commands (pitch and roll, unlimited degrees)
 * - Controls two servo motors (pitch axis and roll axis)
 * - Continuous rotation support (can exceed 360° in either direction)
 * - Emergency stop and return to neutral functions
 * - Position feedback to Unreal (bidirectional IO)
 * 
 * Supported Platforms:
 * - ESP32 (built-in WiFi) - Recommended for this application
 * - ESP8266 (built-in WiFi) - Limited GPIO, may need additional hardware
 * - Arduino + WiFi Shield (ESP8266-based) - Standard Arduino GPIO pins
 * - STM32 + WiFi Module (ESP8266/ESP32-based) - STM32 GPIO pins
 * - Raspberry Pi (built-in WiFi) - GPIO via WiringPi or pigpio
 * - Jetson Nano (built-in WiFi) - GPIO via Jetson GPIO library
 * 
 * Hardware Requirements:
 * - Microcontroller with WiFi capability (see supported platforms above)
 * - Two continuous rotation servo motors (or standard servos with continuous rotation modification)
 * - Optional: Position encoders for feedback (if using modified servos)
 * - Servo motor drivers (PWM capable pins)
 * - Power supply for servo motors (separate from microcontroller power)
 * 
 * Protocol: Binary HoloCade protocol
 * Channel Mapping (matches FGyroState from Unreal):
 * - Channel 0: Pitch (float, degrees - unlimited, can exceed 360°)
 * - Channel 1: Roll (float, degrees - unlimited, can exceed 360°)
 * - Channel 4: Duration (float, seconds) - time to reach target
 * - Channel 7: Emergency stop (bool, true = stop all systems)
 * - Channel 8: Return to neutral (bool, true = return to 0° pitch and roll)
 * 
 * Struct Packets:
 * - Channel 102: FGyroState struct (pitch and roll only)
 * 
 * DOF Mapping:
 * - Servo 0: Pitch axis (continuous rotation)
 * - Servo 1: Roll axis (continuous rotation)
 * - Total: 2 DOF continuous rotation gyroscope
 * 
 * This example uses the HoloCade_Wireless_RX.h template for receiving commands.
 * Copy HoloCade_Wireless_RX.h from FirmwareExamples/Base/Templates/ to your sketch directory.
 * 
 * Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.
 */

#include "HoloCade_Wireless_RX.h"
#include "../Base/Templates/HoloCade_Wireless_TX.h"
#include "../Base/Templates/GyroscopeController.h"
#include "../Base/Templates/Yaskawa_Sigma5_Drive.h"  // Default servo drive implementation

// Struct definitions matching Unreal (must match exactly for binary compatibility)
struct FGyroState {
  float Pitch;  // degrees (unlimited - continuous rotation)
  float Roll;   // degrees (unlimited - continuous rotation)
};

// =====================================
// Configuration
// =====================================

// WiFi credentials (change to match your LAN)
const char* ssid = "VR_Arcade_LAN";
const char* password = "your_password_here";

// Unreal Engine PC IP address (for bidirectional IO feedback)
IPAddress unrealIP(192, 168, 1, 100);
uint16_t unrealPort = 8888;

// Create gyroscope controller instance
GyroscopeController gyroController;

// Unified state
float motionDuration = 1.0f; // Default duration in seconds
unsigned long motionStartTime = 0;
bool motionInProgress = false;

// Gravity reset parameters (received from Unreal on connect)
bool gravityResetEnabled = false;
float resetSpeed = 30.0f;  // Default reset speed (degrees per second)
float resetIdleTimeout = 5.0f;  // Default idle timeout (seconds)

// =====================================
// Setup
// =====================================

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\nHoloCade Flight Sim Experience ECU");
  Serial.println("==================================\n");
  
  // Configure gyroscope controller with professional servo drives
  GyroscopeConfig gyroConfig;
  gyroConfig.servoDriveType = ServoDriveType::YaskawaSigma5;  // Default: Yaskawa Sigma-5
  gyroConfig.maxRotationSpeedDegreesPerSecond = 90.0f;  // Maximum rotation speed
  gyroConfig.smoothingFactor = 0.2f;  // Smoothing factor (0.0-1.0): 0.2 = smooth but responsive
  gyroConfig.enableGravityReset = false;  // Will be set from Unreal on connect
  gyroConfig.resetSpeed = 30.0f;  // Will be set from Unreal on connect
  gyroConfig.resetIdleTimeout = 5.0f;  // Will be set from Unreal on connect
  
  // Configure pitch axis servo drive
  gyroConfig.pitchDriveConfig.nodeId = 1;  // MECHATROLINK station number or EtherCAT node ID
  gyroConfig.pitchDriveConfig.maxVelocity = 90.0f;  // degrees per second
  gyroConfig.pitchDriveConfig.maxAcceleration = 180.0f;  // degrees per second squared
  gyroConfig.pitchDriveConfig.maxTorque = 2.4f;  // Nm (for Sigma-5 500W motor)
  gyroConfig.pitchDriveConfig.useAbsoluteEncoder = true;
  gyroConfig.pitchDriveConfig.encoderResolution = 20;  // 20-bit encoder for Sigma-5
  gyroConfig.pitchDriveConfig.enableBrake = false;
  gyroConfig.pitchDriveConfig.enableSoftLimits = false;  // Continuous rotation - no limits
  
  // Configure roll axis servo drive
  gyroConfig.rollDriveConfig.nodeId = 2;  // MECHATROLINK station number or EtherCAT node ID
  gyroConfig.rollDriveConfig.maxVelocity = 90.0f;
  gyroConfig.rollDriveConfig.maxAcceleration = 180.0f;
  gyroConfig.rollDriveConfig.maxTorque = 2.4f;
  gyroConfig.rollDriveConfig.useAbsoluteEncoder = true;
  gyroConfig.rollDriveConfig.encoderResolution = 20;
  gyroConfig.rollDriveConfig.enableBrake = false;
  gyroConfig.rollDriveConfig.enableSoftLimits = false;
  
  // Initialize gyroscope controller
  if (!gyroController.begin(gyroConfig)) {
    Serial.println("ERROR: Failed to initialize gyroscope controller!");
    while(1) delay(1000);  // Halt on error
  }
  
  // Initialize wireless communication (RX for receiving commands)
  HoloCade_Wireless_Init(ssid, password, 8888);
  
  // Initialize TX for sending feedback to Unreal
  HoloCade_Wireless_TX_Init(ssid, password, unrealIP, unrealPort);
  
  motionStartTime = millis();
  
  Serial.println("\nFlight Sim Experience ECU Ready!");
  Serial.println("Waiting for gyroscope commands from Unreal/Unity...\n");
  Serial.println("Channel Mapping:");
  Serial.println("  Ch 0: Pitch (degrees, unlimited)");
  Serial.println("  Ch 1: Roll (degrees, unlimited)");
  Serial.println("  Ch 4: Duration (seconds)");
  Serial.println("  Ch 7: Emergency Stop (bool)");
  Serial.println("  Ch 8: Return to Neutral (bool)");
  Serial.println("  Ch 9: Gravity Reset Enable (bool, sent on connect)");
  Serial.println("  Ch 10: Reset Speed (float, deg/s, sent on connect)");
  Serial.println("  Ch 11: Reset Idle Timeout (float, seconds, sent on connect)");
  Serial.println("  Ch 102: FGyroState struct (pitch and roll)\n");
}

// =====================================
// Main Loop
// =====================================

void loop() {
  // Process incoming HoloCade commands
  HoloCade_ProcessIncoming();
  
  // Update gyroscope controller
  gyroController.update();
  
  // Send position feedback to Unreal (bidirectional IO) - every 100ms (10 Hz)
  static unsigned long lastFeedbackTime = 0;
  if (millis() - lastFeedbackTime >= 100) {
    SendPositionFeedback();
    lastFeedbackTime = millis();
  }
  
  delay(10); // Control loop update rate (~100 Hz)
}

// =====================================
// HoloCade Command Handlers
// =====================================

void HoloCade_HandleFloat(uint8_t channel, float value) {
  switch (channel) {
    case 0:
      // Channel 0: Pitch (degrees, unlimited - continuous rotation)
      gyroController.setTargetPitch(value);
      Serial.printf("ECU: Pitch = %.2f°\n", value);
      break;
      
    case 1:
      // Channel 1: Roll (degrees, unlimited - continuous rotation)
      gyroController.setTargetRoll(value);
      Serial.printf("ECU: Roll = %.2f°\n", value);
      break;
      
    case 4:
      // Channel 4: Duration (seconds)
      motionDuration = max(value, 0.1f);
      motionStartTime = millis();
      motionInProgress = true;
      Serial.printf("ECU: Motion duration = %.2f seconds\n", motionDuration);
      break;
      
    case 10:
      // Channel 10: Reset speed (degrees per second, sent from Unreal on connect)
      resetSpeed = value;
      gyroController.setResetSpeed(value);
      Serial.printf("ECU: Reset speed = %.2f deg/s\n", value);
      break;
      
    case 11:
      // Channel 11: Reset idle timeout (seconds, sent from Unreal on connect)
      resetIdleTimeout = value;
      gyroController.setResetIdleTimeout(value);
      Serial.printf("ECU: Reset idle timeout = %.2f seconds\n", value);
      break;
  }
}

void HoloCade_HandleBool(uint8_t channel, bool value) {
  if (channel == 7) {
    // Channel 7: Emergency stop
    gyroController.setEmergencyStop(value);
    if (value) {
      Serial.println("ECU: EMERGENCY STOP ACTIVATED");
    } else {
      Serial.println("ECU: Emergency stop released");
    }
  } else if (channel == 8) {
    // Channel 8: Return to neutral (0° pitch and roll)
    if (value) {
      gyroController.returnToNeutral();
      Serial.println("ECU: Returning to neutral position (0° pitch, 0° roll)");
    }
  } else if (channel == 9) {
    // Channel 9: Gravity reset enable (sent from Unreal on connect)
    gravityResetEnabled = value;
    gyroController.setGravityResetEnabled(value);
    Serial.printf("ECU: Gravity reset %s\n", value ? "enabled" : "disabled");
  }
}

// =====================================
// Struct Packet Handlers (MVC Pattern)
// =====================================

void HoloCade_HandleBytes(uint8_t channel, uint8_t* data, uint8_t length) {
  switch (channel) {
    case 102: {
      // Channel 102: FGyroState struct (pitch and roll only)
      if (length >= sizeof(FGyroState)) {
        FGyroState* gyroState = (FGyroState*)data;
        gyroController.setTargetPitch(gyroState->Pitch);
        gyroController.setTargetRoll(gyroState->Roll);
        Serial.printf("ECU: Gyro struct - Pitch: %.2f°, Roll: %.2f°\n", 
          gyroState->Pitch, gyroState->Roll);
      }
      break;
    }
    
    default:
      Serial.printf("ECU: Unknown struct channel: %d\n", channel);
      break;
  }
}

// =====================================
// Bidirectional IO: Position Feedback
// =====================================

void SendPositionFeedback() {
  // Get current positions from controller
  float currentPitch = gyroController.getCurrentPitch();
  float currentRoll = gyroController.getCurrentRoll();
  
  // Send gyroscope state feedback (Channel 102)
  FGyroState gyroFeedback;
  gyroFeedback.Pitch = currentPitch;
  gyroFeedback.Roll = currentRoll;
  HoloCade_SendBytes(102, (uint8_t*)&gyroFeedback, sizeof(FGyroState));
}

