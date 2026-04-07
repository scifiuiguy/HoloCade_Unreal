/*
 * HoloCade Wireless TX Template Header
 * 
 * Standalone wireless transmission template for HoloCade EmbeddedSystems protocol.
 * Include this header in your microcontroller sketch to easily send data to Unreal Engine.
 * 
 * Supports multiple platforms:
 * - ESP32 (WiFi UDP)
 * - ESP8266 (WiFi UDP)
 * - Arduino with WiFi Shield (WiFi UDP)
 * - STM32 with WiFi module (WiFi UDP)
 * - Raspberry Pi (WiFi UDP)
 * - Jetson Nano (WiFi UDP)
 * 
 * For platforms without built-in wireless, see HoloCade_Serial_TX.h
 * 
 * Protocol: Binary HoloCade protocol
 * Packet Format: [0xAA][Type][Channel][Payload...][CRC]
 * 
 * Usage:
 *   #include "HoloCade_Wireless_TX.h"
 *   
 *   void setup() {
 *     HoloCade_Wireless_Init("VR_Arcade_LAN", "password", IPAddress(192,168,1,100), 8888);
 *   }
 *   
 *   void loop() {
 *     HoloCade_SendBool(0, true);  // Send button press
 *     HoloCade_SendFloat(1, 0.75f); // Send sensor value
 *   }
 * 
 * Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.
 */

#ifndef HoloCade_WIRELESS_TX_H
#define HoloCade_WIRELESS_TX_H

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
  // STM32 with WiFi module - adjust includes based on your WiFi module
  #include <WiFi.h>
  #include <WiFiUdp.h>
#elif defined(__RASPBERRY_PI__) || defined(RASPBERRY_PI)
  #define HoloCade_PLATFORM_RASPBERRY_PI
  // Raspberry Pi - use standard socket libraries
  #include <WiFi.h>
  #include <WiFiUdp.h>
#elif defined(__JETSON_NANO__) || defined(JETSON_NANO)
  #define HoloCade_PLATFORM_JETSON
  // Jetson Nano - use standard socket libraries
  #include <WiFi.h>
  #include <WiFiUdp.h>
#else
  #error "HoloCade_Wireless_TX.h: Platform not supported. Use HoloCade_Serial_TX.h for serial communication."
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
IPAddress HoloCade_TargetIP(192, 168, 1, 100);
uint16_t HoloCade_TargetPort = 8888;
bool HoloCade_Initialized = false;

/**
 * Initialize wireless communication
 * @param ssid WiFi network name
 * @param password WiFi password
 * @param targetIP Unreal Engine PC IP address
 * @param targetPort UDP port (default 8888)
 */
void HoloCade_Wireless_Init(const char* ssid, const char* password, IPAddress targetIP, uint16_t targetPort = 8888) {
  Serial.begin(115200);
  Serial.println("\nHoloCade Wireless TX Initializing...");
  
  HoloCade_TargetIP = targetIP;
  HoloCade_TargetPort = targetPort;
  
  // Connect to WiFi
  Serial.printf("Connecting to WiFi: %s\n", ssid);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWiFi connected!");
  Serial.printf("Local IP: %s\n", WiFi.localIP().toString().c_str());
  Serial.printf("Target IP: %s:%d\n", HoloCade_TargetIP.toString().c_str(), HoloCade_TargetPort);
  
  HoloCade_Initialized = true;
  Serial.println("HoloCade Wireless TX Ready!");
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
 * Send bool value
 */
void HoloCade_SendBool(uint8_t channel, bool value) {
  if (!HoloCade_Initialized) return;
  
  uint8_t packet[5];
  packet[0] = HoloCade_PACKET_START_MARKER;
  packet[1] = HoloCade_TYPE_BOOL;
  packet[2] = channel;
  packet[3] = value ? 1 : 0;
  packet[4] = HoloCade_CalculateCRC(packet, 4);
  
  HoloCade_UDP.beginPacket(HoloCade_TargetIP, HoloCade_TargetPort);
  HoloCade_UDP.write(packet, 5);
  HoloCade_UDP.endPacket();
}

/**
 * Send int32 value
 */
void HoloCade_SendInt32(uint8_t channel, int32_t value) {
  if (!HoloCade_Initialized) return;
  
  uint8_t packet[8];
  packet[0] = HoloCade_PACKET_START_MARKER;
  packet[1] = HoloCade_TYPE_INT32;
  packet[2] = channel;
  packet[3] = (value) & 0xFF;
  packet[4] = (value >> 8) & 0xFF;
  packet[5] = (value >> 16) & 0xFF;
  packet[6] = (value >> 24) & 0xFF;
  packet[7] = HoloCade_CalculateCRC(packet, 7);
  
  HoloCade_UDP.beginPacket(HoloCade_TargetIP, HoloCade_TargetPort);
  HoloCade_UDP.write(packet, 8);
  HoloCade_UDP.endPacket();
}

/**
 * Send float value
 */
void HoloCade_SendFloat(uint8_t channel, float value) {
  if (!HoloCade_Initialized) return;
  
  uint8_t packet[8];
  packet[0] = HoloCade_PACKET_START_MARKER;
  packet[1] = HoloCade_TYPE_FLOAT;
  packet[2] = channel;
  
  // Reinterpret float as uint32 for byte-by-byte transmission
  uint32_t intValue = *(uint32_t*)&value;
  packet[3] = (intValue) & 0xFF;
  packet[4] = (intValue >> 8) & 0xFF;
  packet[5] = (intValue >> 16) & 0xFF;
  packet[6] = (intValue >> 24) & 0xFF;
  packet[7] = HoloCade_CalculateCRC(packet, 7);
  
  HoloCade_UDP.beginPacket(HoloCade_TargetIP, HoloCade_TargetPort);
  HoloCade_UDP.write(packet, 8);
  HoloCade_UDP.endPacket();
}

/**
 * Send string value
 */
void HoloCade_SendString(uint8_t channel, const char* str) {
  if (!HoloCade_Initialized) return;
  
  uint8_t len = strlen(str);
  if (len > 255) len = 255;
  
  uint8_t packet[256];
  packet[0] = HoloCade_PACKET_START_MARKER;
  packet[1] = HoloCade_TYPE_STRING;
  packet[2] = channel;
  packet[3] = len;
  memcpy(&packet[4], str, len);
  packet[4 + len] = HoloCade_CalculateCRC(packet, 4 + len);
  
  HoloCade_UDP.beginPacket(HoloCade_TargetIP, HoloCade_TargetPort);
  HoloCade_UDP.write(packet, 5 + len);
  HoloCade_UDP.endPacket();
}

/**
 * Send bytes/struct packet (for struct-based MVC pattern)
 */
void HoloCade_SendBytes(uint8_t channel, uint8_t* data, uint8_t length) {
  if (!HoloCade_Initialized) return;
  if (length > 255) length = 255;
  
  uint8_t packet[256];
  packet[0] = HoloCade_PACKET_START_MARKER;
  packet[1] = HoloCade_TYPE_BYTES;
  packet[2] = channel;
  packet[3] = length;
  memcpy(&packet[4], data, length);
  packet[4 + length] = HoloCade_CalculateCRC(packet, 4 + length);
  
  HoloCade_UDP.beginPacket(HoloCade_TargetIP, HoloCade_TargetPort);
  HoloCade_UDP.write(packet, 5 + length);
  HoloCade_UDP.endPacket();
}

#endif // HoloCade_WIRELESS_TX_H

