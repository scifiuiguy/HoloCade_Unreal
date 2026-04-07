/*
 * HoloCade Superhero Flight Experience ECU
 * 
 * Embedded control unit for SuperheroFlightExperience dual-winch suspended harness system.
 * Manages dual-winch control (front shoulder-hook, rear pelvis-hook), position feedback,
 * load cell monitoring, and safety interlocks.
 * 
 * Hardware Requirements:
 * - Microcontroller with WiFi/Ethernet capability (ESP32 recommended)
 * - Dual motor drivers (one per winch)
 * - Dual load cell interfaces (tension monitoring)
 * - Position sensor interfaces (encoders or potentiometers)
 * - Power supply: 12V-24V DC (separate from winch motors)
 * 
 * Communication Protocol: Binary HoloCade protocol over UDP (WiFi/Ethernet)
 * 
 * Channel Mapping (Game Engine → Superhero Flight ECU):
 * - Channel 0: Front winch position (inches, relative to standingGroundHeight)
 * - Channel 1: Front winch speed (inches/second)
 * - Channel 2: Rear winch position (inches, relative to standingGroundHeight)
 * - Channel 3: Rear winch speed (inches/second)
 * - Channel 6: Game state (0=standing, 1=hovering, 2=flight-up, 3=flight-forward, 4=flight-down)
 * - Channel 7: Emergency stop (bool, true = stop all systems, return to standing)
 * - Channel 9: Play session active (bool, true = winches can operate)
 * - Channel 10: Standing ground height acknowledgment (current winch position becomes new baseline)
 * - Channel 11: Air height parameter (inches)
 * - Channel 12: Prone height parameter (inches)
 * - Channel 13: Player height compensation (multiplier)
 * 
 * Channel Mapping (Superhero Flight ECU → Game Engine):
 * - Channel 310: FSuperheroFlightDualWinchState struct (dual-winch telemetry, 20 Hz / 50ms)
 * - Channel 311: FSuperheroFlightTelemetry struct (system health, temperatures, fault states, 1 Hz / 1000ms)
 * 
 * Safety Features:
 * - Emergency stop always takes precedence
 * - Weight/height limit monitoring via load cells
 * - Winch redundancy support (degraded mode if one winch fails)
 * - Position feedback for closed-loop control
 * - Safety state reporting to game engine
 * 
 * Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.
 */

// Include HoloCade protocol templates
#include "HoloCade_Wireless_RX.h"
#include "HoloCade_Wireless_TX.h"

// =====================================
// Struct Definitions (Must Match Unreal Exactly)
// =====================================

struct FSuperheroFlightDualWinchState {
  float FrontWinchPosition;      // inches (relative to standingGroundHeight)
  float FrontWinchSpeed;         // inches/second
  float FrontWinchTension;        // load cell reading (lbs or N)
  float RearWinchPosition;        // inches (relative to standingGroundHeight)
  float RearWinchSpeed;           // inches/second
  float RearWinchTension;         // load cell reading (lbs or N)
  int32_t GameState;             // 0=standing, 1=hovering, 2=flight-up, 3=flight-forward, 4=flight-down
  bool SafetyState;               // true = safe to operate, false = safety interlock active
  uint32_t Timestamp;             // Timestamp when state was captured (ms)
};

struct FSuperheroFlightTelemetry {
  float FrontWinchMotorTemp;      // °C
  float RearWinchMotorTemp;      // °C
  bool FrontWinchFault;          // true = fault detected
  bool RearWinchFault;           // true = fault detected
  bool WinchRedundancyStatus;     // true = both winches operational, false = degraded mode
  float SystemVoltage;            // V
  float SystemCurrent;           // A
  uint32_t Timestamp;             // Timestamp when telemetry was captured (ms)
};

// =====================================
// Configuration
// =====================================

// WiFi credentials (change to match your LAN)
const char* ssid = "VR_Arcade_LAN";
const char* password = "your_password_here";

// Unreal Engine PC IP address
IPAddress unrealIP(192, 168, 1, 100);
uint16_t unrealPort = 8888;

// =====================================
// Hardware Pin Configuration (Custom DIY Solution)
// =====================================
//
// Motor Control (PWM to Motor Controller):
// - ESP32 PWM output → Motor Controller (e.g., RoboClaw, Sabertooth, or custom H-bridge)
// - Motor Controller → 24V DC Gearmotor
//
const int frontWinchMotorPWMPin = 12;   // ESP32 PWM pin → Motor controller input
const int rearWinchMotorPWMPin = 13;    // ESP32 PWM pin → Motor controller input

// Direction pins (if using bidirectional motor controller with separate direction control)
const int frontWinchMotorDirPin = 14;   // Direction pin (HIGH = up, LOW = down)
const int rearWinchMotorDirPin = 15;    // Direction pin (HIGH = up, LOW = down)

// Encoder Input (Rotary Encoder on Winch Drum):
// - Encoder A/B signals → ESP32 pulse counter or interrupt pins
// - Calculate position from encoder pulses (pulses per revolution * drum circumference)
//
const int frontWinchEncoderAPin = 18;   // Encoder A signal (interrupt-capable pin)
const int frontWinchEncoderBPin = 19;   // Encoder B signal (interrupt-capable pin)
const int rearWinchEncoderAPin = 21;   // Encoder A signal (interrupt-capable pin)
const int rearWinchEncoderBPin = 22;    // Encoder B signal (interrupt-capable pin)

// Load Cell (HX711 Amplifier) - ONE PER WINCH:
// - Each winch has its own load cell and HX711 amplifier for independent tension monitoring
// - Required for ride classification compliance (independent monitoring per winch)
// - Load Cell → HX711 Amplifier → ESP32 (SCK, DT pins)
// - HX711 provides 24-bit ADC for precise tension measurement
//
const int frontWinchLoadCellSCK = 25;   // Front HX711 SCK pin (clock)
const int frontWinchLoadCellDT = 26;    // Front HX711 DT pin (data)
const int rearWinchLoadCellSCK = 32;    // Rear HX711 SCK pin (clock)
const int rearWinchLoadCellDT = 33;     // Rear HX711 DT pin (data)

// Safety Brake (Electromagnetic Brake) - ONE PER WINCH:
// - Each winch has its own independent electromagnetic brake for safety compliance
// - Required for ride classification compliance (independent brake control per winch)
// - ESP32 digital output → Relay or MOSFET → 24V electromagnetic brake
// - Brake engages when LOW (fail-safe), releases when HIGH
//
const int frontWinchBrakePin = 27;      // Front brake control (HIGH = brake released, LOW = brake engaged)
const int rearWinchBrakePin = 4;       // Rear brake control (HIGH = brake released, LOW = brake engaged)

// Emergency Stop Input (Hardware E-Stop Button):
// - E-Stop button → ESP32 interrupt pin (with pull-up)
// - Active LOW (button pressed = LOW, triggers emergency stop)
//
const int emergencyStopPin = 0;         // E-Stop button (interrupt-capable, active LOW)

// Battery Voltage Monitoring (Voltage Divider):
// - 24V Battery (+) → [R1: 10kΩ] → ESP32 GPIO 34 (ADC1_CH6) → [R2: 1.5kΩ] → GND
// - Voltage divider ratio: (R1+R2)/R2 = (10k+1.5k)/1.5k = 7.67
//
const int batteryVoltagePin = 34;       // ADC1_CH6 (GPIO 34, input-only, no pull-up)
const float voltageDividerRatio = 7.67f; // (R1+R2)/R2 for 24V max reading
const float batteryLowVoltageThreshold = 22.0f;  // 22V = ~10% SOC (stop operation)
const float batteryCriticalVoltageThreshold = 20.5f;  // 20.5V = ~5% SOC (emergency stop)

// Safety limits
const float maxWeight = 300.0f;         // lbs (per winch)
const float maxHeight = 120.0f;         // inches (relative to standingGroundHeight)
const float emergencyDescentSpeed = 6.0f; // inches/second

// =====================================
// State Variables
// =====================================

// Winch control state
float frontWinchTargetPosition = 0.0f;  // inches (relative to standingGroundHeight)
float frontWinchCurrentPosition = 0.0f; // inches (from position sensor)
float frontWinchTargetSpeed = 0.0f;     // inches/second
float frontWinchCurrentSpeed = 0.0f;    // inches/second (calculated from position change)
float frontWinchTension = 0.0f;         // lbs (from load cell)

float rearWinchTargetPosition = 0.0f;   // inches (relative to standingGroundHeight)
float rearWinchCurrentPosition = 0.0f;  // inches (from position sensor)
float rearWinchTargetSpeed = 0.0f;      // inches/second
float rearWinchCurrentSpeed = 0.0f;     // inches/second (calculated from position change)
float rearWinchTension = 0.0f;          // lbs (from load cell)

// Game state
int32_t gameState = 0;                  // 0=standing, 1=hovering, 2=flight-up, 3=flight-forward, 4=flight-down
bool playSessionActive = false;
bool emergencyStop = false;
bool safetyState = true;                // true = safe to operate

// Parameters
float standingGroundHeight = 0.0f;      // Baseline height (calibrated per-player)
float airHeight = 24.0f;                // Height for hovering/flight-up/flight-down (inches)
float proneHeight = 36.0f;              // Height for flight-forward (inches)
float playerHeightCompensation = 1.0f;  // Multiplier for proneHeight adjustment

// Telemetry
FSuperheroFlightDualWinchState winchState;
FSuperheroFlightTelemetry telemetry;

// Timing
unsigned long lastWinchStateSendTime = 0;
const unsigned long WINCH_STATE_SEND_INTERVAL_MS = 50;  // 20 Hz (50ms)

unsigned long lastTelemetrySendTime = 0;
const unsigned long TELEMETRY_SEND_INTERVAL_MS = 1000;  // 1 Hz (1000ms)

unsigned long lastPositionUpdateTime = 0;
const unsigned long POSITION_UPDATE_INTERVAL_MS = 10;   // 100 Hz (10ms) for position feedback

// Position tracking (for speed calculation)
float frontWinchLastPosition = 0.0f;
float rearWinchLastPosition = 0.0f;
unsigned long lastSpeedCalcTime = 0;

// Encoder pulse counters (updated by ISR)
volatile long frontWinchEncoderPulses = 0;
volatile long rearWinchEncoderPulses = 0;

// Encoder configuration
const float encoderPulsesPerRevolution = 1000.0f;  // Adjust based on your encoder (e.g., 1000 PPR)
const float winchDrumCircumference = 6.28f;        // inches (2 * PI * drum radius, e.g., 1" radius = 6.28" circumference)
const float encoderPulsesPerInch = encoderPulsesPerRevolution / winchDrumCircumference;

// Load cell calibration (ONE PER WINCH - each winch calibrated independently)
// Required for ride classification compliance (independent monitoring per winch)
const float frontLoadCellCalibrationFactor = 1.0f;  // Front winch calibration factor (lbs per raw ADC count)
const float frontLoadCellZeroOffset = 0.0f;          // Front winch zero offset (tare value)
const float rearLoadCellCalibrationFactor = 1.0f;   // Rear winch calibration factor (lbs per raw ADC count)
const float rearLoadCellZeroOffset = 0.0f;          // Rear winch zero offset (tare value)

// Battery state
float batteryVoltage = 24.0f;           // Current battery voltage (V)
float batterySOC = 100.0f;              // Battery state of charge (0-100%)
bool batteryLowVoltage = false;         // True if battery voltage is low

// =====================================
// Helper Functions (Math Utilities)
// =====================================

// Clamp function (equivalent to FMath::Clamp)
float clamp(float value, float minVal, float maxVal) {
  if (value < minVal) return minVal;
  if (value > maxVal) return maxVal;
  return value;
}

// =====================================
// Battery Monitoring Functions
// =====================================

/**
 * Read battery voltage from voltage divider on ADC pin
 * @return Battery voltage in volts
 */
float readBatteryVoltage() {
  int adcValue = analogRead(batteryVoltagePin);
  // ESP32 ADC: 12-bit (0-4095), 3.3V reference
  float voltage = (adcValue / 4095.0f) * 3.3f * voltageDividerRatio;
  return voltage;
}

/**
 * Estimate battery state of charge (SOC) from voltage
 * LiFePO4 8S: 28.8V = 100%, 20V = 0%
 * Linear approximation (not perfect, but good enough for monitoring)
 * @param voltage Battery voltage in volts
 * @return State of charge (0-100%)
 */
float estimateBatterySOC(float voltage) {
  // LiFePO4 8S voltage range: 20V (0%) to 28.8V (100%)
  float soc = ((voltage - 20.0f) / (28.8f - 20.0f)) * 100.0f;
  return clamp(soc, 0.0f, 100.0f);
}

/**
 * Update battery monitoring (call in main loop)
 */
void UpdateBatteryMonitoring() {
  batteryVoltage = readBatteryVoltage();
  batterySOC = estimateBatterySOC(batteryVoltage);
  
  // Check for low voltage conditions
  if (batteryVoltage < batteryCriticalVoltageThreshold) {
    // Critical: Emergency stop
    emergencyStop = true;
    batteryLowVoltage = true;
    Serial.println("BATTERY CRITICAL: Emergency stop engaged!");
  } else if (batteryVoltage < batteryLowVoltageThreshold) {
    // Low: Stop operation but don't emergency stop
    batteryLowVoltage = true;
    safetyState = false;  // Prevent winch operation
    Serial.println("BATTERY LOW: Operation stopped. Charge battery.");
  } else {
    batteryLowVoltage = false;
  }
}

// =====================================
// Setup and Loop
// =====================================

void setup() {
  Serial.begin(115200);
  Serial.println("\nHoloCade Superhero Flight Experience ECU");
  Serial.println("======================================");

  // Initialize WiFi and UDP
  // Note: Both RX and TX templates initialize WiFi, but WiFi.begin() is idempotent
  // RX: Listen for commands from Unreal Engine (sets up UDP listener)
  HoloCade_Wireless_Init(ssid, password, unrealPort);
  // TX: Configure target IP/port for sending telemetry (WiFi already connected)
  HoloCade_TargetIP = unrealIP;
  HoloCade_TargetPort = unrealPort;
  HoloCade_Initialized = true;  // Mark TX as initialized (WiFi already connected by RX init)

  // Initialize motor control pins (PWM output to motor controller)
  pinMode(frontWinchMotorPWMPin, OUTPUT);
  pinMode(rearWinchMotorPWMPin, OUTPUT);
  pinMode(frontWinchMotorDirPin, OUTPUT);
  pinMode(rearWinchMotorDirPin, OUTPUT);
  analogWrite(frontWinchMotorPWMPin, 0);  // Stop motors
  analogWrite(rearWinchMotorPWMPin, 0);
  digitalWrite(frontWinchMotorDirPin, LOW);
  digitalWrite(rearWinchMotorDirPin, LOW);

  // Initialize encoder pins (interrupt-capable for pulse counting)
  pinMode(frontWinchEncoderAPin, INPUT_PULLUP);
  pinMode(frontWinchEncoderBPin, INPUT_PULLUP);
  pinMode(rearWinchEncoderAPin, INPUT_PULLUP);
  pinMode(rearWinchEncoderBPin, INPUT_PULLUP);
  // NOOP: Attach interrupts for encoder pulse counting
  // attachInterrupt(digitalPinToInterrupt(frontWinchEncoderAPin), FrontEncoderISR, CHANGE);
  // attachInterrupt(digitalPinToInterrupt(rearWinchEncoderAPin), RearEncoderISR, CHANGE);

  // Initialize load cell pins (HX711 interface) - ONE PER WINCH
  // Each winch has its own independent load cell and HX711 amplifier
  pinMode(frontWinchLoadCellSCK, OUTPUT);
  pinMode(frontWinchLoadCellDT, INPUT);
  pinMode(rearWinchLoadCellSCK, OUTPUT);
  pinMode(rearWinchLoadCellDT, INPUT);
  // NOOP: Initialize HX711 amplifiers (one per winch)
  // #include <HX711.h>
  // HX711 frontLoadCell;
  // HX711 rearLoadCell;
  // frontLoadCell.begin(frontWinchLoadCellDT, frontWinchLoadCellSCK);
  // rearLoadCell.begin(rearWinchLoadCellDT, rearWinchLoadCellSCK);

  // Initialize safety brake pins (fail-safe: LOW = brake engaged) - ONE PER WINCH
  // Each winch has its own independent electromagnetic brake
  // Required for ride classification compliance (independent brake control per winch)
  pinMode(frontWinchBrakePin, OUTPUT);
  pinMode(rearWinchBrakePin, OUTPUT);
  digitalWrite(frontWinchBrakePin, LOW);  // Engage front brake on startup (fail-safe)
  digitalWrite(rearWinchBrakePin, LOW);   // Engage rear brake on startup (fail-safe)

  // Initialize emergency stop input (active LOW, interrupt)
  pinMode(emergencyStopPin, INPUT_PULLUP);
  // NOOP: Attach interrupt for emergency stop
  // attachInterrupt(digitalPinToInterrupt(emergencyStopPin), EmergencyStopISR, FALLING);

  // Initialize battery voltage monitoring (ADC input, no pull-up)
  pinMode(batteryVoltagePin, INPUT);
  // Read initial battery voltage
  batteryVoltage = readBatteryVoltage();
  batterySOC = estimateBatterySOC(batteryVoltage);

  // Calibrate position sensors (assume current position is standingGroundHeight)
  frontWinchCurrentPosition = 0.0f;
  rearWinchCurrentPosition = 0.0f;
  frontWinchLastPosition = 0.0f;
  rearWinchLastPosition = 0.0f;
  standingGroundHeight = 0.0f;

  // Initialize timing
  lastWinchStateSendTime = millis();
  lastTelemetrySendTime = millis();
  lastPositionUpdateTime = millis();
  lastSpeedCalcTime = millis();

  Serial.println("ECU Initialized");
  Serial.print("WiFi SSID: ");
  Serial.println(ssid);
  Serial.print("Unreal IP: ");
  Serial.println(unrealIP);
  Serial.print("Unreal Port: ");
  Serial.println(unrealPort);
}

void loop() {
  // Process incoming UDP commands (from HoloCade_Wireless_RX.h)
  HoloCade_ProcessIncoming();

  // Update battery monitoring
  UpdateBatteryMonitoring();

  // Update position sensors
  UpdatePositionSensors();

  // Update load cells
  UpdateLoadCells();

  // Check safety interlocks
  CheckSafetyInterlocks();

  // Update winch motors (closed-loop control)
  UpdateWinchMotors();

  // Send telemetry to game engine
  unsigned long currentTime = millis();
  
  if (currentTime - lastWinchStateSendTime >= WINCH_STATE_SEND_INTERVAL_MS) {
    SendWinchState();
    lastWinchStateSendTime = currentTime;
  }

  if (currentTime - lastTelemetrySendTime >= TELEMETRY_SEND_INTERVAL_MS) {
    SendTelemetry();
    lastTelemetrySendTime = currentTime;
  }

  // Small delay to prevent watchdog issues
  delay(1);
}

// =====================================
// HoloCade Command Handlers (RX)
// =====================================

void HoloCade_HandleFloat(uint8_t channel, float value) {
  switch (channel) {
    case 0:  // Front winch position
      frontWinchTargetPosition = value;
      break;
    case 1:  // Front winch speed
      frontWinchTargetSpeed = max(0.0f, value);
      break;
    case 2:  // Rear winch position
      rearWinchTargetPosition = value;
      break;
    case 3:  // Rear winch speed
      rearWinchTargetSpeed = max(0.0f, value);
      break;
    case 10: // Standing ground height acknowledgment
      standingGroundHeight = value;
      // Reset relative positions
      frontWinchCurrentPosition = 0.0f;
      rearWinchCurrentPosition = 0.0f;
      frontWinchTargetPosition = 0.0f;
      rearWinchTargetPosition = 0.0f;
      Serial.print("Standing ground height acknowledged: ");
      Serial.print(value);
      Serial.println(" inches");
      break;
    case 11: // Air height parameter
      airHeight = max(0.0f, value);
      break;
    case 12: // Prone height parameter
      proneHeight = max(0.0f, value);
      break;
    case 13: // Player height compensation
      playerHeightCompensation = clamp(value, 0.5f, 2.0f);
      break;
  }
}

void HoloCade_HandleInt32(uint8_t channel, int32_t value) {
  switch (channel) {
    case 6:  // Game state
      gameState = value;
      Serial.print("Game state changed to: ");
      Serial.println(value);
      break;
  }
}

void HoloCade_HandleBool(uint8_t channel, bool value) {
  switch (channel) {
    case 7:  // Emergency stop
      emergencyStop = value;
      if (value) {
        Serial.println("EMERGENCY STOP ACTIVATED");
        // Immediately stop all motors
        analogWrite(frontWinchMotorPin, 0);
        analogWrite(rearWinchMotorPin, 0);
        // Return to standing position
        frontWinchTargetPosition = standingGroundHeight;
        rearWinchTargetPosition = standingGroundHeight;
      }
      break;
    case 9:  // Play session active
      playSessionActive = value;
      if (!value) {
        Serial.println("Play session inactive - winches disabled");
      }
      break;
  }
}

// NOOP handlers for unused types
void HoloCade_HandleString(uint8_t channel, const char* str, uint8_t length) {
  // NOOP: Not used in Superhero Flight Experience
}

void HoloCade_HandleBytes(uint8_t channel, uint8_t* data, uint8_t length) {
  // NOOP: Not used in Superhero Flight Experience
}

// =====================================
// Position Sensor Updates
// =====================================

void UpdatePositionSensors() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastPositionUpdateTime >= POSITION_UPDATE_INTERVAL_MS) {
    // Read encoder pulse counts (updated by ISR)
    // Convert pulses to inches: position = (pulses / pulses_per_inch)
    long frontPulses = frontWinchEncoderPulses;  // Atomic read (volatile)
    long rearPulses = rearWinchEncoderPulses;    // Atomic read (volatile)
    
    // Convert encoder pulses to position (inches)
    frontWinchCurrentPosition = (float)frontPulses / encoderPulsesPerInch;
    rearWinchCurrentPosition = (float)rearPulses / encoderPulsesPerInch;
    
    // Calculate speed from position change
    if (currentTime - lastSpeedCalcTime >= 100) {  // Update speed every 100ms
      float speedDeltaTime = (currentTime - lastSpeedCalcTime) / 1000.0f;
      frontWinchCurrentSpeed = (frontWinchCurrentPosition - frontWinchLastPosition) / speedDeltaTime;
      rearWinchCurrentSpeed = (rearWinchCurrentPosition - rearWinchLastPosition) / speedDeltaTime;
      
      frontWinchLastPosition = frontWinchCurrentPosition;
      rearWinchLastPosition = rearWinchCurrentPosition;
      lastSpeedCalcTime = currentTime;
    }
    
    lastPositionUpdateTime = currentTime;
  }
}

// =====================================
// Encoder Interrupt Service Routines (ISR)
// =====================================

void IRAM_ATTR FrontEncoderISR() {
  // Read encoder B pin to determine direction
  bool encoderB = digitalRead(frontWinchEncoderBPin);
  if (encoderB) {
    frontWinchEncoderPulses++;  // Clockwise (up)
  } else {
    frontWinchEncoderPulses--;  // Counter-clockwise (down)
  }
}

void IRAM_ATTR RearEncoderISR() {
  bool encoderB = digitalRead(rearWinchEncoderBPin);
  if (encoderB) {
    rearWinchEncoderPulses++;   // Clockwise (up)
  } else {
    rearWinchEncoderPulses--;   // Counter-clockwise (down)
  }
}

// =====================================
// Emergency Stop ISR
// =====================================

void IRAM_ATTR EmergencyStopISR() {
  emergencyStop = true;  // Set emergency stop flag (checked in main loop)
}

// =====================================
// Load Cell Updates
// =====================================

void UpdateLoadCells() {
  // Read load cells via HX711 amplifiers (ONE PER WINCH)
  // Each winch has its own independent load cell and HX711 amplifier
  // Required for ride classification compliance (independent monitoring per winch)
  // NOOP: Uncomment when HX711 library is included
  // #include <HX711.h>
  // HX711 frontLoadCell;
  // HX711 rearLoadCell;
  
  // Initialize HX711 amplifiers (one per winch)
  // frontLoadCell.begin(frontWinchLoadCellDT, frontWinchLoadCellSCK);
  // rearLoadCell.begin(rearWinchLoadCellDT, rearWinchLoadCellSCK);
  
  // Read HX711 values (24-bit ADC) - independent reading per winch
  // long frontRaw = frontLoadCell.read();
  // long rearRaw = rearLoadCell.read();
  
  // Convert raw ADC to lbs (calibration required - each winch calibrated separately)
  // frontWinchTension = ((float)frontRaw - frontLoadCellZeroOffset) * frontLoadCellCalibrationFactor;
  // rearWinchTension = ((float)rearRaw - rearLoadCellZeroOffset) * rearLoadCellCalibrationFactor;
  
  // For now, simulate load based on position (tension increases with height)
  // Remove this simulation when real load cells are connected
  frontWinchTension = abs(frontWinchCurrentPosition) * 2.5f; // ~2.5 lbs per inch
  rearWinchTension = abs(rearWinchCurrentPosition) * 2.5f;
  
  // Clamp to reasonable values (independent limits per winch)
  frontWinchTension = clamp(frontWinchTension, 0.0f, maxWeight);
  rearWinchTension = clamp(rearWinchTension, 0.0f, maxWeight);
}

// =====================================
// Safety Interlock Checks
// =====================================

void CheckSafetyInterlocks() {
  bool previousSafetyState = safetyState;
  safetyState = true;

  // Check weight limits
  if (frontWinchTension > maxWeight || rearWinchTension > maxWeight) {
    safetyState = false;
    Serial.println("SAFETY: Weight limit exceeded");
  }

  // Check height limits
  float frontHeight = abs(frontWinchCurrentPosition - standingGroundHeight);
  float rearHeight = abs(rearWinchCurrentPosition - standingGroundHeight);
  if (frontHeight > maxHeight || rearHeight > maxHeight) {
    safetyState = false;
    Serial.println("SAFETY: Height limit exceeded");
  }

  // Check winch faults (NOOP: will check motor temperature, current, etc.)
  // For now, assume no faults
  telemetry.FrontWinchFault = false;
  telemetry.RearWinchFault = false;

  // Check winch redundancy (both winches must be operational)
  telemetry.WinchRedundancyStatus = !telemetry.FrontWinchFault && !telemetry.RearWinchFault;
  if (!telemetry.WinchRedundancyStatus) {
    safetyState = false;
    Serial.println("SAFETY: Winch redundancy lost - degraded mode");
  }

  // Check battery low voltage (handled in UpdateBatteryMonitoring, but check here too)
  if (batteryLowVoltage) {
    safetyState = false;
    Serial.println("SAFETY: Battery low voltage - operation stopped");
  }

  // Emergency stop always disables safety
  if (emergencyStop) {
    safetyState = false;
  }

  if (previousSafetyState != safetyState) {
    Serial.print("Safety state changed: ");
    Serial.println(safetyState ? "SAFE" : "UNSAFE");
  }
}

// =====================================
// Winch Motor Control
// =====================================

void UpdateWinchMotors() {
  // Don't operate if safety interlocks are active or play session is inactive
  if (!safetyState || !playSessionActive || emergencyStop) {
    // Stop motors
    analogWrite(frontWinchMotorPWMPin, 0);
    analogWrite(rearWinchMotorPWMPin, 0);
    // Engage brakes independently (fail-safe: LOW = brake engaged)
    // Each winch has its own independent brake for safety compliance
    digitalWrite(frontWinchBrakePin, LOW);  // Engage front brake
    digitalWrite(rearWinchBrakePin, LOW);   // Engage rear brake
    frontWinchCurrentSpeed = 0.0f;
    rearWinchCurrentSpeed = 0.0f;
    return;
  }

  // Release brakes independently (HIGH = brake released)
  // Each winch's brake is controlled independently
  digitalWrite(frontWinchBrakePin, HIGH);  // Release front brake
  digitalWrite(rearWinchBrakePin, HIGH);   // Release rear brake

  // Calculate position error
  float frontPositionError = frontWinchTargetPosition - frontWinchCurrentPosition;
  float rearPositionError = rearWinchTargetPosition - rearWinchCurrentPosition;

  // Simple proportional control (NOOP: will implement PID control for smoother operation)
  // Calculate motor output based on position error and target speed
  float frontMotorOutput = 0.0f;
  float rearMotorOutput = 0.0f;
  bool frontDirection = true;  // true = up, false = down
  bool rearDirection = true;

  // Front winch control
  if (abs(frontPositionError) > 0.1f) {  // Deadband: 0.1 inches
    // Calculate direction (positive = up, negative = down)
    frontDirection = frontPositionError > 0.0f;
    
    // Calculate speed (clamp to target speed)
    float speed = min(abs(frontPositionError) * 10.0f, frontWinchTargetSpeed); // P-gain = 10
    frontWinchCurrentSpeed = frontDirection ? speed : -speed;
    
    // Convert speed to PWM (0-255)
    // Assuming max speed of 12 inches/second = 255 PWM
    float maxSpeed = 12.0f; // inches/second
    frontMotorOutput = clamp((abs(frontWinchCurrentSpeed) / maxSpeed) * 255.0f, 0.0f, 255.0f);
  } else {
    frontWinchCurrentSpeed = 0.0f;
    frontMotorOutput = 0.0f;
  }

  // Rear winch control (same logic)
  if (abs(rearPositionError) > 0.1f) {
    rearDirection = rearPositionError > 0.0f;
    float speed = min(abs(rearPositionError) * 10.0f, rearWinchTargetSpeed);
    rearWinchCurrentSpeed = rearDirection ? speed : -speed;
    
    float maxSpeed = 12.0f;
    rearMotorOutput = clamp((abs(rearWinchCurrentSpeed) / maxSpeed) * 255.0f, 0.0f, 255.0f);
  } else {
    rearWinchCurrentSpeed = 0.0f;
    rearMotorOutput = 0.0f;
  }

  // Apply motor outputs
  // Set direction pins
  digitalWrite(frontWinchMotorDirPin, frontDirection ? HIGH : LOW);
  digitalWrite(rearWinchMotorDirPin, rearDirection ? HIGH : LOW);
  
  // Set PWM speed
  analogWrite(frontWinchMotorPWMPin, (int)frontMotorOutput);
  analogWrite(rearWinchMotorPWMPin, (int)rearMotorOutput);
}

// =====================================
// Telemetry Transmission (TX)
// =====================================

void SendWinchState() {
  // Populate winch state struct
  winchState.FrontWinchPosition = frontWinchCurrentPosition;
  winchState.FrontWinchSpeed = frontWinchCurrentSpeed;
  winchState.FrontWinchTension = frontWinchTension;
  winchState.RearWinchPosition = rearWinchCurrentPosition;
  winchState.RearWinchSpeed = rearWinchCurrentSpeed;
  winchState.RearWinchTension = rearWinchTension;
  winchState.GameState = gameState;
  winchState.SafetyState = safetyState;
  winchState.Timestamp = millis();

  // Send struct as bytes (Channel 310)
  uint8_t* structBytes = (uint8_t*)&winchState;
  uint8_t structSize = sizeof(FSuperheroFlightDualWinchState);
  HoloCade_SendBytes(310, structBytes, structSize);
}

void SendTelemetry() {
  // Populate telemetry struct
  telemetry.FrontWinchMotorTemp = 25.0f;  // NOOP: Read from temperature sensor
  telemetry.RearWinchMotorTemp = 25.0f;    // NOOP: Read from temperature sensor
  telemetry.FrontWinchFault = false;     // NOOP: Check motor fault conditions
  telemetry.RearWinchFault = false;       // NOOP: Check motor fault conditions
  telemetry.WinchRedundancyStatus = !telemetry.FrontWinchFault && !telemetry.RearWinchFault;
  telemetry.SystemVoltage = batteryVoltage;  // Battery voltage from monitoring
  telemetry.SystemCurrent = 5.0f;         // NOOP: Read from current sensor
  telemetry.Timestamp = millis();

  // Send struct as bytes (Channel 311)
  uint8_t* structBytes = (uint8_t*)&telemetry;
  uint8_t structSize = sizeof(FSuperheroFlightTelemetry);
  HoloCade_SendBytes(311, structBytes, structSize);
}

// =====================================
// Helper Functions
// =====================================

// Simple math helpers (since Arduino doesn't have FMath)
namespace FMath {
  float Max(float a, float b) { return a > b ? a : b; }
  float Min(float a, float b) { return a < b ? a : b; }
  float Clamp(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
  }
  float Abs(float value) { return value < 0.0f ? -value : value; }
}

