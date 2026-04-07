// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "RFM69433MHzReceiver.h"
#include "RF433MHz.h"

FRFM69433MHzReceiver::FRFM69433MHzReceiver()
	: bIsConnected(false)
{
}

FRFM69433MHzReceiver::~FRFM69433MHzReceiver()
{
	Shutdown();
}

bool FRFM69433MHzReceiver::Initialize(const FRF433MHzReceiverConfig& Config)
{
	// NOOP: RFM69 integration not yet implemented
	UE_LOG(LogRF433MHz, Warning, TEXT("RFM69433MHzReceiver: Initialize() is NOOP - RFM69 integration not yet implemented"));
	return false;
}

void FRFM69433MHzReceiver::Shutdown()
{
	bIsConnected = false;
}

bool FRFM69433MHzReceiver::IsConnected() const
{
	return bIsConnected;
}

bool FRFM69433MHzReceiver::GetButtonEvents(TArray<FRF433MHzButtonEvent>& OutEvents)
{
	OutEvents.Empty();
	return false;
}

bool FRFM69433MHzReceiver::IsRollingCodeValid() const
{
	return true;
}

int32 FRFM69433MHzReceiver::GetRollingCodeDrift() const
{
	return 0;
}

void FRFM69433MHzReceiver::EnableLearningMode(float TimeoutSeconds)
{
	// NOOP
}

void FRFM69433MHzReceiver::DisableLearningMode()
{
	// NOOP
}

bool FRFM69433MHzReceiver::IsLearningModeActive() const
{
	return false;
}

