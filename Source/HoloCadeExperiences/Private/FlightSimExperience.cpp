// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "FlightSimExperience.h"
#include "2DOFGyroPlatformController.h"
#include "Models/GyroState.h"

AFlightSimExperience::AFlightSimExperience()
{
	GyroscopeController = CreateDefaultSubobject<U2DOFGyroPlatformController>(TEXT("GyroscopeController"));
	PrimaryActorTick.bCanEverTick = true;
}

bool AFlightSimExperience::InitializeExperienceImpl()
{
	if (!Super::InitializeExperienceImpl())
	{
		return false;
	}

	if (!GyroscopeController)
	{
		UE_LOG(LogTemp, Error, TEXT("FlightSimExperience: Gyroscope controller is null"));
		return false;
	}

	// Configure gyroscope system
	FHapticPlatformConfig Config;
	Config.PlatformType = EHoloCadePlatformType::FlightSim_2DOF;
	Config.ControllerIPAddress = TEXT("192.168.1.100");
	Config.ControllerPort = 8888;  // Match firmware UDP port

	// Configure gyroscope settings
	Config.GyroscopeConfig.bEnableContinuousPitch = true;
	Config.GyroscopeConfig.bEnableContinuousRoll = true;
	Config.GyroscopeConfig.MaxRotationSpeed = MaxRotationSpeed;

	// Configure HOTAS
	Config.GyroscopeConfig.HOTASType = HOTASType;
	Config.GyroscopeConfig.bEnableJoystick = bEnableJoystick;
	Config.GyroscopeConfig.bEnableThrottle = bEnableThrottle;
	Config.GyroscopeConfig.bEnablePedals = bEnablePedals;
	Config.GyroscopeConfig.JoystickSensitivity = JoystickSensitivity;
	Config.GyroscopeConfig.ThrottleSensitivity = ThrottleSensitivity;

	if (!GyroscopeController->InitializePlatform(Config))
	{
		UE_LOG(LogTemp, Error, TEXT("FlightSimExperience: Failed to initialize gyroscope"));
		return false;
	}

	if (GyroscopeController->IsHOTASConnected())
	{
		UE_LOG(LogTemp, Log, TEXT("FlightSimExperience: HOTAS connected successfully"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("FlightSimExperience: HOTAS not connected, using standard VR controllers"));
	}

	// Send gravity reset parameters to ECU on connect
	// Proposed channel mapping:
	//  - Ch9  (bool)  : GravityReset enable
	//  - Ch10 (float) : ResetSpeed (deg/s equivalent used by ECU smoothing)
	//  - Ch11 (float) : ResetIdleTimeout (seconds)
	GyroscopeController->SendBool(9, bGravityReset);
	GyroscopeController->SendFloat(10, ResetSpeed);
	GyroscopeController->SendFloat(11, ResetIdleTimeout);

	UE_LOG(LogTemp, Log, TEXT("FlightSimExperience: Initialized successfully"));
	return true;
}

void AFlightSimExperience::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateCockpitTransform(DeltaSeconds);
}

void AFlightSimExperience::ShutdownExperienceImpl()
{
	if (GyroscopeController)
	{
		GyroscopeController->ReturnToNeutral(2.0f);
	}

	Super::ShutdownExperienceImpl();
}

void AFlightSimExperience::SendContinuousRotation(float Pitch, float Roll, float Duration)
{
	if (!GyroscopeController)
	{
		return;
	}

	// Use struct-based MVC pattern for efficient UDP transmission
	// Create gyroscope state from absolute angles (unlimited degrees)
	FGyroState GyroState(Pitch, Roll);
	
	// Send as struct packet (Channel 102 for gyro structs) - more efficient: 1 UDP packet instead of 3
	GyroscopeController->SendGyroStruct(GyroState, 102);
	
	// Send duration separately (or could be part of a full command struct)
	GyroscopeController->SendFloat(4, Duration);
}

FVector2D AFlightSimExperience::GetJoystickInput() const
{
	if (GyroscopeController)
	{
		return GyroscopeController->GetHOTASJoystickInput();
	}
	return FVector2D::ZeroVector;
}

float AFlightSimExperience::GetThrottleInput() const
{
	if (GyroscopeController)
	{
		return GyroscopeController->GetHOTASThrottleInput();
	}
	return 0.0f;
}

float AFlightSimExperience::GetPedalInput() const
{
	if (GyroscopeController)
	{
		return GyroscopeController->GetHOTASPedalInput();
	}
	return 0.0f;
}

bool AFlightSimExperience::IsHOTASConnected() const
{
	if (GyroscopeController)
	{
		return GyroscopeController->IsHOTASConnected();
	}
	return false;
}

void AFlightSimExperience::ReturnToNeutral(float Duration)
{
	if (!GyroscopeController)
	{
		return;
	}

	// Send return to neutral command (Channel 8)
	GyroscopeController->SendBool(8, true);
	
	// Also send neutral gyro state
	FGyroState NeutralState(0.0f, 0.0f);
	GyroscopeController->SendGyroStruct(NeutralState, 102);
	GyroscopeController->SendFloat(4, Duration);
}

void AFlightSimExperience::EmergencyStop()
{
	if (!GyroscopeController)
	{
		return;
	}

	// Send emergency stop command (Channel 7)
	GyroscopeController->SendBool(7, true);
}

void AFlightSimExperience::UpdateCockpitTransform(float DeltaSeconds)
{
	if (!CockpitActor || !GyroscopeController)
	{
		return;
	}

	// Space reset is only active if both spaceReset and gravityReset are enabled
	const bool bSpaceResetActive = bSpaceReset && bGravityReset;

	// Consider stick idle if joystick magnitude is near zero
	const FVector2D Stick = GyroscopeController->GetHOTASJoystickInput();
	const bool bStickIdle = (FMath::Abs(Stick.X) < 0.05f) && (FMath::Abs(Stick.Y) < 0.05f);

	// If space reset active and stick idle: decouple cockpit (freeze at current rotation)
	if (bSpaceResetActive && bStickIdle)
	{
		if (!bCockpitDecoupled)
		{
			DecoupledCockpitRotation = CockpitActor->GetActorRotation();
			bCockpitDecoupled = true;
		}
		// Keep cockpit at saved rotation
		CockpitActor->SetActorRotation(DecoupledCockpitRotation);
		return;
	}

	// If we were decoupled, only recouple once platform is back near zero AND gravityReset has been turned off
	if (bCockpitDecoupled)
	{
		// Require gravityReset to be off before recoupling
		if (bGravityReset)
		{
			// Stay decoupled until gravityReset is false
			CockpitActor->SetActorRotation(DecoupledCockpitRotation);
			return;
		}

		// Check platform near zero using feedback
		FGyroState Feedback;
		bool bHasFeedback = false;
		// Try to read gyro feedback from channel 102
		{
			// Read bytes directly from transport cache
			const TArray<uint8> Bytes = GyroscopeController->GetReceivedBytes(102);
			if (Bytes.Num() >= sizeof(FGyroState))
			{
				FMemory::Memcpy(&Feedback, Bytes.GetData(), sizeof(FGyroState));
				bHasFeedback = true;
			}
		}

		if (bHasFeedback)
		{
			const bool bNearZero =
				(FMath::Abs(Feedback.Pitch) <= ZeroThresholdDegrees) &&
				(FMath::Abs(Feedback.Roll) <= ZeroThresholdDegrees);

			if (!bNearZero)
			{
				// Stay decoupled until platform near zero
				CockpitActor->SetActorRotation(DecoupledCockpitRotation);
				return;
			}
		}

		// Recouple
		bCockpitDecoupled = false;
	}

	// Normal mode: keep cockpit in sync with physical platform when possible
	{
		FGyroState Feedback;
		bool bHasFeedback = false;
		const TArray<uint8> Bytes = GyroscopeController->GetReceivedBytes(102);
		if (Bytes.Num() >= sizeof(FGyroState))
		{
			FMemory::Memcpy(&Feedback, Bytes.GetData(), sizeof(FGyroState));
			bHasFeedback = true;
		}

		if (bHasFeedback)
		{
			// Apply pitch/roll from feedback; preserve current yaw
			FRotator Current = CockpitActor->GetActorRotation();
			FRotator Target(Feedback.Pitch, Current.Yaw, Feedback.Roll);
			CockpitActor->SetActorRotation(Target);
		}
	}
}

