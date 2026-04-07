/*
 * HoloCade Wireless RX Template Header
 * 
 * Standalone wireless reception template for HoloCade EmbeddedSystems protocol.
 * Include this header in your microcontroller sketch to easily receive commands from Unreal Engine.
 * 
 * Supports multiple platforms:
 * - ESP32 (WiFi UDP)
 * - ESP8266 (WiFi UDP)
 * - Arduino with WiFi Shield (WiFi UDP)
 * - STM32 with WiFi module (WiFi UDP)
 * - Raspberry Pi (WiFi UDP)
 * - Jetson Nano (WiFi UDP)
 * 
 * For platforms without built-in wireless, see HoloCade_Serial_RX.h
 * 
 * Protocol: Binary HoloCade protocol
 * Packet Format: [0xAA][Type][Channel][Payload...][CRC]
 * 
 * Usage:
 *   #include "HoloCade_Wireless_RX.h"
 *   
 *   void setup() {
 *     HoloCade_Wireless_Init("VR_Arcade_LAN", "password", 8888);
 *   }
 *   
 *   void loop() {
 *     HoloCade_ProcessIncoming();  // Call this regularly
 *   }
 *   
 *   // Implement handlers
 *   void HoloCade_HandleBool(uint8_t channel, bool value) {
 *     // Handle bool command
 *   }
 *   
 *   void HoloCade_HandleFloat(uint8_t channel, float value) {
 *     // Handle float command
 *   }
 * 
 * Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.
 */

#ifndef HoloCade_WIRELESS_RX_H
#define HoloCade_WIRELESS_RX_H

// Platform detection
#if defined(ESP32)
  #define HoloCade_PLATFORM_ESP
  #include <WiFi.h>
  #include <WiFiUdp.h>
#elif defined(ESP8266)
  #define HoloCade_PLATFORM_ESP
  #include <ESP8266WiFi.h>
  #include <WiFiUdp.h>
#elif defined(ARDUINO_ARCH_STM32)
  #define HoloCade_PLATFORM_STM32
  #include <WiFi.h>
  #include <WiFiUdp.h>
#elif defined(__RASPBERRY_PI__) || defined(RASPBERRY_PI)
  #define HoloCade_PLATFORM_RASPBERRY_PI
  #include <WiFi.h>
  #include <WiFiUdp.h>
#elif defined(__JETSON_NANO__) || defined(JETSON_NANO)
  #define HoloCade_PLATFORM_JETSON
  #include <WiFi.h>
  #include <WiFiUdp.h>
#else
  #error "HoloCade_Wireless_RX.h: Platform not supported. Use HoloCade_Serial_RX.h for serial communication."
#endif

// Protocol constants
#define HoloCade_PACKET_START_MARKER 0xAA

enum HoloCadeDataType {
  HoloCade_TYPE_BOOL = 0,
  HoloCade_TYPE_INT32 = 1,
  HoloCade_TYPE_FLOAT = 2,
  HoloCade_TYPE_STRING = 3,
  HoloCade_TYPE_BYTES = 4
};

// Global UDP object
#if defined(HoloCade_PLATFORM_ESP) || defined(HoloCade_PLATFORM_STM32)
WiFiUDP HoloCade_UDP;
#elif defined(HoloCade_PLATFORM_RASPBERRY_PI) || defined(HoloCade_PLATFORM_JETSON)
WiFiUDP HoloCade_UDP;
#else
#error "Platform UDP not defined"
#endif

// Configuration
uint16_t HoloCade_LocalPort = 8888;
bool HoloCade_Initialized = false;

// Handler function prototypes (implement these in your sketch)
void HoloCade_HandleBool(uint8_t channel, bool value);
void HoloCade_HandleInt32(uint8_t channel, int32_t value);
void HoloCade_HandleFloat(uint8_t channel, float value);
void HoloCade_HandleString(uint8_t channel, const char* str, uint8_t length);
void HoloCade_HandleBytes(uint8_t channel, uint8_t* data, uint8_t length);

/**
 * Initialize wireless communication
 * @param ssid WiFi network name
 * @param password WiFi password
 * @param localPort UDP port to listen on (default 8888)
 */
void HoloCade_Wireless_Init(const char* ssid, const char* password, uint16_t localPort = 8888) {
  Serial.begin(115200);
  Serial.println("\nHoloCade Wireless RX Initializing...");
  
  HoloCade_LocalPort = localPort;
  
  // Connect to WiFi
  Serial.printf("Connecting to WiFi: %s\n", ssid);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWiFi connected!");
  Serial.printf("Local IP: %s\n", WiFi.localIP().toString().c_str());
  
  // Start UDP listener
  HoloCade_UDP.begin(HoloCade_LocalPort);
  Serial.printf("UDP listening on port %d\n", HoloCade_LocalPort);
  
  HoloCade_Initialized = true;
  Serial.println("HoloCade Wireless RX Ready!");
}

/**
 * Calculate CRC checksum
 */
uint8_t HoloCade_CalculateCRC(uint8_t* data, int length) {
  uint8_t crc = 0;
  for (int i = 0; i < length; i++) {
    crc ^= data[i];
  }
  return crc;
}

/**
 * Process incoming packets
 * Call this regularly in your loop()
 */
void HoloCade_ProcessIncoming() {
  if (!HoloCade_Initialized) return;
  
  int packetSize = HoloCade_UDP.parsePacket();
  if (packetSize == 0) return;
  
  // Read packet
  uint8_t buffer[256];
  int len = HoloCade_UDP.read(buffer, 256);
  
  if (len < 5) {
    Serial.println("HoloCade: Packet too small");
    return;
  }
  
  // Validate start marker
  if (buffer[0] != HoloCade_PACKET_START_MARKER) {
    Serial.printf("HoloCade: Invalid start marker: 0x%02X\n", buffer[0]);
    return;
  }
  
  // Validate CRC
  uint8_t receivedCRC = buffer[len - 1];
  uint8_t calculatedCRC = HoloCade_CalculateCRC(buffer, len - 1);
  if (receivedCRC != calculatedCRC) {
    Serial.println("HoloCade: CRC mismatch");
    return;
  }
  
  // Parse packet
  uint8_t type = buffer[1];
  uint8_t channel = buffer[2];
  
  switch (type) {
    case HoloCade_TYPE_BOOL:
      HoloCade_HandleBool(channel, buffer[3] != 0);
      break;
      
    case HoloCade_TYPE_INT32:
      if (len >= 8) {
        int32_t value = (int32_t)buffer[3] | 
                       ((int32_t)buffer[4] << 8) | 
                       ((int32_t)buffer[5] << 16) | 
                       ((int32_t)buffer[6] << 24);
        HoloCade_HandleInt32(channel, value);
      }
      break;
      
    case HoloCade_TYPE_FLOAT:
      if (len >= 8) {
        uint32_t intValue = (uint32_t)buffer[3] | 
                           ((uint32_t)buffer[4] << 8) | 
                           ((uint32_t)buffer[5] << 16) | 
                           ((uint32_t)buffer[6] << 24);
        float value = *(float*)&intValue;
        HoloCade_HandleFloat(channel, value);
      }
      break;
      
    case HoloCade_TYPE_STRING:
      if (len >= 5) {
        uint8_t strLen = buffer[3];
        if (strLen > 0 && len >= 5 + strLen) {
          char str[256];
          memcpy(str, &buffer[4], strLen);
          str[strLen] = '\0';
          HoloCade_HandleString(channel, str, strLen);
        }
      }
      break;
      
    case HoloCade_TYPE_BYTES:
      if (len >= 5) {
        uint8_t byteLen = buffer[3];
        if (byteLen > 0 && len >= 5 + byteLen) {
          // Extract bytes (skip length byte at buffer[3])
          HoloCade_HandleBytes(channel, &buffer[4], byteLen);
        }
      }
      break;
      
    default:
      Serial.printf("HoloCade: Unknown type: %d\n", type);
      break;
  }
}

// Default handler implementations (override in your sketch)
__attribute__((weak)) void HoloCade_HandleBool(uint8_t channel, bool value) {
  Serial.printf("HoloCade: Bool - Ch:%d Val:%s\n", channel, value ? "true" : "false");
}

__attribute__((weak)) void HoloCade_HandleInt32(uint8_t channel, int32_t value) {
  Serial.printf("HoloCade: Int32 - Ch:%d Val:%d\n", channel, value);
}

__attribute__((weak)) void HoloCade_HandleFloat(uint8_t channel, float value) {
  Serial.printf("HoloCade: Float - Ch:%d Val:%.3f\n", channel, value);
}

__attribute__((weak)) void HoloCade_HandleString(uint8_t channel, const char* str, uint8_t length) {
  Serial.printf("HoloCade: String - Ch:%d Val:%s\n", channel, str);
}

__attribute__((weak)) void HoloCade_HandleBytes(uint8_t channel, uint8_t* data, uint8_t length) {
  Serial.printf("HoloCade: Bytes - Ch:%d Len:%d\n", channel, length);
  // Default implementation just logs - override in your sketch to parse struct packets
}

#endif // HoloCade_WIRELESS_RX_H

