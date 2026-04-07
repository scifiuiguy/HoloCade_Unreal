// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "4DOFPlatformController.h"
#include "Models/GunButtonEvents.h"
#include "Models/GunTelemetry.h"

U4DOFPlatformController::U4DOFPlatformController()
{
}

void U4DOFPlatformController::SendTiltStruct(const FTiltState& TiltState, int32 Channel)
{
	if (!bIsInitialized || !IsHardwareConnected())
	{
		UE_LOG(LogTemp, Warning, TEXT("4DOFPlatformController: Cannot send tilt struct - not initialized or not connected"));
		return;
	}

	// Send tilt as struct packet (default Channel 100)
	SendStruct<FTiltState>(Channel, TiltState);
	UE_LOG(LogTemp, Verbose, TEXT("4DOFPlatformController: Sent tilt struct on Ch%d - Pitch: %.2f, Roll: %.2f"), 
		Channel, TiltState.Pitch, TiltState.Roll);
}

void U4DOFPlatformController::SendScissorLiftStruct(const FScissorLiftState& LiftState, int32 Channel)
{
	if (!bIsInitialized || !IsHardwareConnected())
	{
		UE_LOG(LogTemp, Warning, TEXT("4DOFPlatformController: Cannot send scissor lift struct - not initialized or not connected"));
		return;
	}

	// Send scissor lift state as struct packet (default Channel 101)
	SendStruct<FScissorLiftState>(Channel, LiftState);
	UE_LOG(LogTemp, Verbose, TEXT("4DOFPlatformController: Sent scissor lift struct on Ch%d - Y: %.2f, Z: %.2f"), 
		Channel, LiftState.TranslationY, LiftState.TranslationZ);
}

bool U4DOFPlatformController::GetTiltStateFeedback(FTiltState& OutTiltState) const
{
	// Hardware sends tilt state feedback on Channel 100
	// Parse from received bytes cache
	TArray<uint8> ReceivedBytes = GetReceivedBytes(100);
	if (ReceivedBytes.Num() >= sizeof(FTiltState))
	{
		FMemory::Memcpy(&OutTiltState, ReceivedBytes.GetData(), sizeof(FTiltState));
		return true;
	}
	return false;
}

bool U4DOFPlatformController::GetScissorLiftStateFeedback(FScissorLiftState& OutLiftState) const
{
	// Hardware sends scissor lift state feedback on Channel 101
	// Parse from received bytes cache
	TArray<uint8> ReceivedBytes = GetReceivedBytes(101);
	if (ReceivedBytes.Num() >= sizeof(FScissorLiftState))
	{
		FMemory::Memcpy(&OutLiftState, ReceivedBytes.GetData(), sizeof(FScissorLiftState));
		return true;
	}
	return false;
}

bool U4DOFPlatformController::GetGunButtonEvents(FGunButtonEvents& OutButtonEvents) const
{
	// Hardware sends button events on Channel 310
	// Parse from received bytes cache
	TArray<uint8> ReceivedBytes = GetReceivedBytes(310);
	if (ReceivedBytes.Num() >= sizeof(FGunButtonEvents))
	{
		FMemory::Memcpy(&OutButtonEvents, ReceivedBytes.GetData(), sizeof(FGunButtonEvents));
		return true;
	}
	return false;
}

bool U4DOFPlatformController::GetGunTelemetry(FGunTelemetry& OutTelemetry) const
{
	// Hardware sends gun telemetry on Channel 311
	// Parse from received bytes cache
	TArray<uint8> ReceivedBytes = GetReceivedBytes(311);
	if (ReceivedBytes.Num() >= sizeof(FGunTelemetry))
	{
		FMemory::Memcpy(&OutTelemetry, ReceivedBytes.GetData(), sizeof(FGunTelemetry));
		return true;
	}
	return false;
}

