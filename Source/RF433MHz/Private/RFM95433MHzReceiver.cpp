// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "RFM95433MHzReceiver.h"
#include "RF433MHz.h"

FRFM95433MHzReceiver::FRFM95433MHzReceiver()
	: bIsConnected(false)
{
}

FRFM95433MHzReceiver::~FRFM95433MHzReceiver()
{
	Shutdown();
}

bool FRFM95433MHzReceiver::Initialize(const FRF433MHzReceiverConfig& Config)
{
	// NOOP: RFM95 integration not yet implemented
	UE_LOG(LogRF433MHz, Warning, TEXT("RFM95433MHzReceiver: Initialize() is NOOP - RFM95 integration not yet implemented"));
	return false;
}

void FRFM95433MHzReceiver::Shutdown()
{
	bIsConnected = false;
}

bool FRFM95433MHzReceiver::IsConnected() const
{
	return bIsConnected;
}

bool FRFM95433MHzReceiver::GetButtonEvents(TArray<FRF433MHzButtonEvent>& OutEvents)
{
	OutEvents.Empty();
	return false;
}

bool FRFM95433MHzReceiver::IsRollingCodeValid() const
{
	return true;
}

int32 FRFM95433MHzReceiver::GetRollingCodeDrift() const
{
	return 0;
}

void FRFM95433MHzReceiver::EnableLearningMode(float TimeoutSeconds)
{
	// NOOP
}

void FRFM95433MHzReceiver::DisableLearningMode()
{
	// NOOP
}

bool FRFM95433MHzReceiver::IsLearningModeActive() const
{
	return false;
}

