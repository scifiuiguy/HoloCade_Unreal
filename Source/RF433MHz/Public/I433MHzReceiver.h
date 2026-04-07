// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "RF433MHzTypes.h"

// Forward declarations
struct FRF433MHzReceiverConfig;
struct FRF433MHzButtonEvent;

/**
 * I433MHzReceiver - Interface for 433MHz USB receiver implementations
 * 
 * Provides a polymorphic interface for different USB receiver modules:
 * - RTL-SDR: Software-defined radio USB dongle
 * - CC1101: Dedicated 433MHz transceiver module with USB interface
 * - RFM69/RFM95: LoRa/RF modules with USB interface (433MHz capable)
 * - Generic: Off-the-shelf USB dongles available on Amazon/eBay
 * 
 * Each implementation handles module-specific drivers/APIs (libusb, serial/COM ports, proprietary SDKs)
 * and exposes a unified interface for game server code.
 */
class RF433MHZ_API I433MHzReceiver
{
public:
	virtual ~I433MHzReceiver() = default;

	/**
	 * Initialize receiver with configuration
	 * @param Config - Receiver configuration (type, USB device path, security settings)
	 * @return True if initialization successful
	 */
	virtual bool Initialize(const FRF433MHzReceiverConfig& Config) = 0;

	/**
	 * Shutdown receiver and close USB connection
	 */
	virtual void Shutdown() = 0;

	/**
	 * Check if receiver is connected and operational
	 * @return True if receiver is connected
	 */
	virtual bool IsConnected() const = 0;

	/**
	 * Get button events from receiver
	 * @param OutEvents - Array of button events received since last call
	 * @return True if valid events were received
	 */
	virtual bool GetButtonEvents(TArray<FRF433MHzButtonEvent>& OutEvents) = 0;

	/**
	 * Check if rolling code validation is enabled and valid
	 * @return True if rolling code is valid (or validation disabled)
	 */
	virtual bool IsRollingCodeValid() const = 0;

	/**
	 * Get rolling code drift (difference between expected and received code)
	 * @return Code drift value (0 = perfect match, positive = ahead, negative = behind)
	 */
	virtual int32 GetRollingCodeDrift() const = 0;

	/**
	 * Enable code learning mode (for pairing new remotes)
	 * @param TimeoutSeconds - How long learning mode stays active (0 = until disabled)
	 */
	virtual void EnableLearningMode(float TimeoutSeconds = 30.0f) = 0;

	/**
	 * Disable code learning mode
	 */
	virtual void DisableLearningMode() = 0;

	/**
	 * Check if learning mode is active
	 * @return True if learning mode is active
	 */
	virtual bool IsLearningModeActive() const = 0;

	/**
	 * Factory method to create the appropriate receiver based on configuration
	 * @param Config - Receiver configuration containing type and device-specific settings
	 * @return Receiver instance, or nullptr if creation failed
	 */
	static TUniquePtr<I433MHzReceiver> CreateReceiver(const FRF433MHzReceiverConfig& Config);
};

