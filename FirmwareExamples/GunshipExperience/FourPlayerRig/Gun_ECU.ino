/*
 * HoloCade Gunship Experience - Child ECU (Gun_ECU)
 * 
 * Embedded control unit for a single gun station in GunshipExperience.
 * One child ECU per player station (4 total) connects to parent ECU (GunshipExperience_ECU) in star topology.
 * 
 * This child ECU handles:
 * - Dual thumb button input (debounced, rate-limited)
 * - N× solenoid kicker control (with optional redundancy and thermal management)
 * - SteamVR Ultimate tracker pose reading (or relay from parent ECU)
 * - Telemetry reporting to parent ECU (10–30 Hz)
 * 
 * Functionality:
 * - Receives fire commands from parent ECU or game engine
 * - Controls solenoid kickers with pulse envelope (50–200 ms)
 * - Monitors solenoid temperatures (NTC thermistors)
 * - Implements redundancy: alternates between N solenoids based on temperature
 * - Implements throttling: reduces PWM duty if thermal limits exceeded
 * - Reports button states, tracker pose, and telemetry to parent ECU
 * 
 * Supported Platforms:
 * - ESP32 (built-in WiFi/Ethernet) - Recommended for this application
 *   * Ethernet mode: Requires ESP32 with Ethernet PHY (LAN8720, TLK110, etc.)
 *   * WiFi mode: Works with any ESP32
 * - ESP8266 (built-in WiFi) - Limited GPIO, may need additional hardware
 * - STM32 (Ethernet/WiFi) - Robust RT control, recommended for production
 * - Arduino + WiFi Shield (ESP8266-based) - Standard Arduino GPIO pins
 * - Raspberry Pi (built-in WiFi/Ethernet) - GPIO via WiringPi or pigpio
 * - Jetson Nano (built-in WiFi/Ethernet) - GPIO via Jetson GPIO library
 * 
 * **Recommended Hardware: HoloCade Child Shield (LBCS)**
 * This firmware is configured for the HoloCade Child Shield (LBCS) by default:
 * - Ethernet PHY: LAN8720A with MDC=GPIO17, MDIO=GPIO18
 * - Solenoid PWM pins: GPIO5, 27, 19, 21, 22, 23, 25, 26 (GPIO18 replaced with GPIO27)
 * - Temperature ADC pins: GPIO32 (driver), GPIO33, 34, 1, 6, 7, 8, 9, 14 (GPIO35-39 replaced)
 * - Button pins: GPIO2, GPIO4 (available on Child Shield breakout)
 * For other ESP32-Ethernet boards, adjust pin definitions in the configuration section below.
 * 
 * Communication Modes:
 * - Wired Ethernet (recommended): Lower latency, more reliable for child ECU → parent ECU
 *   * Set COMMUNICATION_MODE = COMM_MODE_ETHERNET
 *   * Configure Ethernet PHY pins in configuration section
 * - Wireless WiFi: More flexible deployment, higher latency
 *   * Set COMMUNICATION_MODE = COMM_MODE_WIFI
 *   * Configure WiFi SSID/password in configuration section
 * 
 * Hardware Requirements:
 * - Microcontroller with WiFi/Ethernet capability (see supported platforms above)
 * - N× solenoids (24V pull-type, 20–40N @ 5–8mm) - see Gunship_Hardware_Specs.md
 * - N× Pololu G2 drivers (or IBT-2 modules) with heatsinks and fans
 * - N× NTC thermistors (10 kΩ @ 25°C) for solenoid temperature monitoring
 * - 1× NTC thermistor for PWM driver temperature monitoring (optional)
 * - Dual thumb buttons (GPIO inputs with pull-up)
 * - SteamVR Ultimate tracker (or receive pose from parent ECU)
 * - 24V PSU (3–5A per station)
 * 
 * Protocol: Binary HoloCade protocol
 * Channel Mapping (Child ECU → Parent ECU):
 * - Ch10+n (n=0..3): Fire command bools (engine→child ECU), or child ECU→parent ECU status
 * - Ch20+n: Intensity (0.0–1.0) or envelope index
 * - Ch30+n: Telemetry (active solenoid temp or duty proxy) as float
 * - Ch40+n (if redundant): Active solenoid ID (0 to N-1) as int32
 * - Ch50+n (if redundant): Total solenoid count N as int32
 * - Ch60+n through Ch(60+N-1)+n (if N>16): Individual solenoid temperatures as float
 * - Struct 150+n: Gun pose struct (orientation/position) per station
 * - Ch7: Global emergency stop (already standardized)
 * 
 * This example uses the HoloCade_Wireless_RX.h and HoloCade_Wireless_TX.h templates.
 * Copy these headers from FirmwareExamples/Base/Templates/ to your sketch directory.
 * 
 * For complete hardware specifications, see Gunship_Hardware_Specs.md
 * 
 * Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.
 */

#include "HoloCade_Wireless_RX.h"
#include "../Base/Templates/HoloCade_Wireless_TX.h"

// =====================================
// Configuration
// =====================================

// Communication mode: WIFI or ETHERNET
// Wired Ethernet recommended for lower latency between child ECUs and parent ECU
// (all on same chassis). Wireless can be used if wiring is impractical.
#define COMM_MODE_WIFI 0
#define COMM_MODE_ETHERNET 1
const uint8_t COMMUNICATION_MODE = COMM_MODE_ETHERNET;  // Change to COMM_MODE_WIFI for wireless

// WiFi credentials (only used if COMMUNICATION_MODE == COMM_MODE_WIFI)
const char* ssid = "VR_Arcade_LAN";
const char* password = "your_password_here";

// Ethernet configuration (only used if COMMUNICATION_MODE == COMM_MODE_ETHERNET)
// ESP32 requires an external Ethernet PHY module (ESP32 has MAC but no PHY)
// 
// Recommended PHY Module: LAN8720A (most common, ~$1–$3, widely available)
// Alternatives: TLK110, DP83848, RTL8201
// 
// Pre-built ESP32 Ethernet boards:
// - ESP32-Ethernet-Kit (Olimex) - includes LAN8720
// - ESP32-POE (Olimex) - includes LAN8720 + Power over Ethernet
// - Custom PCB with ESP32 + LAN8720 breakout
//
// Cabling: Standard CAT5e or CAT6 Ethernet cable (RJ45 connectors)
//
// Pin configuration (adjust for your board/PHY module):
#if defined(ESP32) && COMMUNICATION_MODE == COMM_MODE_ETHERNET
  #include <ETH.h>
  // LAN8720 PHY configuration for HoloCade Child Shield
  // Child Shield uses: MDC=GPIO17, MDIO=GPIO18, no PHY power pin
  #define ETH_PHY_ADDR 0        // PHY address (usually 0 or 1)
  #define ETH_PHY_POWER -1       // No PHY power pin on Child Shield (was GPIO16 on generic boards)
  #define ETH_PHY_MDC 17         // MDC pin (Management Data Clock) - Child Shield uses GPIO17 (was GPIO23)
  #define ETH_PHY_MDIO 18        // MDIO pin (Management Data I/O) - Child Shield uses GPIO18
  #define ETH_PHY_TYPE ETH_PHY_LAN8720  // PHY chip type
  #define ETH_CLK_MODE ETH_CLOCK_GPIO17_OUT  // Clock mode (GPIO17 for LAN8720)
  
  // For other PHY modules, adjust ETH_PHY_TYPE:
  // ETH_PHY_TLK110, ETH_PHY_DP83848, ETH_PHY_RTL8201
  //
  // NOTE: For generic ESP32-Ethernet boards (not Child Shield), you may need:
  // #define ETH_PHY_MDC 23
  // #define ETH_PHY_POWER 16
#endif

  // Parent ECU (GunshipExperience_ECU) IP address (for reporting telemetry)
IPAddress parentECU_IP(192, 168, 1, 100);
uint16_t parentECU_Port = 8892;  // Port on parent ECU that receives child ECU telemetry

// Station ID (0-3, set per ECU instance)
const uint8_t STATION_ID = 0;  // Change this: 0, 1, 2, or 3 for each gun station

// =====================================
// Hardware Pin Configuration
// =====================================
//
// **HoloCade Child Shield (LBCS) Pin Mapping:**
// This firmware is configured for the HoloCade Child Shield by default.
// All pins are available on the Child Shield's breakout headers except:
// - Ethernet pins: GPIO17 (MDC), GPIO18 (MDIO), GPIO20 (TXEN), GPIO35-39 (RMII data)
// - UART pins: GPIO43 (U0TXD), GPIO44 (U0RXD)
// - Power/EN pins: GPIO3 (EN), VIN, GND
//
// For other ESP32-Ethernet boards, adjust pin definitions below.

// Dual thumb buttons
const uint8_t BUTTON_0_PIN = 2;  // Left thumb button (GPIO with INPUT_PULLUP) - Available on Child Shield breakout
const uint8_t BUTTON_1_PIN = 4;  // Right thumb button (GPIO with INPUT_PULLUP) - Available on Child Shield breakout

// Solenoid configuration (N solenoids, configurable)
const uint8_t MAX_SOLENOIDS = 8;  // Maximum supported solenoids (limited by GPIO)
uint8_t numSolenoids = 2;  // Default: 2 solenoids (dual redundancy)

// Solenoid driver pins (one GPIO enable per solenoid if using separate drivers)
// For shared driver with relay/multiplexer, use different pin mapping
// NOTE: GPIO18 is used for Ethernet MDIO on Child Shield, so it's replaced with GPIO27
// Child Shield breakout available pins: GPIO5, 19, 21, 22, 23, 25, 26, 27, 40, 41, 42, etc.
uint8_t solenoidEnablePins[MAX_SOLENOIDS] = {5, 27, 19, 21, 22, 23, 25, 26};  // ESP32 GPIO pins (GPIO18→GPIO27 for Child Shield)
uint8_t solenoidPWMPins[MAX_SOLENOIDS] = {5, 27, 19, 21, 22, 23, 25, 26};     // PWM-capable pins (GPIO18→GPIO27 for Child Shield)

// NTC thermistor pins (one per solenoid + one for driver module)
// NOTE: GPIO35-39 are used for Ethernet on Child Shield (RXD0, RXD1, TXD1, CRS_DV, TXD0)
// Child Shield breakout available ADC pins: GPIO1, 4, 5, 6, 7, 8, 9, 10, 14, 32, 33, 34
const uint8_t DRIVER_TEMP_PIN = 32;  // ADC pin for driver module temperature
uint8_t solenoidTempPins[MAX_SOLENOIDS] = {33, 34, 1, 6, 7, 8, 9, 14};  // ADC pins (GPIO35-39→GPIO1,6,7,8,9 for Child Shield)

// PWM configuration
const uint16_t PWM_FREQUENCY = 1000;  // 1 kHz PWM frequency
const uint8_t PWM_RESOLUTION = 8;      // 8-bit resolution (0-255)

// =====================================
// State Variables
// =====================================

// Button state
bool button0State = false;
bool button1State = false;
bool button0LastState = false;
bool button1LastState = false;
unsigned long button0LastPressTime = 0;
unsigned long button1LastPressTime = 0;
const unsigned long BUTTON_DEBOUNCE_MS = 50;  // 50ms debounce
const unsigned long BUTTON_RATE_LIMIT_MS = 100;  // 100ms minimum between presses

// Solenoid state
uint8_t activeSolenoidID = 0;  // Currently active solenoid (0 to N-1)
float solenoidTemperatures[MAX_SOLENOIDS];  // Temperature array for all solenoids
float driverModuleTemperature = 25.0f;  // PWM driver module temperature
bool solenoidEnabled[MAX_SOLENOIDS];  // Enable state per solenoid
uint8_t solenoidPWMValue[MAX_SOLENOIDS];  // Current PWM duty (0-255)

// Thermal management
const float TEMP_THRESHOLD_LOW = 70.0f;   // Start throttling at 70°C
const float TEMP_THRESHOLD_HIGH = 80.0f;  // Switch solenoid at 80°C
const float TEMP_THRESHOLD_SHUTDOWN = 85.0f;  // Thermal shutdown at 85°C
float currentPWMThrottle = 1.0f;  // PWM duty multiplier (1.0 = 100%, 0.5 = 50%)
bool thermalShutdown = false;

// Fire command state
bool fireCommandActive = false;
unsigned long fireStartTime = 0;
unsigned long fireDuration = 100;  // Default 100ms pulse
float fireIntensity = 1.0f;  // 0.0 to 1.0

// Tracker pose (placeholder - implementation depends on tracker integration)
struct FGunPose {
  float PositionX, PositionY, PositionZ;  // Position in cm
  float RotationX, RotationY, RotationZ, RotationW;  // Quaternion rotation
};
FGunPose gunPose = {0, 0, 0, 0, 0, 0, 1};  // Initialize to origin

// Telemetry reporting
unsigned long lastTelemetryTime = 0;
const unsigned long TELEMETRY_INTERVAL_MS = 50;  // 20 Hz telemetry rate

// Session state
bool sessionActive = false;
unsigned long sessionStartTime = 0;

// Game state (received from parent ECU)
bool playSessionActive = false;  // Play session authorization (guns can only fire when true)

// Ethernet state (ESP32 only)
#if defined(ESP32) && COMMUNICATION_MODE == COMM_MODE_ETHERNET
bool ethernetConnected = false;
#endif

// =====================================
// Ethernet Initialization (ESP32 only)
// =====================================

#if defined(ESP32) && COMMUNICATION_MODE == COMM_MODE_ETHERNET
void InitializeEthernet() {
  // Ethernet event handlers
  WiFi.onEvent(EthernetEvent);
  
  // Initialize Ethernet with PHY configuration
  ETH.begin(ETH_PHY_ADDR, ETH_PHY_POWER, ETH_PHY_MDC, ETH_PHY_MDIO, ETH_PHY_TYPE, ETH_CLK_MODE);
  
  Serial.println("Waiting for Ethernet connection...");
  unsigned long startTime = millis();
  while (!ethernetConnected && (millis() - startTime < 10000)) {
    delay(100);
  }
  
  if (ethernetConnected) {
    Serial.printf("Ethernet connected! IP: %s\n", ETH.localIP().toString().c_str());
  } else {
    Serial.println("ERROR: Ethernet connection failed!");
  }
}

void EthernetEvent(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      Serial.println("Ethernet Started");
      ETH.setHostname("GunECU");
      break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      Serial.println("Ethernet Connected");
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      Serial.printf("Ethernet Got IP: %s\n", ETH.localIP().toString().c_str());
      ethernetConnected = true;
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      Serial.println("Ethernet Disconnected");
      ethernetConnected = false;
      break;
    case ARDUINO_EVENT_ETH_STOP:
      Serial.println("Ethernet Stopped");
      ethernetConnected = false;
      break;
    default:
      break;
  }
}
#endif

// =====================================
// Setup
// =====================================

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\nHoloCade Gunship Experience - Child ECU (Gun_ECU)");
  Serial.println("=====================================\n");
  Serial.printf("Station ID: %d\n", STATION_ID);
  Serial.printf("Number of Solenoids: %d\n", numSolenoids);
  
  // Initialize buttons (INPUT_PULLUP)
  pinMode(BUTTON_0_PIN, INPUT_PULLUP);
  pinMode(BUTTON_1_PIN, INPUT_PULLUP);
  button0LastState = digitalRead(BUTTON_0_PIN);
  button1LastState = digitalRead(BUTTON_1_PIN);
  
  // Initialize solenoid drivers
  for (uint8_t i = 0; i < numSolenoids; i++) {
    pinMode(solenoidEnablePins[i], OUTPUT);
    digitalWrite(solenoidEnablePins[i], LOW);  // Disable by default
    solenoidEnabled[i] = false;
    solenoidPWMValue[i] = 0;
    
    // Initialize PWM (ESP32-specific, adjust for other platforms)
    #if defined(ESP32)
    ledcSetup(i, PWM_FREQUENCY, PWM_RESOLUTION);  // Channel i, frequency, resolution
    ledcAttachPin(solenoidPWMPins[i], i);  // Attach pin to channel
    ledcWrite(i, 0);  // Start with 0% duty
    #endif
  }
  
  // Initialize temperature sensors (ADC pins)
  pinMode(DRIVER_TEMP_PIN, INPUT);
  for (uint8_t i = 0; i < numSolenoids; i++) {
    pinMode(solenoidTempPins[i], INPUT);
    solenoidTemperatures[i] = 25.0f;  // Initialize to room temperature
  }
  
  // Initialize network communication (WiFi or Ethernet)
  if (COMMUNICATION_MODE == COMM_MODE_WIFI) {
    // WiFi mode: Use wireless templates
    Serial.println("Initializing WiFi...");
    HoloCade_Wireless_Init(ssid, password, 8888 + STATION_ID);  // Unique port per station
    
    // Configure TX for sending telemetry to parent ECU
    extern IPAddress HoloCade_TargetIP;
    extern uint16_t HoloCade_TargetPort;
    extern bool HoloCade_Initialized;
    HoloCade_TargetIP = parentECU_IP;
    HoloCade_TargetPort = parentECU_Port;
    HoloCade_Initialized = true;  // Mark TX as initialized (WiFi already connected)
    
    Serial.println("WiFi mode initialized");
  } else {
    // Ethernet mode: Initialize Ethernet and configure UDP
    #if defined(ESP32)
    Serial.println("Initializing Ethernet...");
    InitializeEthernet();
    
    if (!ethernetConnected) {
      Serial.println("WARNING: Ethernet not connected. Some features may not work.");
    }
    
    // Configure TX globals for Ethernet mode (WiFiUDP works with Ethernet too)
    extern IPAddress HoloCade_TargetIP;
    extern uint16_t HoloCade_TargetPort;
    extern bool HoloCade_Initialized;
    extern WiFiUDP HoloCade_UDP;
    
    HoloCade_TargetIP = parentECU_IP;
    HoloCade_TargetPort = parentECU_Port;
    
    // Start UDP listener for RX (WiFiUDP works with Ethernet on ESP32)
    HoloCade_UDP.begin(8888 + STATION_ID);
    HoloCade_Initialized = true;
    
    Serial.println("Ethernet mode initialized");
    Serial.printf("UDP listening on port %d\n", 8888 + STATION_ID);
    Serial.printf("Target: %s:%d\n", parentECU_IP.toString().c_str(), parentECU_Port);
    #else
    Serial.println("ERROR: Ethernet mode requires ESP32. Falling back to WiFi.");
    HoloCade_Wireless_Init(ssid, password, 8888 + STATION_ID);
    extern IPAddress HoloCade_TargetIP;
    extern uint16_t HoloCade_TargetPort;
    extern bool HoloCade_Initialized;
    HoloCade_TargetIP = parentECU_IP;
    HoloCade_TargetPort = parentECU_Port;
    HoloCade_Initialized = true;
    #endif
  }
  
  Serial.println("\nChild ECU Ready!");
  Serial.println("Waiting for commands from parent ECU...\n");
  Serial.println("Channel Mapping:");
  Serial.println("  Ch 10+n: Fire command (bool)");
  Serial.println("  Ch 20+n: Fire intensity (float, 0.0-1.0)");
  Serial.println("  Ch 30+n: Telemetry (float)");
  Serial.println("  Ch 7: Emergency stop (bool)\n");
}

// =====================================
// Main Loop
// =====================================

void loop() {
  // Process incoming HoloCade commands
  HoloCade_ProcessIncoming();
  
  // Read buttons (debounced, rate-limited)
  ReadButtons();
  
  // Update solenoid control
  UpdateSolenoidControl();
  
  // Read temperatures
  ReadTemperatures();
  
  // Thermal management (redundancy alternation, throttling)
  UpdateThermalManagement();
  
  // Read tracker pose (placeholder - implement based on tracker integration)
  // ReadTrackerPose();  // TODO: Implement tracker reading
  
  // Send telemetry to parent ECU
  unsigned long currentTime = millis();
  if (currentTime - lastTelemetryTime >= TELEMETRY_INTERVAL_MS) {
    SendTelemetry();
    lastTelemetryTime = currentTime;
  }
  
  delay(10);  // Control loop update rate (~100 Hz)
}

// =====================================
// Button Reading (Debounced, Rate-Limited)
// =====================================

void ReadButtons() {
  unsigned long currentTime = millis();
  
  // Button 0
  bool button0Current = !digitalRead(BUTTON_0_PIN);  // Invert because INPUT_PULLUP (LOW = pressed)
  if (button0Current != button0LastState) {
    button0LastPressTime = currentTime;  // Reset debounce timer
  }
  if ((currentTime - button0LastPressTime) > BUTTON_DEBOUNCE_MS) {
    if (button0Current != button0State) {
      button0State = button0Current;
      if (button0State && (currentTime - button0LastPressTime) > BUTTON_RATE_LIMIT_MS) {
        // Button 0 pressed (debounced, rate-limited)
        OnButton0Pressed();
        button0LastPressTime = currentTime;
      }
    }
  }
  button0LastState = button0Current;
  
  // Button 1
  bool button1Current = !digitalRead(BUTTON_1_PIN);
  if (button1Current != button1LastState) {
    button1LastPressTime = currentTime;
  }
  if ((currentTime - button1LastPressTime) > BUTTON_DEBOUNCE_MS) {
    if (button1Current != button1State) {
      button1State = button1Current;
      if (button1State && (currentTime - button1LastPressTime) > BUTTON_RATE_LIMIT_MS) {
        // Button 1 pressed (debounced, rate-limited)
        OnButton1Pressed();
        button1LastPressTime = currentTime;
      }
    }
  }
  button1LastState = button1Current;
}

void OnButton0Pressed() {
  // Button 0 action (e.g., primary fire mode)
  Serial.println("ECU: Button 0 pressed");
  // Report button state to Primary ECU (handled in SendTelemetry)
}

void OnButton1Pressed() {
  // Button 1 action (e.g., secondary fire mode)
  Serial.println("ECU: Button 1 pressed");
  // Report button state to Primary ECU (handled in SendTelemetry)
}

// =====================================
// Solenoid Control
// =====================================

void UpdateSolenoidControl() {
  if (thermalShutdown) {
    // Disable all solenoids if thermal shutdown active
    for (uint8_t i = 0; i < numSolenoids; i++) {
      DisableSolenoid(i);
    }
    return;
  }
  
  // Safety check: Only allow firing if play session is active
  if (!playSessionActive && fireCommandActive) {
    // Play session became inactive - stop firing immediately
    fireCommandActive = false;
    DisableSolenoid(activeSolenoidID);
    return;
  }
  
  // Update active solenoid based on fire command
  if (fireCommandActive && playSessionActive) {
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - fireStartTime;
    
    if (elapsed < fireDuration) {
      // Fire command active - drive active solenoid
      uint8_t pwmValue = (uint8_t)(255.0f * fireIntensity * currentPWMThrottle);
      EnableSolenoid(activeSolenoidID, pwmValue);
    } else {
      // Fire command complete - disable solenoid
      DisableSolenoid(activeSolenoidID);
      fireCommandActive = false;
    }
  } else {
    // No fire command or play session inactive - ensure all solenoids are disabled
    for (uint8_t i = 0; i < numSolenoids; i++) {
      DisableSolenoid(i);
    }
  }
}

void EnableSolenoid(uint8_t solenoidID, uint8_t pwmValue) {
  if (solenoidID >= numSolenoids) return;
  
  solenoidEnabled[solenoidID] = true;
  solenoidPWMValue[solenoidID] = pwmValue;
  
  #if defined(ESP32)
  ledcWrite(solenoidID, pwmValue);
  #else
  // For other platforms, use analogWrite or digital PWM
  analogWrite(solenoidEnablePins[solenoidID], pwmValue);
  #endif
}

void DisableSolenoid(uint8_t solenoidID) {
  if (solenoidID >= numSolenoids) return;
  
  solenoidEnabled[solenoidID] = false;
  solenoidPWMValue[solenoidID] = 0;
  
  #if defined(ESP32)
  ledcWrite(solenoidID, 0);
  #else
  analogWrite(solenoidEnablePins[solenoidID], 0);
  #endif
  
  digitalWrite(solenoidEnablePins[solenoidID], LOW);  // Ensure disabled
}

// =====================================
// Temperature Reading (NTC Thermistors)
// =====================================

void ReadTemperatures() {
  // Read driver module temperature
  driverModuleTemperature = ReadNTC(DRIVER_TEMP_PIN);
  
  // Read all solenoid temperatures
  for (uint8_t i = 0; i < numSolenoids; i++) {
    solenoidTemperatures[i] = ReadNTC(solenoidTempPins[i]);
  }
}

float ReadNTC(uint8_t adcPin) {
  // Read NTC thermistor via voltage divider (NTC + 10kΩ reference resistor)
  // Formula: R_ntc = R_ref * (ADC_max / ADC_reading - 1)
  // Then convert resistance to temperature using Steinhart-Hart equation
  
  #if defined(ESP32)
  int adcValue = analogRead(adcPin);  // 0-4095 for ESP32 (12-bit ADC)
  float voltage = (adcValue / 4095.0f) * 3.3f;  // ESP32 ADC reference is 3.3V
  #elif defined(ESP8266)
  int adcValue = analogRead(adcPin);  // 0-1023 for ESP8266 (10-bit ADC)
  float voltage = (adcValue / 1023.0f) * 3.3f;
  #else
  int adcValue = analogRead(adcPin);  // Platform-specific ADC
  float voltage = (adcValue / 1023.0f) * 3.3f;  // Adjust for your platform
  #endif
  
  // Voltage divider: V_out = V_in * (R_ref / (R_ntc + R_ref))
  // Solving for R_ntc: R_ntc = R_ref * ((V_in / V_out) - 1)
  const float R_REF = 10000.0f;  // 10 kΩ reference resistor
  const float V_IN = 3.3f;  // Supply voltage
  float r_ntc = R_REF * ((V_IN / voltage) - 1.0f);
  
  // Steinhart-Hart equation: 1/T = A + B*ln(R) + C*(ln(R))^3
  // For 10kΩ @ 25°C NTC (B=3950K typical):
  const float A = 0.001129148f;
  const float B = 0.000234125f;
  const float C = 0.0000000876741f;
  
  float logR = log(r_ntc);
  float tempK = 1.0f / (A + B * logR + C * logR * logR * logR);
  float tempC = tempK - 273.15f;
  
  return tempC;
}

// =====================================
// Thermal Management (Redundancy & Throttling)
// =====================================

void UpdateThermalManagement() {
  // Check for thermal shutdown
  if (driverModuleTemperature > TEMP_THRESHOLD_SHUTDOWN) {
    thermalShutdown = true;
    Serial.println("ECU: THERMAL SHUTDOWN - Driver module too hot!");
    return;
  }
  
  // Check if all solenoids exceed shutdown threshold
  bool allSolenoidsHot = true;
  for (uint8_t i = 0; i < numSolenoids; i++) {
    if (solenoidTemperatures[i] < TEMP_THRESHOLD_SHUTDOWN) {
      allSolenoidsHot = false;
      break;
    }
  }
  if (allSolenoidsHot) {
    thermalShutdown = true;
    Serial.println("ECU: THERMAL SHUTDOWN - All solenoids too hot!");
    return;
  }
  
  // Release thermal shutdown if temperatures drop
  if (thermalShutdown && driverModuleTemperature < (TEMP_THRESHOLD_SHUTDOWN - 5.0f)) {
    bool allSolenoidsCool = true;
    for (uint8_t i = 0; i < numSolenoids; i++) {
      if (solenoidTemperatures[i] >= (TEMP_THRESHOLD_SHUTDOWN - 5.0f)) {
        allSolenoidsCool = false;
        break;
      }
    }
    if (allSolenoidsCool) {
      thermalShutdown = false;
      Serial.println("ECU: Thermal shutdown released");
    }
  }
  
  // Select coolest solenoid at session start (if redundancy enabled)
  if (!sessionActive) {
    SelectCoolestSolenoid();
    sessionActive = true;
    sessionStartTime = millis();
  }
  
  // Fallback: Switch to next coolest if active solenoid exceeds threshold
  if (solenoidTemperatures[activeSolenoidID] > TEMP_THRESHOLD_HIGH) {
    uint8_t nextCoolest = FindCoolestSolenoid();
    if (nextCoolest != activeSolenoidID && solenoidTemperatures[nextCoolest] < TEMP_THRESHOLD_HIGH) {
      Serial.printf("ECU: Switching from solenoid %d (%.1f°C) to %d (%.1f°C)\n",
        activeSolenoidID, solenoidTemperatures[activeSolenoidID],
        nextCoolest, solenoidTemperatures[nextCoolest]);
      activeSolenoidID = nextCoolest;
    }
  }
  
  // Throttling logic: Reduce PWM duty if temperatures high
  float maxSolenoidTemp = 0.0f;
  for (uint8_t i = 0; i < numSolenoids; i++) {
    if (solenoidTemperatures[i] > maxSolenoidTemp) {
      maxSolenoidTemp = solenoidTemperatures[i];
    }
  }
  
  float maxTemp = max(driverModuleTemperature, maxSolenoidTemp);
  
  if (maxTemp > TEMP_THRESHOLD_LOW) {
    // Gradually reduce PWM duty as temperature rises
    // Linear reduction from 100% at 70°C to 50% at 85°C
    float tempAboveThreshold = maxTemp - TEMP_THRESHOLD_LOW;
    float tempRange = TEMP_THRESHOLD_SHUTDOWN - TEMP_THRESHOLD_LOW;  // 15°C range
    float throttleFactor = 1.0f - (tempAboveThreshold / tempRange) * 0.5f;  // Reduce to 50% max
    currentPWMThrottle = constrain(throttleFactor, 0.5f, 1.0f);  // Clamp between 50% and 100%
  } else {
    // Restore full power if temperatures cool
    currentPWMThrottle = 1.0f;
  }
}

void SelectCoolestSolenoid() {
  activeSolenoidID = FindCoolestSolenoid();
  Serial.printf("ECU: Session start - Selected solenoid %d (%.1f°C)\n",
    activeSolenoidID, solenoidTemperatures[activeSolenoidID]);
}

uint8_t FindCoolestSolenoid() {
  uint8_t coolestID = 0;
  float coolestTemp = solenoidTemperatures[0];
  
  for (uint8_t i = 1; i < numSolenoids; i++) {
    if (solenoidTemperatures[i] < coolestTemp) {
      coolestTemp = solenoidTemperatures[i];
      coolestID = i;
    }
  }
  
  return coolestID;
}

// =====================================
// HoloCade Command Handlers
// =====================================

void HoloCade_HandleBool(uint8_t channel, bool value) {
  if (channel == 7) {
    // Channel 7: Global emergency stop
    if (value) {
      Serial.println("ECU: EMERGENCY STOP");
      fireCommandActive = false;
      for (uint8_t i = 0; i < numSolenoids; i++) {
        DisableSolenoid(i);
      }
    }
  } else if (channel == 9) {
    // Channel 9: Play session active (from parent ECU)
    playSessionActive = value;
    Serial.printf("ECU: Play session %s\n", value ? "ACTIVE" : "INACTIVE");
    // If play session becomes inactive, stop firing immediately
    if (!value && fireCommandActive) {
      fireCommandActive = false;
      DisableSolenoid(activeSolenoidID);
      Serial.println("ECU: Fire stopped - play session inactive");
    }
  } else if (channel == (10 + STATION_ID)) {
    // Channel 10+n: Fire command for this station
    // Only allow firing if play session is active
    if (value && !fireCommandActive && playSessionActive) {
      // Start fire command (only if play session is active)
      fireCommandActive = true;
      fireStartTime = millis();
      Serial.printf("ECU: Fire command started (solenoid %d, intensity %.2f)\n",
        activeSolenoidID, fireIntensity);
    } else if (value && !playSessionActive) {
      // Fire command received but play session inactive - ignore
      Serial.println("ECU: Fire command ignored - play session inactive");
    } else if (!value && fireCommandActive) {
      // Stop fire command
      fireCommandActive = false;
      DisableSolenoid(activeSolenoidID);
      Serial.println("ECU: Fire command stopped");
    }
  }
}

void HoloCade_HandleFloat(uint8_t channel, float value) {
  if (channel == (20 + STATION_ID)) {
    // Channel 20+n: Fire intensity (0.0-1.0)
    fireIntensity = constrain(value, 0.0f, 1.0f);
  } else if (channel == 4) {
    // Channel 4: Fire duration (ms) - optional, can be sent separately
    fireDuration = (unsigned long)(constrain(value, 0.05f, 0.2f) * 1000.0f);  // 50-200ms
  }
}

// =====================================
// Telemetry Reporting
// =====================================

void SendTelemetry() {
  // Button states (Ch 10+n: combined state for backward compatibility)
  HoloCade_SendBool(10 + STATION_ID, button0State || button1State);  // Combined button state
  
  // TODO: Send individual button states on separate channels when needed
  // Ch 11+n: Button 0, Ch 12+n: Button 1 (if parent ECU needs individual parsing)
  
  // Fire intensity (Ch 20+n)
  HoloCade_SendFloat(20 + STATION_ID, fireIntensity);
  
  // Active solenoid temperature (Ch 30+n)
  HoloCade_SendFloat(30 + STATION_ID, solenoidTemperatures[activeSolenoidID]);
  
  // Active solenoid ID (Ch 40+n, if redundant)
  if (numSolenoids > 1) {
    HoloCade_SendInt32(40 + STATION_ID, (int32_t)activeSolenoidID);
  }
  
  // Total solenoid count (Ch 50+n, if redundant)
  if (numSolenoids > 1) {
    HoloCade_SendInt32(50 + STATION_ID, (int32_t)numSolenoids);
  }
  
  // Individual solenoid temperatures (Ch 60+n*2, 61+n*2 for first two solenoids)
  // Channel mapping: 60+STATION_ID*2 for first solenoid, 61+STATION_ID*2 for second
  // This allows up to 2 solenoids per station with clear channel mapping
  for (uint8_t i = 0; i < numSolenoids && i < 2; i++) {  // Send first 2 solenoids
    HoloCade_SendFloat(60 + STATION_ID * 2 + i, solenoidTemperatures[i]);
  }
  
  // Driver module temperature (Ch 70+n)
  HoloCade_SendFloat(70 + STATION_ID, driverModuleTemperature);
  
  // Fire command active (Ch 80+n)
  HoloCade_SendBool(80 + STATION_ID, fireCommandActive);
  
  // Thermal shutdown (Ch 90+n)
  HoloCade_SendBool(90 + STATION_ID, thermalShutdown);
  
  // PWM throttle (Ch 95+n)
  HoloCade_SendFloat(95 + STATION_ID, currentPWMThrottle);
  
  // Send gun pose struct (Struct 150+n) - placeholder
  // TODO: Implement pose struct transmission when tracker integration complete
  // HoloCade_SendBytes(150 + STATION_ID, (uint8_t*)&gunPose, sizeof(FGunPose));
}

// =====================================
// Tracker Pose Reading (Placeholder)
// =====================================

void ReadTrackerPose() {
  // TODO: Implement SteamVR Ultimate tracker reading
  // Options:
  // 1. Direct tracker reading via USB/Serial if tracker supports it
  // 2. Receive pose from parent ECU (if centralized tracker relay)
  // 3. Use OpenVR/SteamVR API if running on PC-based ECU (Raspberry Pi, Jetson)
  
  // For now, pose remains at origin
  gunPose.PositionX = 0;
  gunPose.PositionY = 0;
  gunPose.PositionZ = 0;
  gunPose.RotationX = 0;
  gunPose.RotationY = 0;
  gunPose.RotationZ = 0;
  gunPose.RotationW = 1;
}

