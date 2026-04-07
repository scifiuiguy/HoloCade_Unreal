// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "I433MHzReceiver.h"

/**
 * RFM95 USB Receiver Implementation
 * 
 * Implementation for RFM95-based USB modules (LoRa-capable, 433MHz capable).
 * Uses serial/COM port or vendor-specific USB API.
 * 
 * NOOP implementation - actual RFM95 integration will be implemented later.
 */
class RF433MHZ_API FRFM95433MHzReceiver : public I433MHzReceiver
{
public:
	FRFM95433MHzReceiver();
	virtual ~FRFM95433MHzReceiver();

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
	bool bIsConnected;
	// TODO: Add RFM95 specific members
};

