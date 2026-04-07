// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "CarSimExperience.h"
#include "4DOFPlatformController.h"

ACarSimExperience::ACarSimExperience()
{
	PlatformController = CreateDefaultSubobject<U4DOFPlatformController>(TEXT("PlatformController"));
}

bool ACarSimExperience::InitializeExperienceImpl()
{
	if (!Super::InitializeExperienceImpl())
	{
		return false;
	}

	if (!PlatformController)
	{
		UE_LOG(LogTemp, Error, TEXT("CarSimExperience: Platform controller is null"));
		return false;
	}

	// Configure platform for car simulator
	FHapticPlatformConfig Config;
	Config.PlatformType = EHoloCadePlatformType::CarSim_SinglePlayer;
	Config.MaxPitchDegrees = MaxPitch;
	Config.MaxRollDegrees = MaxRoll;
	Config.MaxTranslationY = 50.0f;  // Lateral for sharp turns
	Config.MaxTranslationZ = 50.0f;  // Vertical for bumps
	Config.ControllerIPAddress = TEXT("192.168.1.100");
	Config.ControllerPort = 8080;

	if (!PlatformController->InitializePlatform(Config))
	{
		UE_LOG(LogTemp, Error, TEXT("CarSimExperience: Failed to initialize platform"));
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("CarSimExperience: Initialized successfully"));
	return true;
}

void ACarSimExperience::ShutdownExperienceImpl()
{
	if (PlatformController)
	{
		PlatformController->ReturnToNeutral(1.0f);
	}

	Super::ShutdownExperienceImpl();
}

void ACarSimExperience::SimulateCornerNormalized(float TurnIntensity, float Duration)
{
	if (!PlatformController)
	{
		return;
	}

	// Normalize to -1 to 1, then send as TiltX (roll)
	PlatformController->SendNormalizedMotion(TurnIntensity, 0.0f, 0.0f, Duration);
}

void ACarSimExperience::SimulateAccelerationNormalized(float AccelIntensity, float Duration)
{
	if (!PlatformController)
	{
		return;
	}

	// Normalize to -1 to 1, then send as TiltY (pitch)
	PlatformController->SendNormalizedMotion(0.0f, AccelIntensity, 0.0f, Duration);
}

void ACarSimExperience::SimulateCorner(float LeanAngle, float Duration)
{
	if (!PlatformController)
	{
		return;
	}

	FPlatformMotionCommand Command;
	Command.Pitch = 0.0f;
	Command.Roll = LeanAngle;
	Command.TranslationY = LeanAngle * 0.5f;  // Subtle lateral shift
	Command.TranslationZ = 0.0f;
	Command.Duration = Duration;

	PlatformController->SendMotionCommand(Command);
}

void ACarSimExperience::SimulateAcceleration(float PitchAngle, float Duration)
{
	if (!PlatformController)
	{
		return;
	}

	FPlatformMotionCommand Command;
	Command.Pitch = PitchAngle;
	Command.Roll = 0.0f;
	Command.TranslationY = 0.0f;
	Command.TranslationZ = 0.0f;
	Command.Duration = Duration;

	PlatformController->SendMotionCommand(Command);
}

void ACarSimExperience::SimulateBump(float Intensity, float Duration)
{
	if (!PlatformController)
	{
		return;
	}

	// Create quick up/down motion for bump effect
	FPlatformMotionCommand Command;
	Command.Pitch = 0.0f;
	Command.Roll = 0.0f;
	Command.TranslationY = 0.0f;
	Command.TranslationZ = Intensity * 20.0f;  // Scale intensity to cm
	Command.Duration = Duration * 0.5f;  // Quick rise

	PlatformController->SendMotionCommand(Command);

	// Return to neutral (simulated in real implementation with timer)
}

void ACarSimExperience::ReturnToNeutral(float Duration)
{
	if (PlatformController)
	{
		PlatformController->ReturnToNeutral(Duration);
	}
}

void ACarSimExperience::EmergencyStop()
{
	if (PlatformController)
	{
		PlatformController->EmergencyStop();
	}
}

