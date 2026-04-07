/*
 * HoloCade Scissor Lift Controller - Standalone Example
 * 
 * Standalone embedded module for controlling electric scissor lifts in motion platforms.
 * This example uses the ScissorLift_Controller.h header.
 * 
 * Supports two modes:
 * 1. CAN Bus Mode (default): For Genie/Skyjack and other manufacturer ECUs with CAN interfaces
 * 2. Direct GPIO Mode: For custom builds or testing (uncomment GPIO config section)
 * 
 * For use in combined systems (e.g., GunshipExperience), include the header directly.
 * 
 * Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.
 */

#include "HoloCade_Wireless_RX.h"
#include "../Templates/ScissorLift_Controller.h"

// WiFi credentials (change to match your LAN)
const char* ssid = "VR_Arcade_LAN";
const char* password = "your_password_here";

// Create controller instance
ScissorLiftController liftController;

// =====================================
// Setup
// =====================================

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\nHoloCade Scissor Lift Controller");
  Serial.println("================================\n");
  
  // Configure scissor lift
  ScissorLiftConfig config;
  
  // =====================================
  // CAN BUS MODE (Default - for Genie/Skyjack ECUs)
  // =====================================
  config.useCANBus = true;                    // Enable CAN bus mode
  config.canBaudRate = 500000;                // CAN bus baud rate (125000, 250000, or 500000)
  config.canIdJoystick = 0x180;               // CAN ID for joystick commands (manufacturer-specific)
  config.canIdControl = 0x200;                // CAN ID for control commands (E-stop, etc.)
  config.canIdFeedback = 0x280;               // CAN ID for position feedback (if available)
  config.canCSPin = 10;                       // MCP2515 CS pin (only for MCP2515, default: 10)
  config.useCANFeedback = false;              // Set to true if ECU provides position feedback via CAN
  
  // Position sensor (GPIO analog input - used if useCANFeedback = false)
  config.positionSensorPin = 14;              // GPIO pin for position sensor (use -1 if using CAN feedback)
  config.topLimitPin = 15;                     // GPIO pin for top limit switch (optional, use -1 to disable)
  config.bottomLimitPin = 16;               // GPIO pin for bottom limit switch (optional, use -1 to disable)
  
  // =====================================
  // DIRECT GPIO MODE (Alternative - for custom builds)
  // =====================================
  // Uncomment this section and set useCANBus = false to use direct GPIO control:
  /*
  config.useCANBus = false;                   // Disable CAN bus mode
  config.motorUpPin = 12;                     // GPIO pin for lift up
  config.motorDownPin = 13;                   // GPIO pin for lift down
  config.motorForwardPin = 17;                // GPIO pin for forward drive
  config.motorReversePin = 18;                // GPIO pin for reverse drive
  config.positionSensorPin = 14;             // GPIO pin for position sensor
  config.topLimitPin = 15;                    // GPIO pin for top limit switch
  config.bottomLimitPin = 16;                 // GPIO pin for bottom limit switch
  config.forwardLimitPin = 19;                // GPIO pin for forward limit switch
  config.reverseLimitPin = 20;               // GPIO pin for reverse limit switch
  */
  
  // Forward/reverse drive configuration (optional)
  config.enableForwardReverse = true;         // Set to false if bolted to floor
  config.maxForwardReverseCm = 90.0f;         // Maximum forward/reverse travel in cm (safety limit)
  
  // Motion parameters
  config.maxHeightCm = 300.0f;                // Maximum lift height in cm
  config.minHeightCm = 0.0f;                  // Minimum lift height in cm
  config.softwareUpperLimitCm = 90.0f;        // Software-defined upper limit in cm (virtual limit)
  config.maxSpeedCmPerSec = 10.0f;            // Maximum lift speed (cm/second)
  
  // Operation mode
  config.autoCalibrateMode = true;            // true = auto-calibrate mode, false = fixed mode
  config.autoCalibrateTimeoutMs = 2000;       // Timeout for auto-calibrate mode
  
  // Initialize controller
  liftController.begin(config);
  
  // Initialize wireless communication
  HoloCade_Wireless_Init(ssid, password, 8888);
  
  Serial.println("\nScissor Lift Controller Ready!");
  Serial.println("Waiting for commands from game engine...\n");
  Serial.println("NOTICE: Lift must be calibrated before use.");
  Serial.println("Send calibration command (Channel 2 = true) to enter calibration mode.\n");
  
  if (config.useCANBus) {
    Serial.println("CAN Bus Mode: Configure CAN IDs to match your manufacturer's ECU protocol.");
    Serial.println("NOTE: CAN bus protocol documentation is typically proprietary.");
    Serial.println("NOTE: No open-source projects found documenting Genie/Skyjack CAN protocols.");
    Serial.println("Options:");
    Serial.println("  1. Contact manufacturer support for protocol documentation");
    Serial.println("  2. Use CAN bus analyzer to reverse-engineer protocol (likely required)");
    Serial.println("     - Recommended: CANtact ($50-100), Peak PCAN-USB ($200-300)");
    Serial.println("     - Splice into CAN wires, monitor traffic while using OEM joystick");
    Serial.println("     - Correlate joystick movements with CAN IDs to identify functions");
    Serial.println("  3. Check service manuals for wiring/ECU information");
    Serial.println("Default CAN IDs (0x180, 0x200, 0x280) are examples - replace with actual IDs.");
    Serial.println("You will likely need to reverse-engineer the protocol yourself.\n");
  }
}

// =====================================
// Main Loop
// =====================================

void loop() {
  // Process incoming HoloCade commands
  HoloCade_ProcessIncoming();
  
  // Update controller
  liftController.update();
  
  delay(10); // Small delay for stability
}

// =====================================
// HoloCade Command Handlers
// =====================================

void HoloCade_HandleFloat(uint8_t channel, float value) {
  liftController.handleFloatCommand(channel, value);
}

void HoloCade_HandleBool(uint8_t channel, bool value) {
  liftController.handleBoolCommand(channel, value);
}

