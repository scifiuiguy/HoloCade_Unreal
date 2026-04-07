// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "SuperheroFlight/SuperheroFlightExperience.h"
#include "HoloCadeExperiences.h"
#include "SuperheroFlight/SuperheroFlightECUController.h"
#include "SuperheroFlight/FlightHandsController.h"
#include "SuperheroFlight/GestureDebugger.h"
#include "RF433MHz/Public/RF433MHzReceiver.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

ASuperheroFlightExperience::ASuperheroFlightExperience()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.0f; // Tick every frame

	// Create components
	ECUController = CreateDefaultSubobject<USuperheroFlightECUController>(TEXT("ECUController"));
	FlightHandsController = CreateDefaultSubobject<UFlightHandsController>(TEXT("FlightHandsController"));
	GestureDebugger = CreateDefaultSubobject<UGestureDebugger>(TEXT("GestureDebugger"));
	RF433MHzReceiver = CreateDefaultSubobject<URF433MHzReceiver>(TEXT("RF433MHzReceiver"));

	// Initialize defaults
	AirHeight = 24.0f;
	ProneHeight = 36.0f;
	StandingGroundHeight = 0.0f;
	PlayerHeightCompensation = 1.0f;
	FlyingForwardSpeed = 10.0f;
	FlyingUpSpeed = 5.0f;
	FlyingDownSpeed = 8.0f;
	ArmLength = 28.0f;
	UpToForwardAngle = 45.0f;
	ForwardToDownAngle = 45.0f;
	CurrentGameState = ESuperheroFlightGameState::Standing;
	bPlaySessionActive = false;
	bEmergencyStop = false;
	CalibrationTimeout = 300.0f; // 5 minutes
}

bool ASuperheroFlightExperience::InitializeExperienceImpl()
{
	// No HMD mapper needed - FlightHandsController uses Unreal's native XR APIs directly

	// Initialize ECU connection
	if (!ECUController->InitializeECU(ECUIPAddress, ECUPort))
	{
		UE_LOG(LogSuperheroFlight, Error, TEXT("SuperheroFlightExperience: Failed to initialize ECU"));
		return false;
	}

	// Initialize flight hands controller (client-side)
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (!FlightHandsController->InitializeGestureController(PC))
		{
			UE_LOG(LogSuperheroFlight, Error, TEXT("SuperheroFlightExperience: Failed to initialize flight hands controller"));
			return false;
		}
	}

	// Initialize gesture debugger
	if (!GestureDebugger->InitializeDebugger(FlightHandsController))
	{
		UE_LOG(LogSuperheroFlight, Error, TEXT("SuperheroFlightExperience: Failed to initialize gesture debugger"));
		return false;
	}

	// Configure flight hands controller parameters
	FlightHandsController->UpToForwardAngle = UpToForwardAngle;
	FlightHandsController->ForwardToDownAngle = ForwardToDownAngle;
	FlightHandsController->ArmLength = ArmLength;

	// Initialize 433MHz RF receiver for height calibration
	if (RF433MHzReceiver)
	{
		FRF433MHzReceiverConfig RFConfig;
		RFConfig.ReceiverType = ERF433MHzReceiverType::Generic;  // Default to Generic, can be configured in Blueprint
		RFConfig.USBDevicePath = TEXT("COM3");  // Default, should be configured per installation
		RFConfig.bEnableRollingCodeValidation = true;
		RFConfig.bEnableReplayAttackPrevention = true;
		RFConfig.UpdateRate = 20.0f;  // 20 Hz

		if (RF433MHzReceiver->InitializeReceiver(RFConfig))
		{
			// Subscribe to RF button function events
			RF433MHzReceiver->OnButtonFunctionTriggered.AddDynamic(this, &ASuperheroFlightExperience::HandleCalibrationButton);
			UE_LOG(LogSuperheroFlight, Log, TEXT("SuperheroFlightExperience: RF433MHz receiver initialized"));
		}
		else
		{
			UE_LOG(LogSuperheroFlight, Warning, TEXT("SuperheroFlightExperience: Failed to initialize RF433MHz receiver - height calibration will be unavailable"));
		}
	}

	// Send initial parameters to ECU
	ECUController->SetAirHeight(AirHeight);
	ECUController->SetProneHeight(ProneHeight);
	ECUController->SetPlayerHeightCompensation(PlayerHeightCompensation);
	ECUController->SetGameState(CurrentGameState);
	ECUController->SetPlaySessionActive(bPlaySessionActive);

	UE_LOG(LogSuperheroFlight, Log, TEXT("SuperheroFlightExperience: Initialized"));
	return true;
}

void ASuperheroFlightExperience::ShutdownExperienceImpl()
{
	if (ECUController)
	{
		ECUController->ShutdownECU();
	}

	if (RF433MHzReceiver)
	{
		RF433MHzReceiver->ShutdownReceiver();
	}
}

void ASuperheroFlightExperience::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsInitialized)
	{
		return;
	}

	// Update winch positions based on current game state and gesture input
	UpdateWinchPositions(DeltaTime);

	// Check for gesture state changes
	FSuperheroFlightGestureState CurrentGestureState = FlightHandsController->GetGestureState();
	if (CurrentGestureState.CurrentFlightMode != LastGestureState.CurrentFlightMode ||
		CurrentGestureState.bBothFistsClosed != LastGestureState.bBothFistsClosed)
	{
		HandleGestureStateChanged(CurrentGestureState);
		LastGestureState = CurrentGestureState;
	}

	// Update calibration timeout
	if (!bPlaySessionActive)
	{
		CalibrationInactiveTime += DeltaTime;
		if (CalibrationInactiveTime > CalibrationTimeout)
		{
			UE_LOG(LogSuperheroFlight, Warning, TEXT("SuperheroFlightExperience: Calibration timeout - disabling calibration mode"));
			// Disable calibration mode by disabling RF receiver button processing
			if (RF433MHzReceiver && RF433MHzReceiver->IsConnected())
			{
				// Disable button function processing (calibration mode disabled)
				RF433MHzReceiver->OnButtonFunctionTriggered.RemoveDynamic(this, &ASuperheroFlightExperience::HandleCalibrationButton);
				UE_LOG(LogSuperheroFlight, Log, TEXT("SuperheroFlightExperience: Calibration mode disabled due to timeout"));
			}
		}
	}
	else
	{
		CalibrationInactiveTime = 0.0f;
	}

	// Get latest telemetry from ECU
	if (ECUController->IsECUConnected())
	{
		ECUController->GetDualWinchState(CurrentWinchState);
		ECUController->GetSystemTelemetry(CurrentTelemetry);
	}
}

void ASuperheroFlightExperience::UpdateWinchPositions(float DeltaTime)
{
	if (!ECUController->IsECUConnected() || bEmergencyStop)
	{
		return;
	}

	FSuperheroFlightGestureState GestureState = FlightHandsController->GetGestureState();
	float FrontPosition, RearPosition;
	CalculateTargetWinchPositions(FrontPosition, RearPosition);

	// Calculate speed based on flight mode and gesture throttle
	float Speed = 0.0f;
	switch (CurrentGameState)
	{
	case ESuperheroFlightGameState::FlightForward:
		Speed = FlyingForwardSpeed * GestureState.FlightSpeedThrottle;
		break;
	case ESuperheroFlightGameState::FlightUp:
		Speed = FlyingUpSpeed * GestureState.FlightSpeedThrottle;
		break;
	case ESuperheroFlightGameState::FlightDown:
		Speed = FlyingDownSpeed * GestureState.FlightSpeedThrottle;
		break;
	default:
		Speed = 6.0f; // Default speed for transitions
		break;
	}

	// Send winch commands to ECU
	ECUController->SetDualWinchPositions(FrontPosition, RearPosition, Speed);
}

void ASuperheroFlightExperience::HandleGestureStateChanged(const FSuperheroFlightGestureState& GestureState)
{
	// Transition to new flight mode based on gesture
	ESuperheroFlightGameState NewState = GestureState.CurrentFlightMode;

	// Handle virtual altitude landing (transition from flight-down to standing)
	if (NewState == ESuperheroFlightGameState::FlightDown &&
		GestureState.VirtualAltitude > 0.0f && GestureState.VirtualAltitude < 12.0f)
	{
		NewState = ESuperheroFlightGameState::Standing;
	}

	if (NewState != CurrentGameState)
	{
		TransitionToGameState(NewState);
	}
}

void ASuperheroFlightExperience::HandleCalibrationButton(int32 ButtonCode, const FString& FunctionName, bool bPressed)
{
	if (!bPressed)
	{
		return; // Only process button press events
	}

	// Apply safety interlocks
	if (!CheckCalibrationSafetyInterlocks(FunctionName))
	{
		return;
	}

	// Process calibration command
	float DeltaHeight = 0.0f;
	if (FunctionName == TEXT("HeightUp"))
	{
		DeltaHeight = 6.0f; // Move winch up 6 inches
	}
	else if (FunctionName == TEXT("HeightDown"))
	{
		DeltaHeight = -6.0f; // Move winch down 6 inches
	}

	// Get current winch positions and adjust
	FSuperheroFlightDualWinchState WinchState;
	if (ECUController->GetDualWinchState(WinchState))
	{
		float NewFrontPosition = WinchState.FrontWinchPosition + DeltaHeight;
		float NewRearPosition = WinchState.RearWinchPosition + DeltaHeight;
		ECUController->SetDualWinchPositions(NewFrontPosition, NewRearPosition, 6.0f); // Slow speed for calibration
	}

	// Reset calibration timeout
	CalibrationInactiveTime = 0.0f;
}

void ASuperheroFlightExperience::TransitionToGameState(ESuperheroFlightGameState NewState)
{
	CurrentGameState = NewState;
	ECUController->SetGameState(NewState);
	UE_LOG(LogSuperheroFlight, Log, TEXT("SuperheroFlightExperience: Transitioned to state %d"), (int32)NewState);
}

void ASuperheroFlightExperience::CalculateTargetWinchPositions(float& OutFrontPosition, float& OutRearPosition) const
{
	// Calculate target positions based on current game state
	switch (CurrentGameState)
	{
	case ESuperheroFlightGameState::Standing:
		OutFrontPosition = StandingGroundHeight;
		OutRearPosition = StandingGroundHeight;
		break;

	case ESuperheroFlightGameState::Hovering:
	case ESuperheroFlightGameState::FlightUp:
	case ESuperheroFlightGameState::FlightDown:
		OutFrontPosition = StandingGroundHeight + AirHeight;
		OutRearPosition = StandingGroundHeight + AirHeight;
		break;

	case ESuperheroFlightGameState::FlightForward:
		OutFrontPosition = StandingGroundHeight + AirHeight;
		OutRearPosition = StandingGroundHeight + (ProneHeight * PlayerHeightCompensation);
		break;

	default:
		OutFrontPosition = StandingGroundHeight;
		OutRearPosition = StandingGroundHeight;
		break;
	}
}

bool ASuperheroFlightExperience::CheckCalibrationSafetyInterlocks(const FString& FunctionName) const
{
	// Interlock 1: Calibration mode only
	if (bPlaySessionActive)
	{
		UE_LOG(LogSuperheroFlight, Warning, TEXT("SuperheroFlightExperience: Calibration disabled - play session active"));
		return false;
	}

	// Interlock 2: Emergency stop precedence
	if (bEmergencyStop)
	{
		UE_LOG(LogSuperheroFlight, Warning, TEXT("SuperheroFlightExperience: Calibration disabled - emergency stop active"));
		return false;
	}

	// Interlock 3: Movement limits (enforced in HandleCalibrationButton - ±6 inches)
	// Interlock 4: Physical presence requirement (documented procedure, not enforced by code)
	// Interlock 5: Timeout protection (enforced in Tick)
	if (CalibrationInactiveTime > CalibrationTimeout)
	{
		UE_LOG(LogSuperheroFlight, Warning, TEXT("SuperheroFlightExperience: Calibration disabled - timeout"));
		return false;
	}

	// Interlock 6: Network isolation (enforced at network configuration level)

	return true;
}

void ASuperheroFlightExperience::AcknowledgeStandingGroundHeight()
{
	// Get current winch positions from ECU
	FSuperheroFlightDualWinchState WinchState;
	if (ECUController->GetDualWinchState(WinchState))
	{
		// Use front winch position as baseline (both should be at same height in standing mode)
		StandingGroundHeight = WinchState.FrontWinchPosition;
		ECUController->AcknowledgeStandingGroundHeight(StandingGroundHeight);
		UE_LOG(LogSuperheroFlight, Log, TEXT("SuperheroFlightExperience: Acknowledged standing ground height: %.2f inches"), StandingGroundHeight);
	}
}

