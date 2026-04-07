// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "CC1101433MHzReceiver.h"
#include "RF433MHz.h"

FCC1101433MHzReceiver::FCC1101433MHzReceiver()
	: bIsConnected(false)
{
}

FCC1101433MHzReceiver::~FCC1101433MHzReceiver()
{
	Shutdown();
}

bool FCC1101433MHzReceiver::Initialize(const FRF433MHzReceiverConfig& Config)
{
	// NOOP: CC1101 integration not yet implemented
	UE_LOG(LogRF433MHz, Warning, TEXT("CC1101433MHzReceiver: Initialize() is NOOP - CC1101 integration not yet implemented"));
	return false;
}

void FCC1101433MHzReceiver::Shutdown()
{
	bIsConnected = false;
}

bool FCC1101433MHzReceiver::IsConnected() const
{
	return bIsConnected;
}

bool FCC1101433MHzReceiver::GetButtonEvents(TArray<FRF433MHzButtonEvent>& OutEvents)
{
	OutEvents.Empty();
	return false;
}

bool FCC1101433MHzReceiver::IsRollingCodeValid() const
{
	return true;
}

int32 FCC1101433MHzReceiver::GetRollingCodeDrift() const
{
	return 0;
}

void FCC1101433MHzReceiver::EnableLearningMode(float TimeoutSeconds)
{
	// NOOP
}

void FCC1101433MHzReceiver::DisableLearningMode()
{
	// NOOP
}

bool FCC1101433MHzReceiver::IsLearningModeActive() const
{
	return false;
}

