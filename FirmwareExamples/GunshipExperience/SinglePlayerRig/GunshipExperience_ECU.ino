/*
 * HoloCade Gunship Experience - Parent ECU (GunshipExperience_ECU)
 * 
 * Parent embedded control unit for GunshipExperience motion platform.
 * Uses the Universal Shield (primary ECU for the experience, usually named same as experience).
 * Integrates both scissor lift control and 4-gang actuator system control
 * into a single ECU for complete 4DOF motion platform control.
 * Interfaces with child ECUs (Gun_ECU) for per-station gun control.
 * 
 * This ECU uses the ActuatorSystem_Controller.h and ScissorLift_Controller.h headers
 * to eliminate code duplication and provide modular, reusable controllers.
 * 
 * This ECU receives motion commands from Unreal/Unity GunshipExperience
 * and coordinates both subsystems:
 * - Scissor Lift: Handles vertical translation (TranslationZ) and forward/reverse (TranslationY)
 * - Actuator System: Handles pitch and roll only (2 DOF)
 * 
 * Functionality:
 * - Receives complete motion commands (pitch, roll, forward/reverse, vertical) from game engine
 * - Coordinates scissor lift and actuator system simultaneously
 * - Calibration system for both subsystems
 * - Auto-calibrate mode (auto-revert to zero after timeout) vs Fixed mode (hold position)
 * - Provides unified emergency stop and safety functions with smooth interpolation
 * - Position feedback from both subsystems
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
 * - Electric scissor lift with motor control
 * - Optional: Forward/reverse drive motor (can be disabled if bolted to floor)
 * - 4-gang hydraulic actuator system with proportional valves
 * - Position sensors for all actuators and lift
 * - ESP32 custom PCB (see cost analysis) for valve drivers and sensor I/O
 * - Limit switches for safety (optional but recommended)
 * 
 * Protocol: Binary HoloCade protocol
 * 
 * Channel Mapping (Game Engine → Parent ECU):
 * - Channel 0: Pitch (float, degrees or normalized -1.0 to +1.0)
 * - Channel 1: Roll (float, degrees or normalized -1.0 to +1.0)
 * - Channel 2: TranslationY / Forward-Reverse (float, cm or normalized -1.0 to +1.0)
 * - Channel 3: TranslationZ / Vertical (float, cm or normalized -1.0 to +1.0)
 * - Channel 4: Duration (float, seconds) - time to reach target
 * - Channel 5: Calibration mode (bool, true = enter calibration mode)
 * - Channel 6: Operation mode (bool, true = auto-calibrate mode, false = fixed mode)
 * - Channel 7: Emergency stop (bool, true = stop all systems)
 * - Channel 8: Return to neutral (bool, true = return all to calibrated zero)
 * - Channel 9: Play session active (bool, true = guns can fire, false = guns disabled)
 * - Channel 100: Button event update interval (int32, ms) - server-controlled telemetry rate
 * - Channel 101: Telemetry update interval (int32, ms) - server-controlled telemetry rate
 * 
 * Channel Mapping (Parent ECU → Game Engine):
 * - Channel 100: FTiltState struct (pitch and roll feedback)
 * - Channel 101: FScissorLiftState struct (Y and Z translation feedback)
 * - Channel 310: FGunButtonEvents struct (all 4 stations, fast updates, default 20 Hz)
 *   * Contains: Button0State[4], Button1State[4], Timestamp
 *   * Update rate: Configurable via Channel 100 (default 50ms = 20 Hz)
 * - Channel 311: FGunTelemetry struct (all 4 stations, slow updates, default 1 Hz)
 *   * Contains: Temperatures, solenoid state, fire state, system state
 *   * Update rate: Configurable via Channel 101 (default 1000ms = 1 Hz)
 * 
 * DOF Mapping:
 * - Actuators: Pitch and Roll only (2 DOF, yaw restricted)
 * - Scissor Lift: Vertical (TranslationZ) and Forward/Reverse (TranslationY) (2 DOF)
 * - Total: 4 DOF motion platform
 * 
 * This example uses the HoloCade_Wireless_RX.h template for receiving commands.
 * Copy HoloCade_Wireless_RX.h from FirmwareExamples/Base/Templates/ to your sketch directory.
 * 
 * Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.
 */

#include "HoloCade_Wireless_RX.h"
#include "../Base/Templates/HoloCade_Wireless_TX.h"
#include "../Base/Templates/ActuatorSystem_Controller.h"
#include "../Base/Templates/ScissorLift_Controller.h"

// Struct definitions matching Unreal (must match exactly for binary compatibility)
struct FTiltState {
  float Pitch;  // degrees
  float Roll;    // degrees
};

struct FScissorLiftState {
  float TranslationY;  // cm (forward/reverse)
  float TranslationZ;  // cm (up/down)
};

struct FPlatformMotionCommand {
  float Pitch;          // degrees
  float Roll;           // degrees
  float TranslationY;   // cm
  float TranslationZ;   // cm
  float Duration;       // seconds
};

// Child ECU state struct (internal storage, received from each child ECU)
struct FGunECUState {
  // Button states
  bool Button0State;           // Left thumb button (pressed = true)
  bool Button1State;           // Right thumb button (pressed = true)
  bool Button0LastState;       // Previous state for change detection
  bool Button1LastState;       // Previous state for change detection
  
  // Temperature data
  float ActiveSolenoidTemp;    // Temperature of active solenoid (°C)
  float DriverModuleTemp;      // PWM driver module temperature (°C)
  float SolenoidTemps[8];      // All solenoid temperatures (up to 8 solenoids)
  uint8_t NumSolenoids;        // Total number of solenoids (N)
  
  // Solenoid state
  uint8_t ActiveSolenoidID;    // Currently active solenoid (0 to N-1)
  bool ThermalShutdown;        // Thermal shutdown active (all solenoids disabled)
  float PWMThrottle;           // Current PWM throttle factor (0.5-1.0)
  
  // Fire command state
  bool FireCommandActive;      // Currently firing (solenoid active)
  float FireIntensity;         // Current fire intensity (0.0-1.0)
  unsigned long FireStartTime; // Timestamp when fire started (ms since boot)
  unsigned long FireDuration;  // Fire pulse duration (ms)
  
  // System state
  bool PlaySessionActive;      // Play session authorization (from game engine)
  bool CanFire;                 // Computed: Can fire = PlaySessionActive && !ThermalShutdown
  bool StationConnected;        // Station is sending telemetry (not timed out)
  
  // Tracker pose (placeholder - will be added when tracker integration complete)
  float PoseX, PoseY, PoseZ;
  float PoseRotX, PoseRotY, PoseRotZ, PoseRotW;
  
  // Timestamp
  unsigned long LastTelemetryTime;  // Last time telemetry was received (ms)
};

// Button events struct (fast updates, sent on state change)
// Packed array for all 4 stations in single message
struct FGunButtonEvents {
  bool Button0State[4];        // Left thumb button per station
  bool Button1State[4];        // Right thumb button per station
  unsigned long Timestamp;      // Timestamp when events occurred (ms)
};

// Gun telemetry struct (slow updates, sent periodically)
// Packed array for all 4 stations in single message
struct FGunTelemetry {
  // Temperature data (per station)
  float ActiveSolenoidTemp[4];    // Temperature of active solenoid (°C)
  float DriverModuleTemp[4];      // PWM driver module temperature (°C)
  
  // Solenoid state (per station)
  uint8_t ActiveSolenoidID[4];    // Currently active solenoid (0 to N-1)
  uint8_t NumSolenoids[4];        // Total number of solenoids (N)
  bool ThermalShutdown[4];        // Thermal shutdown active
  float PWMThrottle[4];           // Current PWM throttle factor (0.5-1.0)
  
  // Fire command state (per station)
  bool FireCommandActive[4];      // Currently firing
  float FireIntensity[4];         // Current fire intensity (0.0-1.0)
  unsigned long FireDuration[4];  // Fire pulse duration (ms)
  
  // System state (per station)
  bool PlaySessionActive;         // Play session authorization (same for all)
  bool CanFire[4];                // Computed: Can fire per station
  bool StationConnected[4];       // Station is sending telemetry
  
  // Timestamp
  unsigned long Timestamp;        // Timestamp when telemetry was collected (ms)
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

// Create controller instances
ScissorLiftController liftController;
ActuatorSystemController actuatorController;

// Unified state
float motionDuration = 1.0f; // Default duration in seconds
unsigned long motionStartTime = 0;
bool motionInProgress = false;

// Game state management
bool playSessionActive = false;  // Play session state (controls gun firing authorization)
unsigned long lastGameStateUpdate = 0;
const unsigned long GAME_STATE_UPDATE_INTERVAL_MS = 100;  // Send game state to child ECUs at 10 Hz

// Telemetry update rate control (server-controlled)
unsigned long buttonEventUpdateInterval = 50;   // Button events: 20 Hz (50ms) - fast updates
unsigned long telemetryUpdateInterval = 1000;   // Telemetry: 1 Hz (1000ms) - slow updates
unsigned long lastButtonEventTime = 0;
unsigned long lastTelemetryTime = 0;

// Child ECU state storage (4 stations)
const uint8_t NUM_GUN_STATIONS = 4;
FGunECUState gunECUStates[NUM_GUN_STATIONS];
unsigned long lastGunECUTelemetry[NUM_GUN_STATIONS] = {0, 0, 0, 0};
const unsigned long GUN_ECU_TIMEOUT_MS = 2000;  // 2 second timeout for child ECU telemetry
IPAddress gunECU_IPs[NUM_GUN_STATIONS];  // Store IP addresses of child ECUs (discovered via incoming packets)
uint16_t gunECU_Ports[NUM_GUN_STATIONS] = {8888, 8889, 8890, 8891};  // Child ECU TX ports (child ECUs send to this port)

// UDP socket for receiving from child ECUs (separate from game engine RX)
WiFiUDP gunECU_UDP;
const uint16_t GUN_ECU_RX_PORT = 8892;  // Port to receive child ECU telemetry

// =====================================
// Setup
// =====================================

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\nHoloCade Gunship Experience ECU");
  Serial.println("==============================\n");
  
  // Configure scissor lift
  ScissorLiftConfig liftConfig;
  liftConfig.motorUpPin = 12;
  liftConfig.motorDownPin = 13;
  liftConfig.positionSensorPin = 14;
  liftConfig.topLimitPin = 15;
  liftConfig.bottomLimitPin = 16;
  
  liftConfig.enableForwardReverse = true;
  liftConfig.motorForwardPin = 17;
  liftConfig.motorReversePin = 18;
  liftConfig.forwardLimitPin = 19;
  liftConfig.reverseLimitPin = 20;
  liftConfig.maxForwardReverseCm = 90.0f;
  
  liftConfig.maxHeightCm = 300.0f;
  liftConfig.minHeightCm = 0.0f;
  liftConfig.softwareUpperLimitCm = 90.0f;
  liftConfig.maxSpeedCmPerSec = 10.0f;
  
  liftConfig.autoCalibrateMode = true;
  liftConfig.autoCalibrateTimeoutMs = 2000;
  
  // Initialize scissor lift controller
  liftController.begin(liftConfig);
  
  // Configure actuator system
  ActuatorSystemConfig actuatorConfig;
  actuatorConfig.valvePins[0] = 21;
  actuatorConfig.valvePins[1] = 22;
  actuatorConfig.valvePins[2] = 25;
  actuatorConfig.valvePins[3] = 26;
  actuatorConfig.sensorPins[0] = 32;
  actuatorConfig.sensorPins[1] = 33;
  actuatorConfig.sensorPins[2] = 34;
  actuatorConfig.sensorPins[3] = 35;
  actuatorConfig.lowerLimitPins[0] = 27;
  actuatorConfig.lowerLimitPins[1] = 4;
  actuatorConfig.lowerLimitPins[2] = 5;
  actuatorConfig.lowerLimitPins[3] = 2;
  actuatorConfig.upperLimitPins[0] = 0;
  actuatorConfig.upperLimitPins[1] = 1;
  actuatorConfig.upperLimitPins[2] = 3;
  actuatorConfig.upperLimitPins[3] = 4;
  
  actuatorConfig.maxPitchDeg = 10.0f;
  actuatorConfig.maxRollDeg = 10.0f;
  actuatorConfig.actuatorStrokeCm = 7.62f;
  actuatorConfig.platformWidthCm = 150.0f;
  actuatorConfig.platformLengthCm = 200.0f;
  
  actuatorConfig.kp = 2.0f;
  actuatorConfig.ki = 0.1f;
  actuatorConfig.kd = 0.5f;
  
  actuatorConfig.autoCalibrateMode = true;
  actuatorConfig.autoCalibrateTimeoutMs = 2000;
  
  // Initialize actuator system controller
  actuatorController.begin(actuatorConfig);
  
  // Initialize wireless communication (RX for receiving commands from game engine)
  HoloCade_Wireless_Init(ssid, password, 8888);
  
  // Initialize TX for sending feedback to Unreal
  HoloCade_Wireless_TX_Init(ssid, password, unrealIP, unrealPort);
  
  // Initialize UDP socket for receiving from child ECUs
  gunECU_UDP.begin(GUN_ECU_RX_PORT);
  Serial.printf("Child ECU telemetry listener started on port %d\n", GUN_ECU_RX_PORT);

  // Initialize child ECU state arrays
  for (uint8_t i = 0; i < NUM_GUN_STATIONS; i++) {
    gunECUStates[i].Button0State = false;
    gunECUStates[i].Button1State = false;
    gunECUStates[i].Button0LastState = false;
    gunECUStates[i].Button1LastState = false;
    gunECUStates[i].ActiveSolenoidTemp = 25.0f;
    gunECUStates[i].DriverModuleTemp = 25.0f;
    for (uint8_t j = 0; j < 8; j++) {
      gunECUStates[i].SolenoidTemps[j] = 25.0f;
    }
    gunECUStates[i].NumSolenoids = 1;
    gunECUStates[i].ActiveSolenoidID = 0;
    gunECUStates[i].ThermalShutdown = false;
    gunECUStates[i].PWMThrottle = 1.0f;
    gunECUStates[i].FireCommandActive = false;
    gunECUStates[i].FireIntensity = 0.0f;
    gunECUStates[i].FireStartTime = 0;
    gunECUStates[i].FireDuration = 100;
    gunECUStates[i].PlaySessionActive = false;
    gunECUStates[i].CanFire = false;
    gunECUStates[i].StationConnected = false;
    gunECUStates[i].PoseX = 0.0f;
    gunECUStates[i].PoseY = 0.0f;
    gunECUStates[i].PoseZ = 0.0f;
    gunECUStates[i].PoseRotX = 0.0f;
    gunECUStates[i].PoseRotY = 0.0f;
    gunECUStates[i].PoseRotZ = 0.0f;
    gunECUStates[i].PoseRotW = 1.0f;
    gunECUStates[i].LastTelemetryTime = 0;
    lastGunECUTelemetry[i] = 0;
  }
  
  motionStartTime = millis();
  
  Serial.println("\nGunship Experience ECU Ready!");
  Serial.println("Waiting for motion commands from Unreal/Unity...\n");
  Serial.println("Channel Mapping:");
  Serial.println("  Ch 0: Pitch (degrees or normalized)");
  Serial.println("  Ch 1: Roll (degrees or normalized)");
  Serial.println("  Ch 2: TranslationY / Forward-Reverse (cm or normalized)");
  Serial.println("  Ch 3: TranslationZ / Vertical (cm or normalized)");
  Serial.println("  Ch 4: Duration (seconds)");
  Serial.println("  Ch 5: Calibration Mode (bool)");
  Serial.println("  Ch 6: Operation Mode (bool: auto-calibrate/fixed)");
  Serial.println("  Ch 7: Emergency Stop (bool)");
  Serial.println("  Ch 8: Return to Neutral (bool)\n");
  Serial.println("NOTICE: Both subsystems must be calibrated before use.\n");
}

// =====================================
// Main Loop
// =====================================

void loop() {
  // Process incoming HoloCade commands from game engine
  HoloCade_ProcessIncoming();
  
  // Process incoming telemetry from child ECUs
  ProcessGunECUTelemetry();
  
  // Update both controllers
  liftController.update();
  actuatorController.update();
  
  // Send game state to child ECUs (play session active/inactive)
  unsigned long currentTime = millis();
  if (currentTime - lastGameStateUpdate >= GAME_STATE_UPDATE_INTERVAL_MS) {
    SendGameStateToGunECUs();
    lastGameStateUpdate = currentTime;
  }
  
  // Send position feedback to Unreal (bidirectional IO) - every 100ms (10 Hz)
  static unsigned long lastFeedbackTime = 0;
  if (currentTime - lastFeedbackTime >= 100) {
    SendPositionFeedback();
    lastFeedbackTime = currentTime;
  }
  
  // Send button events (fast updates, on change or periodic)
  if (currentTime - lastButtonEventTime >= buttonEventUpdateInterval) {
    SendGunButtonEvents();
    lastButtonEventTime = currentTime;
  }
  
  // Send telemetry (slow updates, periodic)
  if (currentTime - lastTelemetryTime >= telemetryUpdateInterval) {
    SendGunTelemetry();
    lastTelemetryTime = currentTime;
  }
  
  delay(10); // Control loop update rate (~100 Hz)
}

// =====================================
// HoloCade Command Handlers
// =====================================

void HoloCade_HandleFloat(uint8_t channel, float value) {
  switch (channel) {
    case 0:
      // Channel 0: Pitch (degrees or normalized -1.0 to +1.0)
      actuatorController.handleFloatCommand(0, value);
      break;
      
    case 1:
      // Channel 1: Roll (degrees or normalized -1.0 to +1.0)
      actuatorController.handleFloatCommand(1, value);
      break;
      
    case 2:
      // Channel 2: TranslationY / Forward-Reverse (cm or normalized -1.0 to +1.0)
      liftController.handleFloatCommand(1, value); // Channel 1 for forward/reverse in lift controller
      break;
      
    case 3:
      // Channel 3: TranslationZ / Vertical (cm or normalized -1.0 to +1.0)
      liftController.handleFloatCommand(0, value); // Channel 0 for vertical in lift controller
      break;
      
    case 4:
      // Channel 4: Duration (seconds)
      motionDuration = max(value, 0.1f);
      motionStartTime = millis();
      motionInProgress = true;
      Serial.printf("ECU: Motion duration = %.2f seconds\n", motionDuration);
      break;
  }
}

void HoloCade_HandleBool(uint8_t channel, bool value) {
  if (channel == 5) {
    // Channel 5: Calibration mode
    // Calibrate lift first, then actuators
    static bool calibratingLift = true;
    static bool calibrationActive = false;
    
    if (value && !calibrationActive) {
      // Start calibration
      calibrationActive = true;
      calibratingLift = true;
      liftController.setCalibrationMode(true);
      Serial.println("ECU: Entering calibration mode");
      Serial.println("First calibrate lift (bottom limit = zero), then actuators.");
      Serial.println("Upper limit is software-defined (90cm default).\n");
    } else if (!value && calibrationActive) {
      // End calibration
      if (calibratingLift) {
        // Finish lift calibration, start actuator calibration
        liftController.setCalibrationMode(false);
        if (liftController.isCalibrated()) {
          calibratingLift = false;
          actuatorController.setCalibrationMode(true);
          Serial.println("ECU: Lift calibrated. Now calibrating actuators...\n");
        }
      } else {
        // Finish actuator calibration
        actuatorController.setCalibrationMode(false);
        calibratingLift = true; // Reset for next calibration cycle
        calibrationActive = false;
        Serial.println("ECU: All subsystems calibrated!\n");
      }
    }
  } else if (channel == 6) {
    // Channel 6: Operation mode (true = auto-calibrate, false = fixed)
    liftController.setOperationMode(value);
    actuatorController.setOperationMode(value);
    Serial.printf("ECU: Operation mode = %s\n", value ? "Auto-calibrate" : "Fixed");
  } else if (channel == 7) {
    // Channel 7: Emergency stop
    liftController.setEmergencyStop(value);
    actuatorController.setEmergencyStop(value);
    if (value) {
      Serial.println("ECU: EMERGENCY STOP ACTIVATED");
    } else {
      Serial.println("ECU: Emergency stop released");
    }
  } else if (channel == 8) {
    // Channel 8: Return to neutral (calibrated zero)
    if (value) {
      liftController.returnToNeutral();
      actuatorController.returnToNeutral();
      Serial.println("ECU: Returning to calibrated zero position");
    }
  } else if (channel == 9) {
    // Channel 9: Play session active (from game engine)
    playSessionActive = value;
    Serial.printf("ECU: Play session %s\n", value ? "ACTIVE" : "INACTIVE");
    // Game state will be sent to child ECUs in main loop
  }
}

void HoloCade_HandleInt32(uint8_t channel, int32_t value) {
  if (channel == 100) {
    // Channel 100: Button event update interval (ms) - server-controlled
    buttonEventUpdateInterval = max((unsigned long)value, 10UL);  // Minimum 10ms (100 Hz max)
    Serial.printf("ECU: Button event update interval set to %lu ms\n", buttonEventUpdateInterval);
  } else if (channel == 101) {
    // Channel 101: Telemetry update interval (ms) - server-controlled
    telemetryUpdateInterval = max((unsigned long)value, 100UL);  // Minimum 100ms (10 Hz max)
    Serial.printf("ECU: Telemetry update interval set to %lu ms\n", telemetryUpdateInterval);
  }
}

// =====================================
// Struct Packet Handlers (MVC Pattern)
// =====================================

void HoloCade_HandleBytes(uint8_t channel, uint8_t* data, uint8_t length) {
  switch (channel) {
    case 100: {
      // Channel 100: FTiltState struct (pitch and roll only)
      if (length >= sizeof(FTiltState)) {
        FTiltState* tiltState = (FTiltState*)data;
        actuatorController.handleFloatCommand(0, tiltState->Pitch);
        actuatorController.handleFloatCommand(1, tiltState->Roll);
        Serial.printf("ECU: Tilt struct - Pitch: %.2f, Roll: %.2f\n", 
          tiltState->Pitch, tiltState->Roll);
      }
      break;
    }
    
    case 101: {
      // Channel 101: FScissorLiftState struct (Y and Z translations only)
      if (length >= sizeof(FScissorLiftState)) {
        FScissorLiftState* liftState = (FScissorLiftState*)data;
        liftController.handleFloatCommand(1, liftState->TranslationY); // Forward/reverse
        liftController.handleFloatCommand(0, liftState->TranslationZ);  // Up/down
        Serial.printf("ECU: Scissor lift struct - Y: %.2f, Z: %.2f\n", 
          liftState->TranslationY, liftState->TranslationZ);
      }
      break;
    }
    
    case 200: {
      // Channel 200: FPlatformMotionCommand struct (full command)
      if (length >= sizeof(FPlatformMotionCommand)) {
        FPlatformMotionCommand* command = (FPlatformMotionCommand*)data;
        actuatorController.handleFloatCommand(0, command->Pitch);
        actuatorController.handleFloatCommand(1, command->Roll);
        liftController.handleFloatCommand(1, command->TranslationY);
        liftController.handleFloatCommand(0, command->TranslationZ);
        motionDuration = max(command->Duration, 0.1f);
        motionStartTime = millis();
        motionInProgress = true;
        Serial.printf("ECU: Full command struct - Pitch: %.2f, Roll: %.2f, Y: %.2f, Z: %.2f, Duration: %.2f\n",
          command->Pitch, command->Roll, command->TranslationY, command->TranslationZ, command->Duration);
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
  // Get current positions from controllers
  // Note: Using target positions as current positions (controllers track these internally)
  float currentPitch = actuatorController.getTargetPitch();
  float currentRoll = actuatorController.getTargetRoll();
  float currentLiftY = liftController.getCurrentForwardPosition();
  float currentLiftZ = liftController.getCurrentHeight();
  
  // Send tilt state feedback (Channel 100)
  FTiltState tiltFeedback;
  tiltFeedback.Pitch = currentPitch;
  tiltFeedback.Roll = currentRoll;
  HoloCade_SendBytes(100, (uint8_t*)&tiltFeedback, sizeof(FTiltState));
  
  // Send scissor lift state feedback (Channel 101)
  FScissorLiftState liftFeedback;
  liftFeedback.TranslationY = currentLiftY;
  liftFeedback.TranslationZ = currentLiftZ;
  HoloCade_SendBytes(101, (uint8_t*)&liftFeedback, sizeof(FScissorLiftState));
}

// =====================================
// Child ECU Telemetry Processing
// =====================================

void ProcessGunECUTelemetry() {
  int packetSize = gunECU_UDP.parsePacket();
  if (packetSize == 0) return;
  
  // Read packet
  uint8_t buffer[256];
  int len = gunECU_UDP.read(buffer, 256);
  
  if (len < 5) return;  // Minimum packet size
  
  // Validate start marker
  if (buffer[0] != 0xAA) return;  // HoloCade start marker
  
  // Get sender IP (for station identification)
  IPAddress senderIP = gunECU_UDP.remoteIP();
  
  // Parse HoloCade packet: [0xAA][Type][Channel][Payload...][CRC]
  uint8_t packetType = buffer[1];
  uint8_t channel = buffer[2];
  
  // Determine station ID from channel or IP (channels 10-13 for stations 0-3)
  uint8_t stationID = 255;  // Invalid
  if (channel >= 10 && channel <= 13) {
    stationID = channel - 10;
  } else if (channel >= 30 && channel <= 33) {
    stationID = channel - 30;
  } else if (channel >= 40 && channel <= 43) {
    stationID = channel - 40;
  } else if (channel >= 50 && channel <= 53) {
    stationID = channel - 50;
  } else if (channel >= 60 && channel <= 67) {
    // Individual solenoid temps: 60-67 map to stations 0-3 (60+stationID*2, 61+stationID*2)
    // Simplified: use first digit
    stationID = (channel - 60) / 2;
    if (stationID >= NUM_GUN_STATIONS) stationID = 255;
  } else if (channel >= 70 && channel <= 73) {
    stationID = channel - 70;
  }
  
  if (stationID >= NUM_GUN_STATIONS) {
    // Unknown station, try to identify by IP (if we've seen it before)
    for (uint8_t i = 0; i < NUM_GUN_STATIONS; i++) {
      if (gunECU_IPs[i] == senderIP) {
        stationID = i;
        break;
      }
    }
    // If still unknown, assign to first available slot
    if (stationID >= NUM_GUN_STATIONS) {
      for (uint8_t i = 0; i < NUM_GUN_STATIONS; i++) {
        if (gunECU_IPs[i] == IPAddress(0, 0, 0, 0)) {
          gunECU_IPs[i] = senderIP;
          stationID = i;
          Serial.printf("Parent ECU: Discovered child ECU station %d at IP %s\n", i, senderIP.toString().c_str());
          break;
        }
      }
    }
  }
  
  if (stationID >= NUM_GUN_STATIONS) return;  // Invalid station
  
  // Store sender IP for this station
  gunECU_IPs[stationID] = senderIP;
  lastGunECUTelemetry[stationID] = millis();
  
  // Parse data based on channel and type
  if (packetType == 0) {  // BOOL
    if (channel >= 10 && channel <= 13) {
      // Button state (combined - child ECU sends combined state)
      // Note: child ECU currently sends combined button state
      // Individual button parsing would require separate channels (e.g., 10+n for button 0, 11+n for button 1)
      bool buttonState = (buffer[3] != 0);
      gunECUStates[stationID].Button0State = buttonState;  // For now, treat as combined
      gunECUStates[stationID].Button1State = buttonState;  // TODO: Parse individual buttons when child ECU sends them separately
    } else if (channel >= 80 && channel <= 83) {
      // Fire command active (Channel 80+n)
      bool wasFiring = gunECUStates[stationID].FireCommandActive;
      gunECUStates[stationID].FireCommandActive = (buffer[3] != 0);
      // Track fire start time when fire command becomes active
      if (!wasFiring && gunECUStates[stationID].FireCommandActive) {
        gunECUStates[stationID].FireStartTime = millis();
      }
    } else if (channel >= 90 && channel <= 93) {
      // Thermal shutdown (Channel 90+n)
      gunECUStates[stationID].ThermalShutdown = (buffer[3] != 0);
    }
  } else if (packetType == 1) {  // INT32
    if (channel >= 40 && channel <= 43) {
      // Active solenoid ID
      int32_t value = (int32_t)(buffer[3] | (buffer[4] << 8) | (buffer[5] << 16) | (buffer[6] << 24));
      gunECUStates[stationID].ActiveSolenoidID = (uint8_t)value;
    } else if (channel >= 50 && channel <= 53) {
      // Total solenoid count
      int32_t value = (int32_t)(buffer[3] | (buffer[4] << 8) | (buffer[5] << 16) | (buffer[6] << 24));
      gunECUStates[stationID].NumSolenoids = (uint8_t)value;
    }
  } else if (packetType == 2) {  // FLOAT
    if (channel >= 30 && channel <= 33) {
      // Active solenoid temperature
      uint32_t intValue = buffer[3] | (buffer[4] << 8) | (buffer[5] << 16) | (buffer[6] << 24);
      gunECUStates[stationID].ActiveSolenoidTemp = *(float*)&intValue;
      // Also update in array
      if (gunECUStates[stationID].ActiveSolenoidID < 8) {
        gunECUStates[stationID].SolenoidTemps[gunECUStates[stationID].ActiveSolenoidID] = gunECUStates[stationID].ActiveSolenoidTemp;
      }
    } else if (channel >= 60 && channel <= 67) {
      // Individual solenoid temperatures (Channel 60+stationID*2+i for station n, solenoid i)
      // Channel mapping: Station 0: 60,61 | Station 1: 62,63 | Station 2: 64,65 | Station 3: 66,67
      uint8_t channelOffset = channel - 60;
      uint8_t stationFromChannel = channelOffset / 2;
      uint8_t solenoidIndex = channelOffset % 2;
      if (stationFromChannel == stationID && solenoidIndex < 8) {
        uint32_t intValue = buffer[3] | (buffer[4] << 8) | (buffer[5] << 16) | (buffer[6] << 24);
        gunECUStates[stationID].SolenoidTemps[solenoidIndex] = *(float*)&intValue;
      }
    } else if (channel >= 70 && channel <= 73) {
      // Driver module temperature
      uint32_t intValue = buffer[3] | (buffer[4] << 8) | (buffer[5] << 16) | (buffer[6] << 24);
      gunECUStates[stationID].DriverModuleTemp = *(float*)&intValue;
    } else if (channel >= 20 && channel <= 23) {
      // Fire intensity (Channel 20+n)
      uint32_t intValue = buffer[3] | (buffer[4] << 8) | (buffer[5] << 16) | (buffer[6] << 24);
      gunECUStates[stationID].FireIntensity = *(float*)&intValue;
    } else if (channel >= 95 && channel <= 98) {
      // PWM throttle (Channel 95+n)
      uint32_t intValue = buffer[3] | (buffer[4] << 8) | (buffer[5] << 16) | (buffer[6] << 24);
      gunECUStates[stationID].PWMThrottle = *(float*)&intValue;
    }
  }
  
  // Update timestamp and connection status
  unsigned long currentTime = millis();
  gunECUStates[stationID].LastTelemetryTime = currentTime;
  gunECUStates[stationID].StationConnected = true;
  
  // Update fire duration (calculate from start time if firing)
  if (gunECUStates[stationID].FireCommandActive && gunECUStates[stationID].FireStartTime > 0) {
    gunECUStates[stationID].FireDuration = currentTime - gunECUStates[stationID].FireStartTime;
  } else {
    gunECUStates[stationID].FireDuration = 0;
  }
  
  // Update computed state
  gunECUStates[stationID].PlaySessionActive = playSessionActive;
  gunECUStates[stationID].CanFire = playSessionActive && !gunECUStates[stationID].ThermalShutdown;
}

// =====================================
// Game State Management
// =====================================

void SendGameStateToGunECUs() {
  // Send play session state to all known child ECUs
  for (uint8_t i = 0; i < NUM_GUN_STATIONS; i++) {
    if (gunECU_IPs[i] != IPAddress(0, 0, 0, 0)) {
      // Channel 9: Play session active (bool)
      // Use the TX template's send function but target child ECU IP
      uint8_t packet[5];
      packet[0] = 0xAA;  // Start marker
      packet[1] = 0;      // BOOL type
      packet[2] = 9;      // Channel 9
      packet[3] = playSessionActive ? 1 : 0;
      
      // Calculate CRC
      uint8_t crc = 0;
      for (int j = 0; j < 4; j++) {
        crc ^= packet[j];
      }
      packet[4] = crc;
      
      // Send to child ECU
      gunECU_UDP.beginPacket(gunECU_IPs[i], gunECU_Ports[i]);
      gunECU_UDP.write(packet, 5);
      gunECU_UDP.endPacket();
    }
  }
}

// =====================================
// Button Events (Fast Updates)
// =====================================

void SendGunButtonEvents() {
  // Send button events for all 4 stations in a single packed message
  // Channel 310: FGunButtonEvents struct (all 4 stations, fast updates)
  // Update rate: Configurable (default 20 Hz / 50ms)
  // Sent on state change or periodic (whichever comes first)
  
  FGunButtonEvents buttonEvents;
  unsigned long currentTime = millis();
  bool hasChanges = false;
  
  for (uint8_t i = 0; i < NUM_GUN_STATIONS; i++) {
    // Copy current button states
    buttonEvents.Button0State[i] = gunECUStates[i].Button0State;
    buttonEvents.Button1State[i] = gunECUStates[i].Button1State;
    
    // Check for state changes (for future optimization: send only on change)
    if (gunECUStates[i].Button0State != gunECUStates[i].Button0LastState ||
        gunECUStates[i].Button1State != gunECUStates[i].Button1LastState) {
      hasChanges = true;
      // Update last state
      gunECUStates[i].Button0LastState = gunECUStates[i].Button0State;
      gunECUStates[i].Button1LastState = gunECUStates[i].Button1State;
    }
  }
  
  buttonEvents.Timestamp = currentTime;
  
  // Send button events (always send periodic updates, even if no changes)
  // Future optimization: Only send on change if server requests event-driven mode
  HoloCade_SendBytes(310, (uint8_t*)&buttonEvents, sizeof(FGunButtonEvents));
}

// =====================================
// Gun Telemetry (Slow Updates)
// =====================================

void SendGunTelemetry() {
  // Send telemetry for all 4 stations in a single packed message
  // Channel 311: FGunTelemetry struct (all 4 stations, slow updates)
  // Update rate: Configurable (default 1 Hz / 1000ms)
  //
  // Contains:
  // - Temperature data (ActiveSolenoidTemp, DriverModuleTemp)
  // - Solenoid state (ActiveSolenoidID, NumSolenoids, ThermalShutdown, PWMThrottle)
  // - Fire command state (FireCommandActive, FireIntensity, FireDuration)
  // - System state (PlaySessionActive, CanFire, StationConnected)
  //
  // Game engine uses this for:
  // - Console monitor display (real-time status for all guns)
  // - Safety monitoring (thermal shutdown, connection status)
  // - Performance metrics (temperature trends, fire duration)
  
  FGunTelemetry telemetry;
  unsigned long currentTime = millis();
  
  for (uint8_t i = 0; i < NUM_GUN_STATIONS; i++) {
    // Check if station is active (received telemetry recently)
    bool stationActive = (currentTime - lastGunECUTelemetry[i] < GUN_ECU_TIMEOUT_MS);
    gunECUStates[i].StationConnected = stationActive;
    
    // Update computed state
    gunECUStates[i].PlaySessionActive = playSessionActive;
    gunECUStates[i].CanFire = playSessionActive && !gunECUStates[i].ThermalShutdown;
    
    // Update fire duration if currently firing
    if (gunECUStates[i].FireCommandActive && gunECUStates[i].FireStartTime > 0) {
      gunECUStates[i].FireDuration = currentTime - gunECUStates[i].FireStartTime;
    }
    
    // Copy telemetry data to packed struct
    telemetry.ActiveSolenoidTemp[i] = gunECUStates[i].ActiveSolenoidTemp;
    telemetry.DriverModuleTemp[i] = gunECUStates[i].DriverModuleTemp;
    telemetry.ActiveSolenoidID[i] = gunECUStates[i].ActiveSolenoidID;
    telemetry.NumSolenoids[i] = gunECUStates[i].NumSolenoids;
    telemetry.ThermalShutdown[i] = gunECUStates[i].ThermalShutdown;
    telemetry.PWMThrottle[i] = gunECUStates[i].PWMThrottle;
    telemetry.FireCommandActive[i] = gunECUStates[i].FireCommandActive;
    telemetry.FireIntensity[i] = gunECUStates[i].FireIntensity;
    telemetry.FireDuration[i] = gunECUStates[i].FireDuration;
    telemetry.CanFire[i] = gunECUStates[i].CanFire;
    telemetry.StationConnected[i] = gunECUStates[i].StationConnected;
  }
  
  telemetry.PlaySessionActive = playSessionActive;
  telemetry.Timestamp = currentTime;
  
  // Send telemetry (always send, even if stations disconnected)
  HoloCade_SendBytes(311, (uint8_t*)&telemetry, sizeof(FGunTelemetry));
}

// =====================================
// Game State Command Handler
// =====================================
