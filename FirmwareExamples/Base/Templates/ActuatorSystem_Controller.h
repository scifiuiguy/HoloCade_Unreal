/*
 * HoloCade Actuator System Controller Header
 * 
 * Reusable header file for controlling 4-gang hydraulic actuator systems.
 * Handles pitch and roll commands from game engine (2 DOF).
 * 
 * This header can be included in any sketch to add actuator control functionality.
 * Use ActuatorSystem_Controller_Standalone.ino for a standalone example.
 * 
 * Usage:
 *   #include "ActuatorSystem_Controller.h"
 *   
 *   ActuatorSystemController actuatorController;
 *   
 *   void setup() {
 *     actuatorController.begin(config);
 *   }
 *   
 *   void loop() {
 *     actuatorController.update();
 *   }
 * 
 * Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.
 */

#ifndef ACTUATOR_SYSTEM_CONTROLLER_H
#define ACTUATOR_SYSTEM_CONTROLLER_H

#include <Arduino.h>

// Configuration structure for actuator system
struct ActuatorSystemConfig {
  // Pin configuration
  int valvePins[4];           // PWM pins for proportional valves
  int sensorPins[4];          // Analog pins for position sensors
  int lowerLimitPins[4];      // Lower limit switches (optional, use -1 to disable)
  int upperLimitPins[4];       // Upper limit switches (optional, use -1 to disable)
  
  // Motion parameters
  float maxPitchDeg;           // Maximum pitch angle in degrees
  float maxRollDeg;            // Maximum roll angle in degrees
  float actuatorStrokeCm;      // Actuator stroke length in cm
  float platformWidthCm;       // Platform width (for geometry calculations)
  float platformLengthCm;      // Platform length (for geometry calculations)
  
  // PID control parameters
  float kp;                    // Proportional gain
  float ki;                    // Integral gain
  float kd;                    // Derivative gain
  
  // Operation mode
  bool autoCalibrateMode;      // true = auto-calibrate mode, false = fixed mode
  unsigned long autoCalibrateTimeoutMs; // Timeout for auto-calibrate mode
};

class ActuatorSystemController {
public:
  ActuatorSystemController();
  
  // Initialize the controller with configuration
  void begin(const ActuatorSystemConfig& config);
  
  // Main update loop - call this in your loop() function
  void update();
  
  // Command handlers - call these from your HoloCade_HandleFloat/Bool functions
  void handleFloatCommand(uint8_t channel, float value);
  void handleBoolCommand(uint8_t channel, bool value);
  
  // Get current state
  float getTargetPitch() const { return targetPitch; }
  float getTargetRoll() const { return targetRoll; }
  bool isEmergencyStop() const { return emergencyStop; }
  bool isCalibrated(int actuatorIndex) const { 
    return actuatorIndex >= 0 && actuatorIndex < 4 ? isCalibratedArray[actuatorIndex] : false; 
  }
  
  // Manual control (optional - for direct control without HoloCade commands)
  void setTargetPitch(float pitch);
  void setTargetRoll(float roll);
  void setEmergencyStop(bool stop);
  void setCalibrationMode(bool enable);
  void setOperationMode(bool autoCalibrate);
  void returnToNeutral();
  
private:
  // Configuration
  ActuatorSystemConfig config;
  bool initialized;
  
  // Calibration parameters
  float actuatorCalibratedZero[4];
  float actuatorLowerLimit[4];
  float actuatorUpperLimit[4];
  bool isCalibratedArray[4];
  
  // E-stop smoothing
  bool emergencyStop;
  bool eStopSmoothingActive;
  float eStopSmoothStartPositions[4];
  unsigned long eStopSmoothStartTime;
  static const float E_STOP_SMOOTH_DURATION_MS;
  
  // State tracking
  float targetPitch;
  float targetRoll;
  float currentActuatorPositions[4];
  float targetActuatorPositions[4];
  float calibratedActuatorPositions[4];
  
  // PID state
  float integralError[4];
  float lastError[4];
  unsigned long lastUpdateTime;
  unsigned long lastCommandTime;
  
  // Calibration mode
  bool calibrationMode;
  int currentCalibrationActuator;
  bool calibratingLower;
  
  // Internal methods
  void updatePositionSensors();
  void calculateTargetPositions();
  void handleCalibrationMode();
  void handleEStopSmoothing();
  void executePIDControl();
  bool checkLimitSwitch(int pin);
};

// Implementation
const float ActuatorSystemController::E_STOP_SMOOTH_DURATION_MS = 500.0f;

ActuatorSystemController::ActuatorSystemController() 
  : initialized(false), emergencyStop(false), eStopSmoothingActive(false),
    targetPitch(0.0f), targetRoll(0.0f), calibrationMode(false),
    currentCalibrationActuator(0), calibratingLower(true),
    lastUpdateTime(0), lastCommandTime(0) {
  // Initialize config with defaults (will be set in begin())
  config.actuatorStrokeCm = 7.62f; // Default 3 inches
  for (int i = 0; i < 4; i++) {
    actuatorCalibratedZero[i] = 0.0f;
    actuatorLowerLimit[i] = -config.actuatorStrokeCm / 2.0f;
    actuatorUpperLimit[i] = config.actuatorStrokeCm / 2.0f;
    isCalibratedArray[i] = false;
    currentActuatorPositions[i] = 0.0f;
    targetActuatorPositions[i] = 0.0f;
    calibratedActuatorPositions[i] = 0.0f;
    integralError[i] = 0.0f;
    lastError[i] = 0.0f;
    eStopSmoothStartPositions[i] = 0.0f;
  }
}

void ActuatorSystemController::begin(const ActuatorSystemConfig& cfg) {
  config = cfg;
  
  // Configure valve control pins (PWM)
  for (int i = 0; i < 4; i++) {
    pinMode(config.valvePins[i], OUTPUT);
    analogWrite(config.valvePins[i], 0);
  }
  
  // Configure position sensor pins (analog input)
  for (int i = 0; i < 4; i++) {
    pinMode(config.sensorPins[i], INPUT);
  }
  
  // Configure limit switch pins (optional)
  for (int i = 0; i < 4; i++) {
    if (config.lowerLimitPins[i] >= 0) {
      pinMode(config.lowerLimitPins[i], INPUT_PULLUP);
    }
    if (config.upperLimitPins[i] >= 0) {
      pinMode(config.upperLimitPins[i], INPUT_PULLUP);
    }
  }
  
  // Initialize calibration limits
  for (int i = 0; i < 4; i++) {
    actuatorLowerLimit[i] = -config.actuatorStrokeCm / 2.0f;
    actuatorUpperLimit[i] = config.actuatorStrokeCm / 2.0f;
  }
  
  lastUpdateTime = millis();
  lastCommandTime = millis();
  initialized = true;
}

void ActuatorSystemController::update() {
  if (!initialized) return;
  
  // Update position sensors
  updatePositionSensors();
  
  // Handle calibration mode
  if (calibrationMode) {
    handleCalibrationMode();
  } else {
    // Normal operation mode
    // Check auto-calibrate timeout
    if (config.autoCalibrateMode) {
      unsigned long timeSinceLastCommand = millis() - lastCommandTime;
      if (timeSinceLastCommand > config.autoCalibrateTimeoutMs) {
        // Auto-revert to calibrated zero
        targetPitch = 0.0f;
        targetRoll = 0.0f;
        calculateTargetPositions();
      }
    }
    
    // Calculate target actuator positions from pitch/roll
    calculateTargetPositions();
    
    // Handle e-stop smoothing
    if (emergencyStop) {
      handleEStopSmoothing();
    } else {
      // Execute closed-loop control
      executePIDControl();
    }
  }
}

void ActuatorSystemController::handleFloatCommand(uint8_t channel, float value) {
  lastCommandTime = millis();
  
  if (channel == 0) {
    // Channel 0: Pitch command
    if (value >= -1.0f && value <= 1.0f) {
      targetPitch = value * config.maxPitchDeg;
    } else {
      targetPitch = constrain(value, -config.maxPitchDeg, config.maxPitchDeg);
    }
  } else if (channel == 1) {
    // Channel 1: Roll command
    if (value >= -1.0f && value <= 1.0f) {
      targetRoll = value * config.maxRollDeg;
    } else {
      targetRoll = constrain(value, -config.maxRollDeg, config.maxRollDeg);
    }
  }
}

void ActuatorSystemController::handleBoolCommand(uint8_t channel, bool value) {
  lastCommandTime = millis();
  
  if (channel == 2) {
    // Channel 2: Calibration mode
    if (value) {
      calibrationMode = true;
      currentCalibrationActuator = 0;
      calibratingLower = true;
    } else {
      calibrationMode = false;
    }
  } else if (channel == 3) {
    // Channel 3: Operation mode
    config.autoCalibrateMode = value;
  } else if (channel == 4) {
    // Channel 4: Emergency stop
    if (value) {
      if (!emergencyStop) {
        for (int i = 0; i < 4; i++) {
          eStopSmoothStartPositions[i] = calibratedActuatorPositions[i];
        }
        eStopSmoothStartTime = millis();
        eStopSmoothingActive = true;
      }
      emergencyStop = true;
    } else {
      emergencyStop = false;
      eStopSmoothingActive = true;
      eStopSmoothStartTime = millis();
    }
  } else if (channel == 5) {
    // Channel 5: Return to neutral
    if (value) {
      returnToNeutral();
    }
  }
}

void ActuatorSystemController::setTargetPitch(float pitch) {
  targetPitch = constrain(pitch, -config.maxPitchDeg, config.maxPitchDeg);
  lastCommandTime = millis();
}

void ActuatorSystemController::setTargetRoll(float roll) {
  targetRoll = constrain(roll, -config.maxRollDeg, config.maxRollDeg);
  lastCommandTime = millis();
}

void ActuatorSystemController::setEmergencyStop(bool stop) {
  if (stop && !emergencyStop) {
    for (int i = 0; i < 4; i++) {
      eStopSmoothStartPositions[i] = calibratedActuatorPositions[i];
    }
    eStopSmoothStartTime = millis();
    eStopSmoothingActive = true;
  }
  emergencyStop = stop;
  if (!stop) {
    eStopSmoothingActive = true;
    eStopSmoothStartTime = millis();
  }
}

void ActuatorSystemController::setCalibrationMode(bool enable) {
  calibrationMode = enable;
  if (enable) {
    currentCalibrationActuator = 0;
    calibratingLower = true;
  }
}

void ActuatorSystemController::setOperationMode(bool autoCalibrate) {
  config.autoCalibrateMode = autoCalibrate;
}

void ActuatorSystemController::returnToNeutral() {
  targetPitch = 0.0f;
  targetRoll = 0.0f;
  emergencyStop = false;
  eStopSmoothingActive = false;
  lastCommandTime = millis();
}

void ActuatorSystemController::updatePositionSensors() {
  for (int i = 0; i < 4; i++) {
    int sensorValue = analogRead(config.sensorPins[i]);
    float voltage = (float)sensorValue / 4095.0f * 3.3f;
    float current = (voltage - 1.0f) / 0.25f;
    float normalized = (current - 4.0f) / 16.0f;
    currentActuatorPositions[i] = (normalized - 0.5f) * config.actuatorStrokeCm;
    
    if (isCalibratedArray[i]) {
      float range = actuatorUpperLimit[i] - actuatorLowerLimit[i];
      if (range > 0.01f) {
        calibratedActuatorPositions[i] = 
          (currentActuatorPositions[i] - actuatorCalibratedZero[i]) / (range / 2.0f);
        calibratedActuatorPositions[i] = constrain(calibratedActuatorPositions[i], -1.0f, 1.0f);
      }
    }
  }
}

void ActuatorSystemController::calculateTargetPositions() {
  float pitchRad = radians(targetPitch);
  float rollRad = radians(targetRoll);
  
  float frontLeft = (config.platformLengthCm / 2.0f) * sin(pitchRad) - 
                    (config.platformWidthCm / 2.0f) * sin(rollRad);
  float frontRight = (config.platformLengthCm / 2.0f) * sin(pitchRad) + 
                     (config.platformWidthCm / 2.0f) * sin(rollRad);
  float rearLeft = -(config.platformLengthCm / 2.0f) * sin(pitchRad) - 
                   (config.platformWidthCm / 2.0f) * sin(rollRad);
  float rearRight = -(config.platformLengthCm / 2.0f) * sin(pitchRad) + 
                    (config.platformWidthCm / 2.0f) * sin(rollRad);
  
  float targetCalibratedPositions[4] = {frontLeft, frontRight, rearLeft, rearRight};
  
  for (int i = 0; i < 4; i++) {
    if (isCalibratedArray[i]) {
      float range = actuatorUpperLimit[i] - actuatorLowerLimit[i];
      targetActuatorPositions[i] = actuatorCalibratedZero[i] + (targetCalibratedPositions[i] / (range / 2.0f));
      targetActuatorPositions[i] = constrain(targetActuatorPositions[i], 
                                             actuatorLowerLimit[i], 
                                             actuatorUpperLimit[i]);
    } else {
      targetActuatorPositions[i] = constrain(targetCalibratedPositions[i], 
                                             -config.actuatorStrokeCm/2, 
                                             config.actuatorStrokeCm/2);
    }
  }
}

void ActuatorSystemController::handleCalibrationMode() {
  if (currentCalibrationActuator >= 4) {
    calibrationMode = false;
    return;
  }
  
  bool lowerLimitHit = checkLimitSwitch(config.lowerLimitPins[currentCalibrationActuator]);
  bool upperLimitHit = checkLimitSwitch(config.upperLimitPins[currentCalibrationActuator]);
  
  if (calibratingLower) {
    if (lowerLimitHit) {
      actuatorLowerLimit[currentCalibrationActuator] = currentActuatorPositions[currentCalibrationActuator];
      calibratingLower = false;
    } else {
      analogWrite(config.valvePins[currentCalibrationActuator], 128);
    }
  } else {
    if (upperLimitHit) {
      actuatorUpperLimit[currentCalibrationActuator] = currentActuatorPositions[currentCalibrationActuator];
      actuatorCalibratedZero[currentCalibrationActuator] = 
        (actuatorLowerLimit[currentCalibrationActuator] + actuatorUpperLimit[currentCalibrationActuator]) / 2.0f;
      isCalibratedArray[currentCalibrationActuator] = true;
      currentCalibrationActuator++;
      calibratingLower = true;
      delay(500);
    } else {
      analogWrite(config.valvePins[currentCalibrationActuator], 128);
    }
  }
  
  if (lowerLimitHit || upperLimitHit) {
    analogWrite(config.valvePins[currentCalibrationActuator], 0);
  }
}

void ActuatorSystemController::handleEStopSmoothing() {
  if (!eStopSmoothingActive) {
    for (int i = 0; i < 4; i++) {
      analogWrite(config.valvePins[i], 0);
    }
    return;
  }
  
  unsigned long currentTime = millis();
  float elapsed = (currentTime - eStopSmoothStartTime);
  float progress = elapsed / E_STOP_SMOOTH_DURATION_MS;
  
  if (progress >= 1.0f) {
    eStopSmoothingActive = false;
    if (emergencyStop) {
      for (int i = 0; i < 4; i++) {
        analogWrite(config.valvePins[i], 0);
      }
    }
  } else {
    for (int i = 0; i < 4; i++) {
      float targetPos = emergencyStop ? 0.0f : calibratedActuatorPositions[i];
      float currentPos = eStopSmoothStartPositions[i];
      float smoothedPos = currentPos + (targetPos - currentPos) * progress;
      
      if (isCalibratedArray[i]) {
        float range = actuatorUpperLimit[i] - actuatorLowerLimit[i];
        float rawTarget = actuatorCalibratedZero[i] + (smoothedPos * (range / 2.0f));
        float error = rawTarget - currentActuatorPositions[i];
        float pTerm = config.kp * error * 0.5f;
        int pwmValue = constrain((int)(abs(pTerm) * 25.5f), 0, 128);
        if (error > 0) {
          analogWrite(config.valvePins[i], pwmValue);
        } else {
          analogWrite(config.valvePins[i], 0);
        }
      }
    }
  }
}

void ActuatorSystemController::executePIDControl() {
  unsigned long currentTime = millis();
  float deltaTime = (currentTime - lastUpdateTime) / 1000.0f;
  if (deltaTime <= 0) deltaTime = 0.01f;
  lastUpdateTime = currentTime;
  
  for (int i = 0; i < 4; i++) {
    if (!isCalibratedArray[i]) {
      analogWrite(config.valvePins[i], 0);
      continue;
    }
    
    float error = targetActuatorPositions[i] - currentActuatorPositions[i];
    float pTerm = config.kp * error;
    integralError[i] += error * deltaTime;
    integralError[i] = constrain(integralError[i], -10.0f, 10.0f);
    float iTerm = config.ki * integralError[i];
    float dTerm = config.kd * (error - lastError[i]) / deltaTime;
    lastError[i] = error;
    
    float pidOutput = pTerm + iTerm + dTerm;
    int pwmValue = 0;
    if (abs(pidOutput) > 0.1f) {
      pwmValue = constrain((int)(abs(pidOutput) * 25.5f), 0, 255);
      if (pidOutput < 0) {
        pwmValue = -pwmValue;
      }
    }
    
    if (pwmValue > 0) {
      analogWrite(config.valvePins[i], pwmValue);
    } else {
      analogWrite(config.valvePins[i], 0);
    }
    
    if (config.lowerLimitPins[i] >= 0 && checkLimitSwitch(config.lowerLimitPins[i]) && pwmValue < 0) {
      analogWrite(config.valvePins[i], 0);
    }
    if (config.upperLimitPins[i] >= 0 && checkLimitSwitch(config.upperLimitPins[i]) && pwmValue > 0) {
      analogWrite(config.valvePins[i], 0);
    }
  }
}

bool ActuatorSystemController::checkLimitSwitch(int pin) {
  if (pin < 0) return false;
  return digitalRead(pin) == LOW;
}

#endif // ACTUATOR_SYSTEM_CONTROLLER_H

