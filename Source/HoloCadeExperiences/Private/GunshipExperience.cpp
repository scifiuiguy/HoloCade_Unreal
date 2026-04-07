// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "GunshipExperience.h"
#include "4DOFPlatformController.h"

AGunshipExperience::AGunshipExperience()
{
	PlatformController = CreateDefaultSubobject<U4DOFPlatformController>(TEXT("PlatformController"));
	bMultiplayerEnabled = true;

	// Default 4-seat configuration
	SeatLocations.Add(FVector(-100, -100, 0));  // Front Left
	SeatLocations.Add(FVector(100, -100, 0));   // Front Right
	SeatLocations.Add(FVector(-100, 100, 0));   // Rear Left
	SeatLocations.Add(FVector(100, 100, 0));    // Rear Right
}

bool AGunshipExperience::InitializeExperienceImpl()
{
	if (!Super::InitializeExperienceImpl())
	{
		return false;
	}

	if (!PlatformController)
	{
		UE_LOG(LogTemp, Error, TEXT("GunshipExperience: Platform controller is null"));
		return false;
	}

	// Configure platform for 4-player gunship
	FHapticPlatformConfig Config;
	Config.PlatformType = EHoloCadePlatformType::Gunship_FourPlayer;
	Config.MaxPitchDegrees = MaxPitch;
	Config.MaxRollDegrees = MaxRoll;
	Config.MaxTranslationY = 100.0f;
	Config.MaxTranslationZ = 100.0f;
	Config.ControllerIPAddress = TEXT("192.168.1.100");
	Config.ControllerPort = 8080;

	if (!PlatformController->InitializePlatform(Config))
	{
		UE_LOG(LogTemp, Error, TEXT("GunshipExperience: Failed to initialize platform"));
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("GunshipExperience: Initialized for 4 players"));
	return true;
}

void AGunshipExperience::ShutdownExperienceImpl()
{
	if (PlatformController)
	{
		PlatformController->ReturnToNeutral(1.0f);
	}

	Super::ShutdownExperienceImpl();
}

void AGunshipExperience::SendGunshipTilt(float TiltX, float TiltY, float ForwardOffset, float VerticalOffset, float Duration)
{
	if (!PlatformController)
	{
		return;
	}

	// Use struct-based MVC pattern for efficient UDP transmission
	// Create tilt state from normalized input
	FTiltState TiltState = FTiltState::FromNormalized(TiltY, TiltX, MaxPitch, MaxRoll);
	
	// Create scissor lift state from normalized input
	FScissorLiftState LiftState = FScissorLiftState::FromNormalized(ForwardOffset, VerticalOffset, 100.0f, 100.0f);
	
	// Send as struct packets (more efficient: 2 UDP packets instead of 4)
	PlatformController->SendTiltStruct(TiltState, 100);
	PlatformController->SendScissorLiftStruct(LiftState, 101);
	
	// Send duration separately (or could be part of a full command struct)
	PlatformController->SendFloat(4, Duration);
}

void AGunshipExperience::SendGunshipMotion(float Pitch, float Roll, float ForwardOffset, float VerticalOffset, float Duration)
{
	if (!PlatformController)
	{
		return;
	}

	// Use struct-based MVC pattern for efficient UDP transmission
	// Option 1: Send as single full command struct (Channel 200) - most efficient
	FPlatformMotionCommand Command;
	Command.Pitch = FMath::Clamp(Pitch, -MaxPitch, MaxPitch);
	Command.Roll = FMath::Clamp(Roll, -MaxRoll, MaxRoll);
	// TranslationY = forward/reverse (scissor lift), TranslationZ = up/down (scissor lift)
	Command.TranslationY = ForwardOffset;
	Command.TranslationZ = VerticalOffset;
	Command.Duration = Duration;

	// Send as single struct packet (Channel 200) - 1 UDP packet instead of 5
	PlatformController->SendMotionCommand(Command, true);
}

void AGunshipExperience::ReturnToNeutral(float Duration)
{
	if (PlatformController)
	{
		PlatformController->ReturnToNeutral(Duration);
	}
}

void AGunshipExperience::EmergencyStop()
{
	if (PlatformController)
	{
		PlatformController->EmergencyStop();
	}
}

