// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "I433MHzReceiver.h"

/**
 * RTL-SDR USB Receiver Implementation
 * 
 * Implementation for RTL-SDR USB dongles (software-defined radio).
 * Uses librtlsdr library for USB communication.
 * 
 * NOOP implementation - actual RTL-SDR integration will be implemented later.
 */
class RF433MHZ_API FRTL433MHzReceiver : public I433MHzReceiver
{
public:
	FRTL433MHzReceiver();
	virtual ~FRTL433MHzReceiver();

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
	// TODO: Add RTL-SDR specific members (device handle, etc.)
};

