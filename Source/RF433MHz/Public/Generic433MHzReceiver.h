// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "I433MHzReceiver.h"
#include "RF433MHzTypes.h"

/**
 * Generic 433MHz USB Receiver Implementation
 * 
 * Default implementation for off-the-shelf USB dongles available on Amazon/eBay.
 * Uses serial/COM port communication (most generic receivers appear as USB serial devices).
 * 
 * This is a NOOP implementation - actual USB communication will be implemented based on
 * specific receiver hardware specifications.
 */
class RF433MHZ_API FGeneric433MHzReceiver : public I433MHzReceiver
{
public:
	FGeneric433MHzReceiver();
	virtual ~FGeneric433MHzReceiver();

	// I433MHzReceiver interface
	virtual bool Initialize(const FRF433MHzReceiverConfig& Config) override;
	virtual void Shutdown() override;
	virtual bool IsConnected() const override;
	virtual bool GetButtonEvents(TArray<FRF433MHzButtonEvent>& OutEvents) override;
	virtual bool IsRollingCodeValid() const override;
	virtual int32 GetRollingCodeDrift() const override;
	virtual void EnableLearningMode(float TimeoutSeconds = 30.0f) override;
	virtual void DisableLearningMode() override;
	virtual bool IsLearningModeActive() const override;

private:
	/** Configuration */
	FRF433MHzReceiverConfig CurrentConfig;

	/** Connection state */
	bool bIsConnected;

	/** Learning mode state */
	bool bLearningModeActive;
	float LearningModeTimeout;
	float LearningModeTimer;

	/** Rolling code state */
	uint32 ExpectedRollingCode;
	uint32 LastReceivedRollingCode;
	TArray<uint32> ReceivedRollingCodes; // For replay attack prevention

	/** Button event buffer */
	TArray<FRF433MHzButtonEvent> ButtonEventBuffer;

	/** Timestamp for replay attack prevention */
	float LastEventTimestamp;
	TMap<uint32, float> LastCodeTimestamps; // Rolling code -> timestamp

	/** Initialize USB serial connection */
	bool InitializeSerialConnection();

	/** Close USB serial connection */
	void CloseSerialConnection();

	/** Read data from USB serial port */
	bool ReadSerialData(TArray<uint8>& OutData);

	/** Parse button event from raw data */
	bool ParseButtonEvent(const TArray<uint8>& Data, FRF433MHzButtonEvent& OutEvent);

	/** Validate rolling code */
	bool ValidateRollingCode(uint32 Code);

	/** Check for replay attack */
	bool IsReplayAttack(uint32 RollingCode, float Timestamp);
};

