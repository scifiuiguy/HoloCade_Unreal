/*
 * HoloCade Servo Drive Interface
 * 
 * Abstract interface for professional servo motor control.
 * Supports multiple servo drive brands (Yaskawa, Panasonic, Kollmorgen)
 * with a unified API for gyroscope control.
 * 
 * This interface allows the gyroscope controller to work with any
 * professional servo system without code changes.
 * 
 * Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.
 */

#ifndef SERVO_DRIVE_INTERFACE_H
#define SERVO_DRIVE_INTERFACE_H

#include <Arduino.h>

// Control modes for servo drives
enum class ServoControlMode {
  Position,   // Position control (absolute angle)
  Velocity,   // Velocity control (degrees per second)
  Torque      // Torque control (Nm)
};

// Drive status information
struct ServoDriveStatus {
  bool isInitialized;           // Drive initialized and ready
  bool isEnabled;                // Drive enabled (not in emergency stop)
  bool isMoving;                 // Drive is currently moving
  float currentPosition;         // Current absolute position (degrees)
  float currentVelocity;         // Current velocity (degrees per second)
  float currentTorque;           // Current torque (Nm)
  bool encoderFault;             // Encoder fault detected
  bool overcurrentFault;         // Overcurrent fault detected
  bool overtemperatureFault;     // Overtemperature fault detected
  uint32_t errorCode;            // Drive-specific error code (0 = no error)
};

// Drive configuration
struct ServoDriveConfig {
  // Communication settings (varies by drive type)
  // For EtherCAT: IP address, node ID
  // For MECHATROLINK: station number
  // For analog: pin assignments
  uint8_t nodeId;               // Network node ID (EtherCAT/MECHATROLINK)
  IPAddress ipAddress;           // IP address (if using Ethernet)
  
  // Motion limits
  float maxVelocity;             // Maximum velocity (degrees per second)
  float maxAcceleration;         // Maximum acceleration (degrees per second squared)
  float maxTorque;               // Maximum torque (Nm)
  
  // Encoder settings
  bool useAbsoluteEncoder;       // Use absolute encoder (multi-turn)
  uint16_t encoderResolution;     // Encoder resolution (bits, typically 20-24)
  
  // Safety settings
  bool enableBrake;              // Enable motor brake
  bool enableSoftLimits;         // Enable software position limits
  float softLimitMin;            // Minimum soft limit (degrees)
  float softLimitMax;            // Maximum soft limit (degrees)
};

// Abstract base class for servo drive control
class IServoDrive {
public:
  virtual ~IServoDrive() {}
  
  // Initialize the servo drive with configuration
  // Returns true if initialization successful
  virtual bool initialize(const ServoDriveConfig& config) = 0;
  
  // Shutdown and disable the drive
  virtual void shutdown() = 0;
  
  // Set control mode (position, velocity, or torque)
  virtual bool setControlMode(ServoControlMode mode) = 0;
  
  // Position control: Set target position (degrees, absolute)
  // For continuous rotation, values can exceed 360°
  virtual bool setTargetPosition(float positionDegrees) = 0;
  
  // Velocity control: Set target velocity (degrees per second)
  // Positive = clockwise, negative = counter-clockwise
  virtual bool setTargetVelocity(float velocityDegreesPerSecond) = 0;
  
  // Torque control: Set target torque (Nm)
  virtual bool setTargetTorque(float torqueNm) = 0;
  
  // Enable/disable the drive
  virtual bool enable() = 0;
  virtual bool disable() = 0;
  
  // Emergency stop (immediately stop motion)
  virtual void emergencyStop() = 0;
  virtual void clearEmergencyStop() = 0;
  
  // Get current drive status
  virtual bool getStatus(ServoDriveStatus& status) = 0;
  
  // Get current absolute position (degrees)
  // Returns true if position is valid
  virtual bool getCurrentPosition(float& positionDegrees) = 0;
  
  // Get current velocity (degrees per second)
  virtual bool getCurrentVelocity(float& velocityDegreesPerSecond) = 0;
  
  // Get current torque (Nm)
  virtual bool getCurrentTorque(float& torqueNm) = 0;
  
  // Reset encoder to zero position (for absolute encoders, this sets current position as zero)
  virtual bool resetEncoder() = 0;
  
  // Update drive (call this in main loop for drives that need periodic updates)
  virtual void update() = 0;
  
  // Get drive type name (for debugging)
  virtual const char* getDriveTypeName() const = 0;
};

#endif // SERVO_DRIVE_INTERFACE_H



