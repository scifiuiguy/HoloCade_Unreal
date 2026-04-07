/*
 * HoloCade Kollmorgen AKM Servo Drive Implementation
 * 
 * Concrete implementation of IServoDrive for Kollmorgen AKM series servo motors.
 * Premium option for Flight Sim Experience gyroscope system (military/defense-grade).
 * 
 * Communication: EtherCAT
 * Encoder: Absolute multi-turn encoder (24-bit resolution)
 * 
 * Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.
 */

#ifndef KOLLMORGEN_AKM_DRIVE_H
#define KOLLMORGEN_AKM_DRIVE_H

#include "ServoDrive_Interface.h"
#include <Arduino.h>

class KollmorgenAKMDrive : public IServoDrive {
public:
  KollmorgenAKMDrive();
  virtual ~KollmorgenAKMDrive();
  
  // IServoDrive interface implementation
  virtual bool initialize(const ServoDriveConfig& config) override;
  virtual void shutdown() override;
  virtual bool setControlMode(ServoControlMode mode) override;
  virtual bool setTargetPosition(float positionDegrees) override;
  virtual bool setTargetVelocity(float velocityDegreesPerSecond) override;
  virtual bool setTargetTorque(float torqueNm) override;
  virtual bool enable() override;
  virtual bool disable() override;
  virtual void emergencyStop() override;
  virtual void clearEmergencyStop() override;
  virtual bool getStatus(ServoDriveStatus& status) override;
  virtual bool getCurrentPosition(float& positionDegrees) override;
  virtual bool getCurrentVelocity(float& velocityDegreesPerSecond) override;
  virtual bool getCurrentTorque(float& torqueNm) override;
  virtual bool resetEncoder() override;
  virtual void update() override;
  virtual const char* getDriveTypeName() const override { return "Kollmorgen AKM"; }
  
private:
  ServoDriveConfig config;
  bool initialized;
  bool enabled;
  bool emergencyStopActive;
  ServoControlMode currentControlMode;
  float targetPosition;
  float targetVelocity;
  float targetTorque;
  float currentPosition;
  float currentVelocity;
  float currentTorque;
  unsigned long lastUpdateTime;
  unsigned long lastStatusUpdateTime;
  ServoDriveStatus cachedStatus;
  
  // EtherCAT communication methods
  bool sendEtherCATCommand(uint16_t address, uint32_t data);
  bool readEtherCATRegister(uint16_t address, uint32_t& data);
  bool initializeEtherCAT();
  void updatePositionFromEncoder();
  void updateStatus();
};

// =====================================
// Implementation
// =====================================

KollmorgenAKMDrive::KollmorgenAKMDrive()
  : initialized(false), enabled(false), emergencyStopActive(false),
    currentControlMode(ServoControlMode::Position),
    targetPosition(0.0f), targetVelocity(0.0f), targetTorque(0.0f),
    currentPosition(0.0f), currentVelocity(0.0f), currentTorque(0.0f),
    lastUpdateTime(0), lastStatusUpdateTime(0) {
  memset(&cachedStatus, 0, sizeof(ServoDriveStatus));
}

KollmorgenAKMDrive::~KollmorgenAKMDrive() {
  shutdown();
}

bool KollmorgenAKMDrive::initialize(const ServoDriveConfig& cfg) {
  config = cfg;
  
  if (!initializeEtherCAT()) {
    return false;
  }
  
  if (config.useAbsoluteEncoder) {
    updatePositionFromEncoder();
  }
  
  setControlMode(ServoControlMode::Position);
  initialized = true;
  cachedStatus.isInitialized = true;
  
  return true;
}

void KollmorgenAKMDrive::shutdown() {
  if (initialized) {
    disable();
    emergencyStop();
    initialized = false;
    cachedStatus.isInitialized = false;
  }
}

bool KollmorgenAKMDrive::setControlMode(ServoControlMode mode) {
  if (!initialized) return false;
  currentControlMode = mode;
  
  // Kollmorgen AKD drive EtherCAT PDO mapping
  uint32_t modeValue = (mode == ServoControlMode::Position) ? 0x00000001 :
                       (mode == ServoControlMode::Velocity) ? 0x00000002 : 0x00000003;
  
  return sendEtherCATCommand(0x6040, modeValue);  // Control word register
}

bool KollmorgenAKMDrive::setTargetPosition(float positionDegrees) {
  if (!initialized || !enabled || emergencyStopActive) return false;
  if (currentControlMode != ServoControlMode::Position) {
    if (!setControlMode(ServoControlMode::Position)) return false;
  }
  
  targetPosition = positionDegrees;
  
  // Convert degrees to encoder counts (24-bit encoder = 16,777,216 counts per revolution)
  int32_t encoderCounts = (int32_t)(positionDegrees * 16777216.0f / 360.0f);
  
  return sendEtherCATCommand(0x607A, (uint32_t)encoderCounts);  // Target position register
}

bool KollmorgenAKMDrive::setTargetVelocity(float velocityDegreesPerSecond) {
  if (!initialized || !enabled || emergencyStopActive) return false;
  if (currentControlMode != ServoControlMode::Velocity) {
    if (!setControlMode(ServoControlMode::Velocity)) return false;
  }
  
  targetVelocity = constrain(velocityDegreesPerSecond, -config.maxVelocity, config.maxVelocity);
  float rpm = targetVelocity / 6.0f;
  int32_t rpmValue = (int32_t)constrain(rpm, -3000, 3000);
  
  return sendEtherCATCommand(0x60FF, (uint32_t)rpmValue);  // Target velocity register
}

bool KollmorgenAKMDrive::setTargetTorque(float torqueNm) {
  if (!initialized || !enabled || emergencyStopActive) return false;
  if (currentControlMode != ServoControlMode::Torque) {
    if (!setControlMode(ServoControlMode::Torque)) return false;
  }
  
  targetTorque = constrain(torqueNm, -config.maxTorque, config.maxTorque);
  float torquePercent = (targetTorque / config.maxTorque) * 100.0f;
  int16_t torqueValue = (int16_t)constrain(torquePercent, -100, 100);
  
  return sendEtherCATCommand(0x6071, (uint32_t)torqueValue);  // Target torque register
}

bool KollmorgenAKMDrive::enable() {
  if (!initialized) return false;
  if (sendEtherCATCommand(0x6040, 0x0000000F)) {  // Servo ON sequence
    enabled = true;
    cachedStatus.isEnabled = true;
    return true;
  }
  return false;
}

bool KollmorgenAKMDrive::disable() {
  if (!initialized) return false;
  sendEtherCATCommand(0x6040, 0x00000000);
  enabled = false;
  cachedStatus.isEnabled = false;
  return true;
}

void KollmorgenAKMDrive::emergencyStop() {
  if (!initialized) return;
  sendEtherCATCommand(0x6040, 0x00000002);  // Quick stop
  emergencyStopActive = true;
  enabled = false;
  cachedStatus.isEnabled = false;
}

void KollmorgenAKMDrive::clearEmergencyStop() {
  if (!initialized) return;
  sendEtherCATCommand(0x6040, 0x00000000);
  emergencyStopActive = false;
}

bool KollmorgenAKMDrive::getStatus(ServoDriveStatus& status) {
  if (!initialized) return false;
  updateStatus();
  status = cachedStatus;
  return true;
}

bool KollmorgenAKMDrive::getCurrentPosition(float& positionDegrees) {
  if (!initialized) return false;
  updatePositionFromEncoder();
  positionDegrees = currentPosition;
  return true;
}

bool KollmorgenAKMDrive::getCurrentVelocity(float& velocityDegreesPerSecond) {
  if (!initialized) return false;
  updateStatus();
  velocityDegreesPerSecond = currentVelocity;
  return true;
}

bool KollmorgenAKMDrive::getCurrentTorque(float& torqueNm) {
  if (!initialized) return false;
  updateStatus();
  torqueNm = currentTorque;
  return true;
}

bool KollmorgenAKMDrive::resetEncoder() {
  if (!initialized) return false;
  sendEtherCATCommand(0x6040, 0x00000010);  // Encoder reset command
  currentPosition = 0.0f;
  cachedStatus.currentPosition = 0.0f;
  return true;
}

void KollmorgenAKMDrive::update() {
  if (!initialized) return;
  unsigned long currentTime = millis();
  if (currentTime - lastUpdateTime >= 10) {
    updatePositionFromEncoder();
    lastUpdateTime = currentTime;
  }
  if (currentTime - lastStatusUpdateTime >= 100) {
    updateStatus();
    lastStatusUpdateTime = currentTime;
  }
}

bool KollmorgenAKMDrive::sendEtherCATCommand(uint16_t address, uint32_t data) {
  // TODO: Implement EtherCAT communication for Kollmorgen AKD drives
  return true;
}

bool KollmorgenAKMDrive::readEtherCATRegister(uint16_t address, uint32_t& data) {
  // TODO: Implement EtherCAT register read
  data = 0;
  return true;
}

bool KollmorgenAKMDrive::initializeEtherCAT() {
  // TODO: Initialize EtherCAT master/slave interface for Kollmorgen
  return true;
}

void KollmorgenAKMDrive::updatePositionFromEncoder() {
  uint32_t encoderCounts;
  if (readEtherCATRegister(0x6064, encoderCounts)) {  // Actual position register
    currentPosition = (float)(int32_t)encoderCounts * 360.0f / 16777216.0f;
    cachedStatus.currentPosition = currentPosition;
  }
}

void KollmorgenAKMDrive::updateStatus() {
  uint32_t statusWord;
  if (readEtherCATRegister(0x6041, statusWord)) {  // Status word register
    cachedStatus.isEnabled = (statusWord & 0x0001) != 0;
    cachedStatus.isMoving = (statusWord & 0x0002) != 0;
    cachedStatus.encoderFault = (statusWord & 0x0004) != 0;
    cachedStatus.overcurrentFault = (statusWord & 0x0008) != 0;
    cachedStatus.overtemperatureFault = (statusWord & 0x0010) != 0;
    cachedStatus.errorCode = (statusWord >> 16) & 0xFFFF;
  }
  
  uint32_t velocityValue, torqueValue;
  if (readEtherCATRegister(0x606C, velocityValue)) {  // Actual velocity register
    float rpm = (int32_t)velocityValue;
    currentVelocity = rpm * 6.0f;
    cachedStatus.currentVelocity = currentVelocity;
  }
  if (readEtherCATRegister(0x6077, torqueValue)) {  // Actual torque register
    float torquePercent = (int16_t)torqueValue;
    currentTorque = (torquePercent / 100.0f) * config.maxTorque;
    cachedStatus.currentTorque = currentTorque;
  }
}

#endif // KOLLMORGEN_AKM_DRIVE_H



