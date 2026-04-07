// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "RTL433MHzReceiver.h"
#include "RF433MHz.h"

FRTL433MHzReceiver::FRTL433MHzReceiver()
	: bIsConnected(false)
{
}

FRTL433MHzReceiver::~FRTL433MHzReceiver()
{
	Shutdown();
}

bool FRTL433MHzReceiver::Initialize(const FRF433MHzReceiverConfig& Config)
{
	// NOOP: RTL-SDR integration not yet implemented
	UE_LOG(LogRF433MHz, Warning, TEXT("RTL433MHzReceiver: Initialize() is NOOP - RTL-SDR integration not yet implemented"));
	return false;
}

void FRTL433MHzReceiver::Shutdown()
{
	bIsConnected = false;
}

bool FRTL433MHzReceiver::IsConnected() const
{
	return bIsConnected;
}

bool FRTL433MHzReceiver::GetButtonEvents(TArray<FRF433MHzButtonEvent>& OutEvents)
{
	OutEvents.Empty();
	return false;
}

bool FRTL433MHzReceiver::IsRollingCodeValid() const
{
	return true;
}

int32 FRTL433MHzReceiver::GetRollingCodeDrift() const
{
	return 0;
}

void FRTL433MHzReceiver::EnableLearningMode(float TimeoutSeconds)
{
	// NOOP
}

void FRTL433MHzReceiver::DisableLearningMode()
{
	// NOOP
}

bool FRTL433MHzReceiver::IsLearningModeActive() const
{
	return false;
}

