/*
 * HoloCade Gyroscope Controller Header
 * 
 * Reusable header file for controlling 2DOF continuous rotation gyroscopes.
 * Handles pitch and roll commands from game engine via professional servo motors.
 * 
 * Supports multiple servo drive brands through IServoDrive interface:
 * - Yaskawa Sigma-5 (default)
 * - Panasonic Minas A6
 * - Kollmorgen AKM
 * 
 * This header can be included in any sketch to add gyroscope control functionality.
 * 
 * Usage:
 *   #include "GyroscopeController.h"
 *   #include "Yaskawa_Sigma5_Drive.h"  // or Panasonic_MinasA6_Drive.h, Kollmorgen_AKM_Drive.h
 *   
 *   GyroscopeController gyroController;
 *   
 *   void setup() {
 *     GyroscopeConfig config;
 *     config.servoDriveType = ServoDriveType::YaskawaSigma5;  // or PanasonicMinasA6, KollmorgenAKM
 *     config.maxRotationSpeedDegreesPerSecond = 90.0f;
 *     config.smoothingFactor = 0.2f;
 *     gyroController.begin(config);
 *   }
 *   
 *   void loop() {
 *     gyroController.update();
 *   }
 * 
 * Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.
 */

#ifndef GYROSCOPE_CONTROLLER_H
#define GYROSCOPE_CONTROLLER_H

#include <Arduino.h>
#include "ServoDrive_Interface.h"

// Servo drive type selection
enum class ServoDriveType {
  YaskawaSigma5,      // Default - Yaskawa Sigma-5 series
  PanasonicMinasA6,   // Panasonic Minas A6 series
  KollmorgenAKM       // Kollmorgen AKM series (premium)
};

// Configuration structure for gyroscope system
struct GyroscopeConfig {
  // Servo drive type selection
  ServoDriveType servoDriveType;  // Which servo drive brand to use (default: YaskawaSigma5)
  
  // Servo drive configuration (passed to IServoDrive::initialize)
  ServoDriveConfig pitchDriveConfig;  // Configuration for pitch axis servo drive
  ServoDriveConfig rollDriveConfig;   // Configuration for roll axis servo drive
  
  // Motion parameters
  float maxRotationSpeedDegreesPerSecond;  // Maximum rotation speed in degrees per second
  
  // Smoothing/interpolation parameters
  float smoothingFactor;        // Smoothing factor (0.0 to 1.0, higher = smoother but slower response)
                                 // 0.0 = no smoothing (instant), 1.0 = maximum smoothing
                                 // Recommended: 0.1 to 0.3 for responsive but smooth motion
  
  // Gravity reset parameters
  bool enableGravityReset;       // If true, gyros smoothstep toward up (0° pitch, 0° roll) when idle
  float resetSpeed;              // Speed for gravity reset smoothstep (degrees per second)
  float resetIdleTimeout;        // Idle timeout in seconds before gravity reset activates
};

class GyroscopeController {
public:
  GyroscopeController();
  ~GyroscopeController();
  
  // Initialize the controller with configuration
  // Creates appropriate servo drive instances based on servoDriveType
  // Returns true if initialization successful
  bool begin(const GyroscopeConfig& config);
  
  // Main update loop - call this in your loop() function
  void update();
  
  // Command handlers - call these from your HoloCade_HandleFloat/Bool functions
  void setTargetPitch(float pitch);
  void setTargetRoll(float roll);
  void setEmergencyStop(bool stop);
  void returnToNeutral();
  
  // Gravity reset control
  void setGravityResetEnabled(bool enabled);
  void setResetSpeed(float speedDegreesPerSecond);
  void setResetIdleTimeout(float timeoutSeconds);
  
  // Get current state
  float getCurrentPitch() const { return currentPitch; }
  float getCurrentRoll() const { return currentRoll; }
  float getTargetPitch() const { return targetPitch; }
  float getTargetRoll() const { return targetRoll; }
  bool isEmergencyStop() const { return emergencyStop; }
  
  // Get servo drive instances (for advanced control)
  IServoDrive* getPitchDrive() { return pitchDrive; }
  IServoDrive* getRollDrive() { return rollDrive; }
  
private:
  // Configuration
  GyroscopeConfig config;
  bool initialized;
  
  // Servo drive instances (polymorphic - can be any IServoDrive implementation)
  IServoDrive* pitchDrive;
  IServoDrive* rollDrive;
  
  // State tracking
  float targetPitch;
  float targetRoll;
  float currentPitch;
  float currentRoll;
  
  // Smoothing interpolation state
  float smoothedPitch;
  float smoothedRoll;
  
  // Gravity reset state
  bool gravityResetEnabled;
  float resetSpeed;
  float resetIdleTimeout;
  unsigned long lastInputTime;  // Last time we received input (for idle detection)
  
  // Emergency stop
  bool emergencyStop;
  
  // Timing
  unsigned long lastUpdateTime;
  
  // Internal methods
  IServoDrive* createServoDrive(ServoDriveType type);
  void updateServos();
  void updateGravityReset(float deltaTime);
  bool isIdle() const;
};

// =====================================
// Implementation
// =====================================

GyroscopeController::GyroscopeController() 
  : initialized(false), pitchDrive(nullptr), rollDrive(nullptr),
    targetPitch(0.0f), targetRoll(0.0f), 
    currentPitch(0.0f), currentRoll(0.0f),
    smoothedPitch(0.0f), smoothedRoll(0.0f),
    gravityResetEnabled(false), resetSpeed(30.0f), resetIdleTimeout(5.0f),
    lastInputTime(0), emergencyStop(false), lastUpdateTime(0) {
}

GyroscopeController::~GyroscopeController() {
  if (pitchDrive) {
    pitchDrive->shutdown();
    delete pitchDrive;
    pitchDrive = nullptr;
  }
  if (rollDrive) {
    rollDrive->shutdown();
    delete rollDrive;
    rollDrive = nullptr;
  }
}

bool GyroscopeController::begin(const GyroscopeConfig& cfg) {
  config = cfg;
  
  // Create servo drive instances based on drive type
  pitchDrive = createServoDrive(config.servoDriveType);
  rollDrive = createServoDrive(config.servoDriveType);
  
  if (!pitchDrive || !rollDrive) {
    // Failed to create drives
    return false;
  }
  
  // Initialize pitch drive
  if (!pitchDrive->initialize(config.pitchDriveConfig)) {
    return false;
  }
  
  // Initialize roll drive
  if (!rollDrive->initialize(config.rollDriveConfig)) {
    return false;
  }
  
  // Enable drives
  if (!pitchDrive->enable() || !rollDrive->enable()) {
    return false;
  }
  
  // Set control mode to position
  if (!pitchDrive->setControlMode(ServoControlMode::Position) ||
      !rollDrive->setControlMode(ServoControlMode::Position)) {
    return false;
  }
  
  // Initialize gravity reset
  gravityResetEnabled = config.enableGravityReset;
  resetSpeed = config.resetSpeed;
  resetIdleTimeout = config.resetIdleTimeout;
  lastInputTime = millis();
  
  initialized = true;
  return true;
}

IServoDrive* GyroscopeController::createServoDrive(ServoDriveType type) {
  // Factory method to create appropriate servo drive instance
  // Note: Include the appropriate drive header file in your sketch
  switch (type) {
    case ServoDriveType::YaskawaSigma5: {
      #ifdef YASKAWA_SIGMA5_DRIVE_H
      return new YaskawaSigma5Drive();
      #else
      // Yaskawa_Sigma5_Drive.h not included - return nullptr
      return nullptr;
      #endif
    }
    case ServoDriveType::PanasonicMinasA6: {
      #ifdef PANASONIC_MINASA6_DRIVE_H
      return new PanasonicMinasA6Drive();
      #else
      // Panasonic_MinasA6_Drive.h not included - return nullptr
      return nullptr;
      #endif
    }
    case ServoDriveType::KollmorgenAKM: {
      #ifdef KOLLMORGEN_AKM_DRIVE_H
      return new KollmorgenAKMDrive();
      #else
      // Kollmorgen_AKM_Drive.h not included - return nullptr
      return nullptr;
      #endif
    }
    default:
      return nullptr;
  }
}

void GyroscopeController::update() {
  if (!initialized || !pitchDrive || !rollDrive) return;
  
  // Update servo drives (they may need periodic updates for communication)
  pitchDrive->update();
  rollDrive->update();
  
  unsigned long currentTime = millis();
  float deltaTime = (currentTime - lastUpdateTime) / 1000.0f;  // Convert to seconds
  if (deltaTime <= 0.0f || deltaTime > 1.0f) {
    lastUpdateTime = currentTime;
    return;
  }
  
  if (emergencyStop) {
    // Emergency stop - disable drives
    pitchDrive->emergencyStop();
    rollDrive->emergencyStop();
    lastUpdateTime = currentTime;
    return;
  }
  
  // Get current positions from drives (absolute encoder feedback)
  pitchDrive->getCurrentPosition(currentPitch);
  rollDrive->getCurrentPosition(currentRoll);
  
  // Apply smoothing interpolation to prevent jerky motion during network hiccups
  float smoothingFactor = config.smoothingFactor;
  smoothingFactor = constrain(smoothingFactor, 0.0f, 1.0f);
  
  // Calculate smoothed interpolation step (clamped to max rotation speed)
  float maxRotationSpeed = config.maxRotationSpeedDegreesPerSecond;
  float maxDelta = maxRotationSpeed * deltaTime;
  
  // Update smoothed pitch
  float pitchDelta = targetPitch - smoothedPitch;
  float pitchStep = pitchDelta * smoothingFactor;
  if (abs(pitchStep) > maxDelta) {
    pitchStep = (pitchStep > 0 ? maxDelta : -maxDelta);
  }
  smoothedPitch += pitchStep;
  
  // Update smoothed roll
  float rollDelta = targetRoll - smoothedRoll;
  float rollStep = rollDelta * smoothingFactor;
  if (abs(rollStep) > maxDelta) {
    rollStep = (rollStep > 0 ? maxDelta : -maxDelta);
  }
  smoothedRoll += rollStep;
  
  // Handle gravity reset if enabled
  if (gravityResetEnabled) {
    updateGravityReset(deltaTime);
  }
  
  // Update servo positions via drive interface
  updateServos();
  
  lastUpdateTime = currentTime;
}

void GyroscopeController::setTargetPitch(float pitch) {
  targetPitch = pitch;
  lastInputTime = millis();  // Update last input time for idle detection
}

void GyroscopeController::setTargetRoll(float roll) {
  targetRoll = roll;
  lastInputTime = millis();  // Update last input time for idle detection
}

void GyroscopeController::setEmergencyStop(bool stop) {
  emergencyStop = stop;
  if (stop && pitchDrive && rollDrive) {
    pitchDrive->emergencyStop();
    rollDrive->emergencyStop();
    targetPitch = currentPitch;
    targetRoll = currentRoll;
  } else if (!stop && pitchDrive && rollDrive) {
    pitchDrive->clearEmergencyStop();
    rollDrive->clearEmergencyStop();
    pitchDrive->enable();
    rollDrive->enable();
  }
}

void GyroscopeController::returnToNeutral() {
  targetPitch = 0.0f;
  targetRoll = 0.0f;
  emergencyStop = false;
  lastInputTime = millis();
}

void GyroscopeController::setGravityResetEnabled(bool enabled) {
  gravityResetEnabled = enabled;
}

void GyroscopeController::setResetSpeed(float speedDegreesPerSecond) {
  resetSpeed = speedDegreesPerSecond;
}

void GyroscopeController::setResetIdleTimeout(float timeoutSeconds) {
  resetIdleTimeout = timeoutSeconds;
}

void GyroscopeController::updateServos() {
  if (!pitchDrive || !rollDrive) return;
  
  // Send smoothed target positions to servo drives
  // The drives handle position control internally
  pitchDrive->setTargetPosition(smoothedPitch);
  rollDrive->setTargetPosition(smoothedRoll);
}

void GyroscopeController::updateGravityReset(float deltaTime) {
  if (!gravityResetEnabled) return;
  
  // Check if idle (no input for resetIdleTimeout seconds)
  if (isIdle()) {
    // Smoothstep toward zero (up position)
    float pitchDelta = 0.0f - smoothedPitch;
    float rollDelta = 0.0f - smoothedRoll;
    
    // Calculate step size based on reset speed
    float maxResetDelta = resetSpeed * deltaTime;
    
    // Apply gravity reset smoothing
    if (abs(pitchDelta) > 0.1f) {
      float pitchStep = constrain(pitchDelta, -maxResetDelta, maxResetDelta);
      smoothedPitch += pitchStep;
    } else {
      smoothedPitch = 0.0f;
    }
    
    if (abs(rollDelta) > 0.1f) {
      float rollStep = constrain(rollDelta, -maxResetDelta, maxResetDelta);
      smoothedRoll += rollStep;
    } else {
      smoothedRoll = 0.0f;
    }
    
    // Update targets to match smoothed values (so gravity reset continues)
    targetPitch = smoothedPitch;
    targetRoll = smoothedRoll;
  }
}

bool GyroscopeController::isIdle() const {
  unsigned long currentTime = millis();
  float idleTime = (currentTime - lastInputTime) / 1000.0f;  // Convert to seconds
  return idleTime >= resetIdleTimeout;
}

#endif // GYROSCOPE_CONTROLLER_H

