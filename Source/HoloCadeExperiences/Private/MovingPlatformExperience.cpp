// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "MovingPlatformExperience.h"
#include "4DOFPlatformController.h"

AMovingPlatformExperience::AMovingPlatformExperience()
{
	PlatformController = CreateDefaultSubobject<U4DOFPlatformController>(TEXT("PlatformController"));
}

bool AMovingPlatformExperience::InitializeExperienceImpl()
{
	if (!Super::InitializeExperienceImpl())
	{
		return false;
	}

	if (!PlatformController)
	{
		UE_LOG(LogTemp, Error, TEXT("MovingPlatformExperience: Platform controller is null"));
		return false;
	}

	// Configure platform
	FHapticPlatformConfig Config;
	Config.PlatformType = EHoloCadePlatformType::MovingPlatform_SinglePlayer;
	Config.MaxPitchDegrees = MaxPitch;
	Config.MaxRollDegrees = MaxRoll;
	Config.MaxTranslationY = 0.0f;  // Lateral not typically used for standing platform
	Config.MaxTranslationZ = MaxVerticalTranslation;
	Config.ControllerIPAddress = TEXT("192.168.1.100");
	Config.ControllerPort = 8080;

	if (!PlatformController->InitializePlatform(Config))
	{
		UE_LOG(LogTemp, Error, TEXT("MovingPlatformExperience: Failed to initialize platform"));
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("MovingPlatformExperience: Initialized successfully"));
	return true;
}

void AMovingPlatformExperience::ShutdownExperienceImpl()
{
	if (PlatformController)
	{
		PlatformController->ReturnToNeutral(1.0f);
	}

	Super::ShutdownExperienceImpl();
}

void AMovingPlatformExperience::SendPlatformTilt(float TiltX, float TiltY, float VerticalOffset, float Duration)
{
	if (!PlatformController)
	{
		return;
	}

	// Use the normalized API - automatically scales to hardware capabilities
	PlatformController->SendNormalizedMotion(TiltX, TiltY, VerticalOffset, Duration);
}

void AMovingPlatformExperience::SendPlatformMotion(float Pitch, float Roll, float VerticalOffset, float Duration)
{
	if (!PlatformController)
	{
		return;
	}

	FPlatformMotionCommand Command;
	Command.Pitch = Pitch;
	Command.Roll = Roll;
	Command.TranslationY = 0.0f;
	Command.TranslationZ = VerticalOffset;
	Command.Duration = Duration;

	PlatformController->SendMotionCommand(Command);
}

void AMovingPlatformExperience::ReturnToNeutral(float Duration)
{
	if (PlatformController)
	{
		PlatformController->ReturnToNeutral(Duration);
	}
}

void AMovingPlatformExperience::EmergencyStop()
{
	if (PlatformController)
	{
		PlatformController->EmergencyStop();
	}
}

