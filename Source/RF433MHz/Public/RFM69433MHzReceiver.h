// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "I433MHzReceiver.h"

/**
 * RFM69 USB Receiver Implementation
 * 
 * Implementation for RFM69-based USB modules.
 * Uses serial/COM port or vendor-specific USB API.
 * 
 * NOOP implementation - actual RFM69 integration will be implemented later.
 */
class RF433MHZ_API FRFM69433MHzReceiver : public I433MHzReceiver
{
public:
	FRFM69433MHzReceiver();
	virtual ~FRFM69433MHzReceiver();

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
	// TODO: Add RFM69 specific members
};

