/*
 * HoloCade Scissor Lift Controller Header
 * 
 * Reusable header file for controlling electric scissor lifts in motion platforms.
 * Handles vertical translation (TranslationZ) and forward/reverse (TranslationY) commands.
 * 
 * Supports two modes:
 * 1. CAN Bus Mode (default): Communicates with manufacturer ECU via CAN bus (e.g., Genie/Skyjack)
 * 2. Direct GPIO Mode: Direct motor control via GPIO pins (for custom builds or testing)
 * 
 * This header can be included in any sketch to add scissor lift control functionality.
 * Use ScissorLift_Controller.ino for a standalone example.
 * 
 * Usage:
 *   #include "ScissorLift_Controller.h"
 *   
 *   ScissorLiftController liftController;
 *   
 *   void setup() {
 *     liftController.begin(config);
 *   }
 *   
 *   void loop() {
 *     liftController.update();
 *   }
 * 
 * Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.
 */

#ifndef SCISSOR_LIFT_CONTROLLER_H
#define SCISSOR_LIFT_CONTROLLER_H

#include <Arduino.h>
#include "HoloCade_CAN.h"

// Configuration structure for scissor lift
struct ScissorLiftConfig {
  // Control mode
  bool useCANBus;              // true = CAN bus mode (manufacturer ECU), false = direct GPIO mode
  
  // CAN Bus configuration (used when useCANBus = true)
  uint32_t canBaudRate;        // CAN bus baud rate (typically 125000, 250000, or 500000)
  uint32_t canIdJoystick;      // CAN ID for joystick/command messages (manufacturer-specific)
  uint32_t canIdControl;       // CAN ID for control commands (E-stop, etc.)
  uint32_t canIdFeedback;       // CAN ID for position feedback (if available from ECU)
  int canCSPin;                // MCP2515 CS pin (only for MCP2515, default: 10)
  
  // Direct GPIO configuration (used when useCANBus = false)
  int motorUpPin;              // GPIO pin for lift up (direct mode only)
  int motorDownPin;            // GPIO pin for lift down (direct mode only)
  int motorForwardPin;          // GPIO pin for forward drive (direct mode only)
  int motorReversePin;         // GPIO pin for reverse drive (direct mode only)
  
  // Position sensor (may be GPIO analog input or CAN feedback)
  int positionSensorPin;       // GPIO pin for position sensor (analog input, use -1 if using CAN feedback)
  bool useCANFeedback;         // true = read position from CAN bus, false = use GPIO analog input
  
  // Limit switches (optional - may be GPIO or CAN feedback)
  int topLimitPin;             // GPIO pin for top limit switch (optional, use -1 to disable)
  int bottomLimitPin;          // GPIO pin for bottom limit switch (optional, use -1 to disable)
  int forwardLimitPin;         // GPIO pin for forward limit switch (optional, use -1 to disable)
  int reverseLimitPin;         // GPIO pin for reverse limit switch (optional, use -1 to disable)
  
  // Forward/reverse drive configuration (optional)
  bool enableForwardReverse;   // Set to false if bolted to floor
  float maxForwardReverseCm;   // Maximum forward/reverse travel in cm (safety limit)
  
  // Motion parameters
  float maxHeightCm;           // Maximum lift height in cm
  float minHeightCm;           // Minimum lift height in cm
  float softwareUpperLimitCm; // Software-defined upper limit in cm (virtual limit)
  float maxSpeedCmPerSec;      // Maximum lift speed (cm/second)
  
  // Operation mode
  bool autoCalibrateMode;      // true = auto-calibrate mode, false = fixed mode
  unsigned long autoCalibrateTimeoutMs; // Timeout for auto-calibrate mode
};

class ScissorLiftController {
public:
  ScissorLiftController();
  
  // Initialize the controller with configuration
  void begin(const ScissorLiftConfig& config);
  
  // Main update loop - call this in your loop() function
  void update();
  
  // Command handlers - call these from your HoloCade_HandleFloat/Bool functions
  void handleFloatCommand(uint8_t channel, float value);
  void handleBoolCommand(uint8_t channel, bool value);
  
  // Get current state
  float getCurrentHeight() const { return currentHeight; }
  float getTargetHeight() const { return targetHeight; }
  float getCurrentForwardPosition() const { return currentForwardPosition; }
  float getTargetForwardPosition() const { return targetForwardPosition; }
  bool isEmergencyStop() const { return emergencyStop; }
  bool isCalibrated() const { return isCalibratedFlag; }
  
  // Manual control (optional - for direct control without HoloCade commands)
  void setTargetHeight(float height);
  void setTargetForwardPosition(float position);
  void setEmergencyStop(bool stop);
  void setCalibrationMode(bool enable);
  void setOperationMode(bool autoCalibrate);
  void returnToNeutral();
  
private:
  // Configuration
  ScissorLiftConfig config;
  bool initialized;
  
  // Calibration parameters
  float calibratedZeroHeight;
  float calibratedBottomLimit;
  float calibratedUpperLimit;
  bool isCalibratedFlag;
  float forwardReverseZeroPosition;
  
  // E-stop smoothing
  bool emergencyStop;
  bool eStopSmoothingActive;
  float eStopSmoothStartHeight;
  float eStopSmoothStartForward;
  unsigned long eStopSmoothStartTime;
  static const float E_STOP_SMOOTH_DURATION_MS;
  
  // State tracking
  float currentHeight;
  float targetHeight;
  float currentForwardPosition;
  float targetForwardPosition;
  bool isMoving;
  bool isMovingForward;
  unsigned long lastCommandTime;
  unsigned long lastForwardUpdate;
  
  // Calibration mode
  bool calibrationMode;
  bool calibratingVertical;
  
  // Internal methods
  void updatePositionSensor();
  void checkLimitSwitches();
  void handleCalibration();
  void executeMotion();
  void executeForwardReverseMotion();
  void moveUp();
  void moveDown();
  void stopMotor();
  void stopForwardReverse();
  bool checkTopLimit();
  bool checkBottomLimit();
  void handleEStopSmoothing();
};

// Implementation
const float ScissorLiftController::E_STOP_SMOOTH_DURATION_MS = 500.0f;

ScissorLiftController::ScissorLiftController()
  : initialized(false), emergencyStop(false), eStopSmoothingActive(false),
    calibratedZeroHeight(0.0f), calibratedBottomLimit(0.0f), 
    calibratedUpperLimit(90.0f), isCalibratedFlag(false),
    forwardReverseZeroPosition(0.0f),
    currentHeight(0.0f), targetHeight(0.0f),
    currentForwardPosition(0.0f), targetForwardPosition(0.0f),
    isMoving(false), isMovingForward(false),
    lastCommandTime(0), lastForwardUpdate(0),
    calibrationMode(false), calibratingVertical(true) {
}

void ScissorLiftController::begin(const ScissorLiftConfig& cfg) {
  config = cfg;
  
  if (config.useCANBus) {
    // Initialize CAN bus communication
    if (!HoloCade_CAN_Init(config.canBaudRate, config.canCSPin)) {
      Serial.println("ScissorLiftController: CAN bus initialization failed!");
      initialized = false;
      return;
    }
    Serial.println("ScissorLiftController: CAN bus mode initialized");
    Serial.printf("  Joystick CAN ID: 0x%03X\n", config.canIdJoystick);
    Serial.printf("  Control CAN ID: 0x%03X\n", config.canIdControl);
    Serial.printf("  Feedback CAN ID: 0x%03X\n", config.canIdFeedback);
  } else {
    // Direct GPIO mode - configure motor control pins
    pinMode(config.motorUpPin, OUTPUT);
    pinMode(config.motorDownPin, OUTPUT);
    digitalWrite(config.motorUpPin, LOW);
    digitalWrite(config.motorDownPin, LOW);
    
    // Configure forward/reverse drive pins (if enabled)
    if (config.enableForwardReverse) {
      pinMode(config.motorForwardPin, OUTPUT);
      pinMode(config.motorReversePin, OUTPUT);
      digitalWrite(config.motorForwardPin, LOW);
      digitalWrite(config.motorReversePin, LOW);
      
      if (config.forwardLimitPin >= 0) {
        pinMode(config.forwardLimitPin, INPUT_PULLUP);
      }
      if (config.reverseLimitPin >= 0) {
        pinMode(config.reverseLimitPin, INPUT_PULLUP);
      }
    }
    Serial.println("ScissorLiftController: Direct GPIO mode initialized");
  }
  
  // Configure position sensor (if using GPIO, not CAN feedback)
  if (!config.useCANFeedback && config.positionSensorPin >= 0) {
    pinMode(config.positionSensorPin, INPUT);
  }
  
  // Configure limit switches (if using GPIO, not CAN feedback)
  if (config.topLimitPin >= 0) {
    pinMode(config.topLimitPin, INPUT_PULLUP);
  }
  if (config.bottomLimitPin >= 0) {
    pinMode(config.bottomLimitPin, INPUT_PULLUP);
  }
  
  // Initialize calibration
  calibratedZeroHeight = config.minHeightCm;
  calibratedBottomLimit = config.minHeightCm;
  calibratedUpperLimit = config.softwareUpperLimitCm;
  currentHeight = config.minHeightCm;
  targetHeight = config.minHeightCm;
  
  lastCommandTime = millis();
  initialized = true;
}

void ScissorLiftController::update() {
  if (!initialized) return;
  
  // Update position sensor reading
  updatePositionSensor();
  
  // Check limit switches
  checkLimitSwitches();
  
  // Check auto-calibrate timeout
  if (!emergencyStop && config.autoCalibrateMode) {
    unsigned long timeSinceLastCommand = millis() - lastCommandTime;
    if (timeSinceLastCommand > config.autoCalibrateTimeoutMs) {
      targetHeight = calibratedZeroHeight;
      targetForwardPosition = forwardReverseZeroPosition;
    }
  }
  
  // Execute motion if not in emergency stop
  if (!emergencyStop) {
    executeMotion();
    if (config.enableForwardReverse) {
      executeForwardReverseMotion();
    }
  } else {
    handleEStopSmoothing();
  }
}

void ScissorLiftController::handleFloatCommand(uint8_t channel, float value) {
  lastCommandTime = millis();
  
  if (channel == 0) {
    // Channel 0: Vertical translation command
    if (value >= -1.0f && value <= 1.0f && abs(value) < 0.01f) {
      targetHeight = calibratedZeroHeight;
    } else if (value >= -1.0f && value <= 1.0f) {
      if (isCalibratedFlag) {
        targetHeight = calibratedZeroHeight + (value * calibratedUpperLimit);
      } else {
        targetHeight = config.minHeightCm + (value * config.softwareUpperLimitCm);
      }
    } else {
      targetHeight = value;
    }
    
    if (isCalibratedFlag) {
      targetHeight = constrain(targetHeight, calibratedZeroHeight, calibratedZeroHeight + calibratedUpperLimit);
    } else {
      targetHeight = constrain(targetHeight, config.minHeightCm, config.minHeightCm + config.softwareUpperLimitCm);
    }
  } else if (channel == 1 && config.enableForwardReverse) {
    // Channel 1: Forward/reverse translation command
    if (value >= -1.0f && value <= 1.0f && abs(value) < 0.01f) {
      targetForwardPosition = 0.0f;
    } else if (value >= -1.0f && value <= 1.0f) {
      targetForwardPosition = value * config.maxForwardReverseCm;
    } else {
      targetForwardPosition = value;
    }
    
    targetForwardPosition = constrain(targetForwardPosition, 
                                     -config.maxForwardReverseCm, 
                                     config.maxForwardReverseCm);
  }
}

void ScissorLiftController::handleBoolCommand(uint8_t channel, bool value) {
  lastCommandTime = millis();
  
  if (channel == 2) {
    // Channel 2: Calibration mode
    if (value) {
      calibrationMode = true;
      calibratingVertical = true;
      if (config.enableForwardReverse) {
        forwardReverseZeroPosition = currentForwardPosition;
      }
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
        eStopSmoothStartHeight = currentHeight;
        eStopSmoothStartForward = currentForwardPosition;
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

void ScissorLiftController::setTargetHeight(float height) {
  targetHeight = height;
  lastCommandTime = millis();
}

void ScissorLiftController::setTargetForwardPosition(float position) {
  targetForwardPosition = position;
  lastCommandTime = millis();
}

void ScissorLiftController::setEmergencyStop(bool stop) {
  if (stop && !emergencyStop) {
    eStopSmoothStartHeight = currentHeight;
    eStopSmoothStartForward = currentForwardPosition;
    eStopSmoothStartTime = millis();
    eStopSmoothingActive = true;
    
    // Send E-stop command via CAN bus if in CAN mode
    if (config.useCANBus) {
      HoloCade_CAN_SendLiftEmergencyStop(true, config.canIdControl);
    }
  }
  emergencyStop = stop;
  if (!stop) {
    eStopSmoothingActive = true;
    eStopSmoothStartTime = millis();
    
    // Release E-stop via CAN bus if in CAN mode
    if (config.useCANBus) {
      HoloCade_CAN_SendLiftEmergencyStop(false, config.canIdControl);
    }
  }
}

void ScissorLiftController::setCalibrationMode(bool enable) {
  calibrationMode = enable;
  if (enable) {
    calibratingVertical = true;
    if (config.enableForwardReverse) {
      forwardReverseZeroPosition = currentForwardPosition;
    }
  }
}

void ScissorLiftController::setOperationMode(bool autoCalibrate) {
  config.autoCalibrateMode = autoCalibrate;
}

void ScissorLiftController::returnToNeutral() {
  targetHeight = calibratedZeroHeight;
  targetForwardPosition = forwardReverseZeroPosition;
  emergencyStop = false;
  eStopSmoothingActive = false;
  lastCommandTime = millis();
}

void ScissorLiftController::updatePositionSensor() {
  if (config.useCANFeedback) {
    // TODO: Implement CAN bus feedback reading
    // This would require implementing CAN message reception in HoloCade_CAN.h
    // For now, we'll use a placeholder that reads from CAN feedback messages
    // Developers should implement CAN message reception based on their ECU's protocol
    // Example: Parse CAN message from config.canIdFeedback and extract height/position
    Serial.println("ScissorLiftController: CAN feedback mode - implement CAN message reception");
  } else if (config.positionSensorPin >= 0) {
    // GPIO analog input mode
    int sensorValue = analogRead(config.positionSensorPin);
    float normalized = (float)sensorValue / 4095.0f;
    currentHeight = config.minHeightCm + (normalized * (config.maxHeightCm - config.minHeightCm));
  }
  
  // Update forward/reverse position (simplified - may need CAN feedback for accuracy)
  unsigned long currentTime = millis();
  float deltaTime = (currentTime - lastForwardUpdate) / 1000.0f;
  if (deltaTime > 0.01f && config.enableForwardReverse) {
    lastForwardUpdate = currentTime;
    
    if (!config.useCANBus && !config.useCANFeedback) {
      // Direct GPIO mode - estimate position from motor state
      if (digitalRead(config.motorForwardPin) == HIGH) {
        currentForwardPosition += 5.0f * deltaTime;
      } else if (digitalRead(config.motorReversePin) == HIGH) {
        currentForwardPosition -= 5.0f * deltaTime;
      }
    }
    // In CAN mode, forward/reverse position should ideally come from CAN feedback
    // For now, we estimate based on target (less accurate)
    
    currentForwardPosition = constrain(currentForwardPosition, 
                                       forwardReverseZeroPosition - config.maxForwardReverseCm, 
                                       forwardReverseZeroPosition + config.maxForwardReverseCm);
  }
}

void ScissorLiftController::checkLimitSwitches() {
  if (isMoving) {
    if (checkTopLimit()) {
      if (config.useCANBus) {
        // In CAN mode, stop command is sent via CAN bus
        stopMotor();
      } else if (digitalRead(config.motorUpPin) == HIGH) {
        stopMotor();
      }
    }
    if (checkBottomLimit()) {
      if (config.useCANBus) {
        // In CAN mode, stop command is sent via CAN bus
        stopMotor();
      } else if (digitalRead(config.motorDownPin) == HIGH) {
        stopMotor();
      }
    }
  }
  
  if (isMovingForward && config.enableForwardReverse) {
    if (config.forwardLimitPin >= 0 && digitalRead(config.forwardLimitPin) == LOW) {
      if (config.useCANBus || digitalRead(config.motorForwardPin) == HIGH) {
        stopForwardReverse();
      }
    }
    if (config.reverseLimitPin >= 0 && digitalRead(config.reverseLimitPin) == LOW) {
      if (config.useCANBus || digitalRead(config.motorReversePin) == HIGH) {
        stopForwardReverse();
      }
    }
  }
}

void ScissorLiftController::handleCalibration() {
  if (!calibrationMode) return;
  
  if (calibratingVertical) {
    bool bottomLimitHit = checkBottomLimit();
    
    if (bottomLimitHit) {
      calibratedBottomLimit = currentHeight;
      calibratedZeroHeight = currentHeight;
      isCalibratedFlag = true;
      calibrationMode = false;
    } else {
      moveDown();
    }
  }
}

void ScissorLiftController::executeMotion() {
  if (calibrationMode) {
    handleCalibration();
    return;
  }
  
  float heightError = targetHeight - currentHeight;
  float tolerance = 1.0f;
  
  if (abs(heightError) < tolerance) {
    stopMotor();
    return;
  }
  
  if (heightError > 0) {
    if (!checkTopLimit()) {
      moveUp();
    } else {
      stopMotor();
    }
  } else {
    if (!checkBottomLimit()) {
      moveDown();
    } else {
      stopMotor();
    }
  }
}

void ScissorLiftController::executeForwardReverseMotion() {
  if (!config.enableForwardReverse || calibrationMode) return;
  
  float forwardError = targetForwardPosition - currentForwardPosition;
  float tolerance = 0.5f;
  
  if (abs(forwardError) < tolerance) {
    stopForwardReverse();
    return;
  }
  
  if (forwardError > 0 && currentForwardPosition >= forwardReverseZeroPosition + config.maxForwardReverseCm) {
    stopForwardReverse();
    return;
  }
  if (forwardError < 0 && currentForwardPosition <= forwardReverseZeroPosition - config.maxForwardReverseCm) {
    stopForwardReverse();
    return;
  }
  
  if (config.useCANBus) {
    // CAN bus mode - send joystick command with forward/reverse component
    float verticalCommand = 0.0f;
    if (isCalibratedFlag) {
      float heightRange = calibratedUpperLimit;
      if (heightRange > 0.01f) {
        verticalCommand = (targetHeight - calibratedZeroHeight) / heightRange;
      }
    } else {
      float heightRange = config.softwareUpperLimitCm;
      if (heightRange > 0.01f) {
        verticalCommand = (targetHeight - config.minHeightCm) / heightRange;
      }
    }
    verticalCommand = constrain(verticalCommand, -1.0f, 1.0f);
    
    float forwardCommand = (targetForwardPosition - forwardReverseZeroPosition) / config.maxForwardReverseCm;
    forwardCommand = constrain(forwardCommand, -1.0f, 1.0f);
    
    HoloCade_CAN_SendLiftJoystickCommand(verticalCommand, forwardCommand, config.canIdJoystick);
    isMovingForward = true;
  } else {
    // Direct GPIO mode
    if (forwardError > 0) {
      if (config.forwardLimitPin >= 0 && digitalRead(config.forwardLimitPin) == LOW) {
        stopForwardReverse();
      } else {
        digitalWrite(config.motorReversePin, LOW);
        digitalWrite(config.motorForwardPin, HIGH);
        isMovingForward = true;
      }
    } else {
      if (config.reverseLimitPin >= 0 && digitalRead(config.reverseLimitPin) == LOW) {
        stopForwardReverse();
      } else {
        digitalWrite(config.motorForwardPin, LOW);
        digitalWrite(config.motorReversePin, HIGH);
        isMovingForward = true;
      }
    }
  }
}

void ScissorLiftController::moveUp() {
  if (config.useCANBus) {
    // Send CAN command for upward movement
    // Convert target height to joystick command (-1.0 to +1.0)
    float verticalCommand = 0.0f;
    if (isCalibratedFlag) {
      float heightRange = calibratedUpperLimit;
      if (heightRange > 0.01f) {
        verticalCommand = (targetHeight - calibratedZeroHeight) / heightRange;
      }
    } else {
      float heightRange = config.softwareUpperLimitCm;
      if (heightRange > 0.01f) {
        verticalCommand = (targetHeight - config.minHeightCm) / heightRange;
      }
    }
    verticalCommand = constrain(verticalCommand, -1.0f, 1.0f);
    
    // Send joystick command via CAN bus
    float forwardCommand = 0.0f;
    if (config.enableForwardReverse) {
      forwardCommand = (targetForwardPosition - forwardReverseZeroPosition) / config.maxForwardReverseCm;
      forwardCommand = constrain(forwardCommand, -1.0f, 1.0f);
    }
    
    HoloCade_CAN_SendLiftJoystickCommand(verticalCommand, forwardCommand, config.canIdJoystick);
  } else {
    // Direct GPIO mode
    digitalWrite(config.motorDownPin, LOW);
    digitalWrite(config.motorUpPin, HIGH);
  }
  isMoving = true;
}

void ScissorLiftController::moveDown() {
  if (config.useCANBus) {
    // Send CAN command for downward movement (same as moveUp, but with negative command)
    float verticalCommand = 0.0f;
    if (isCalibratedFlag) {
      float heightRange = calibratedUpperLimit;
      if (heightRange > 0.01f) {
        verticalCommand = (targetHeight - calibratedZeroHeight) / heightRange;
      }
    } else {
      float heightRange = config.softwareUpperLimitCm;
      if (heightRange > 0.01f) {
        verticalCommand = (targetHeight - config.minHeightCm) / heightRange;
      }
    }
    verticalCommand = constrain(verticalCommand, -1.0f, 1.0f);
    
    float forwardCommand = 0.0f;
    if (config.enableForwardReverse) {
      forwardCommand = (targetForwardPosition - forwardReverseZeroPosition) / config.maxForwardReverseCm;
      forwardCommand = constrain(forwardCommand, -1.0f, 1.0f);
    }
    
    HoloCade_CAN_SendLiftJoystickCommand(verticalCommand, forwardCommand, config.canIdJoystick);
  } else {
    // Direct GPIO mode
    digitalWrite(config.motorUpPin, LOW);
    digitalWrite(config.motorDownPin, HIGH);
  }
  isMoving = true;
}

void ScissorLiftController::stopMotor() {
  if (config.useCANBus) {
    // Send zero command via CAN bus (stop vertical movement)
    HoloCade_CAN_SendLiftJoystickCommand(0.0f, 0.0f, config.canIdJoystick);
  } else {
    // Direct GPIO mode
    digitalWrite(config.motorUpPin, LOW);
    digitalWrite(config.motorDownPin, LOW);
  }
  isMoving = false;
}

void ScissorLiftController::stopForwardReverse() {
  if (config.enableForwardReverse) {
    if (config.useCANBus) {
      // Send zero forward/reverse command via CAN bus
      float verticalCommand = 0.0f;
      if (isCalibratedFlag) {
        float heightRange = calibratedUpperLimit;
        if (heightRange > 0.01f) {
          verticalCommand = (targetHeight - calibratedZeroHeight) / heightRange;
        }
      } else {
        float heightRange = config.softwareUpperLimitCm;
        if (heightRange > 0.01f) {
          verticalCommand = (targetHeight - config.minHeightCm) / heightRange;
        }
      }
      verticalCommand = constrain(verticalCommand, -1.0f, 1.0f);
      
      HoloCade_CAN_SendLiftJoystickCommand(verticalCommand, 0.0f, config.canIdJoystick);
    } else {
      // Direct GPIO mode
      digitalWrite(config.motorForwardPin, LOW);
      digitalWrite(config.motorReversePin, LOW);
    }
    isMovingForward = false;
  }
}

bool ScissorLiftController::checkTopLimit() {
  if (config.topLimitPin >= 0) {
    return digitalRead(config.topLimitPin) == LOW;
  }
  if (isCalibratedFlag) {
    return currentHeight >= (calibratedZeroHeight + calibratedUpperLimit - 5.0f);
  }
  return currentHeight >= (config.minHeightCm + config.softwareUpperLimitCm - 5.0f);
}

bool ScissorLiftController::checkBottomLimit() {
  if (config.bottomLimitPin >= 0) {
    return digitalRead(config.bottomLimitPin) == LOW;
  }
  if (isCalibratedFlag) {
    return currentHeight <= (calibratedBottomLimit + 5.0f);
  }
  return currentHeight <= (config.minHeightCm + 5.0f);
}

void ScissorLiftController::handleEStopSmoothing() {
  if (!eStopSmoothingActive) {
    stopMotor();
    stopForwardReverse();
    return;
  }
  
  unsigned long currentTime = millis();
  float elapsed = (currentTime - eStopSmoothStartTime);
  float progress = elapsed / E_STOP_SMOOTH_DURATION_MS;
  
  if (progress >= 1.0f) {
    eStopSmoothingActive = false;
    stopMotor();
    stopForwardReverse();
  } else {
    float targetHeightSmooth = eStopSmoothStartHeight + 
                               (calibratedZeroHeight - eStopSmoothStartHeight) * progress;
    float targetForwardSmooth = eStopSmoothStartForward + 
                                (forwardReverseZeroPosition - eStopSmoothStartForward) * progress;
    
    float oldTargetHeight = targetHeight;
    float oldTargetForward = targetForwardPosition;
    
    targetHeight = targetHeightSmooth;
    targetForwardPosition = targetForwardSmooth;
    
    executeMotion();
    if (config.enableForwardReverse) {
      executeForwardReverseMotion();
    }
    
    targetHeight = oldTargetHeight;
    targetForwardPosition = oldTargetForward;
  }
}

#endif // SCISSOR_LIFT_CONTROLLER_H

