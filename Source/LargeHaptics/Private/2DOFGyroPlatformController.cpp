// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "2DOFGyroPlatformController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "HOTASInputMappingContext.h"

U2DOFGyroPlatformController::U2DOFGyroPlatformController()
{
}

void U2DOFGyroPlatformController::BeginPlay()
{
	Super::BeginPlay();
}

bool U2DOFGyroPlatformController::InitializePlatform(const FHapticPlatformConfig& InConfig)
{
	// Call base class initialization first
	if (!Super::InitializePlatform(InConfig))
	{
		return false;
	}

	// Initialize HOTAS if configured
	if (Config.GyroscopeConfig.HOTASType != EHoloCadeHOTASType::None)
	{
		if (!InitializeHOTAS())
		{
			UE_LOG(LogTemp, Warning, TEXT("2DOFGyroPlatformController: HOTAS initialization failed, continuing without HOTAS"));
		}
	}

	return true;
}

void U2DOFGyroPlatformController::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Process HOTAS input and map to gyroscope rotation
	if (bIsInitialized && IsHOTASConnected() && Config.GyroscopeConfig.bEnableJoystick)
	{
		ProcessHOTASInputToGyro(DeltaTime);
	}
}

void U2DOFGyroPlatformController::SendGyroStruct(const FGyroState& GyroState, int32 Channel)
{
	if (!bIsInitialized || !IsHardwareConnected())
	{
		UE_LOG(LogTemp, Warning, TEXT("2DOFGyroPlatformController: Cannot send gyro struct - not initialized or not connected"));
		return;
	}

	// Send gyro state as struct packet (default Channel 102)
	SendStruct<FGyroState>(Channel, GyroState);
	UE_LOG(LogTemp, Verbose, TEXT("2DOFGyroPlatformController: Sent gyro struct on Ch%d - Pitch: %.2f°, Roll: %.2f°"), 
		Channel, GyroState.Pitch, GyroState.Roll);
}

void U2DOFGyroPlatformController::SendGyroFromNormalized(float NormalizedPitch, float NormalizedRoll, float DeltaTime, float CurrentPitch, float CurrentRoll, int32 Channel)
{
	if (!bIsInitialized || !IsHardwareConnected())
	{
		UE_LOG(LogTemp, Warning, TEXT("2DOFGyroPlatformController: Cannot send gyro from normalized - not initialized or not connected"));
		return;
	}

	// Get max rotation speed from config
	float MaxRotationSpeed = 90.0f; // Default
	if (bIsInitialized)
	{
		MaxRotationSpeed = Config.GyroscopeConfig.MaxRotationSpeed;
	}

	// Create gyro state from normalized input
	FGyroState GyroState = FGyroState::FromNormalized(
		NormalizedPitch, 
		NormalizedRoll, 
		MaxRotationSpeed, 
		DeltaTime, 
		CurrentPitch, 
		CurrentRoll
	);

	// Send as struct
	SendGyroStruct(GyroState, Channel);
}

void U2DOFGyroPlatformController::ProcessHOTASInputToGyro(float DeltaTime)
{
	if (!bIsInitialized || !IsHardwareConnected() || !IsHOTASConnected())
	{
		return;
	}

	// Get HOTAS joystick input (already normalized -1.0 to +1.0, with sensitivity applied)
	FVector2D JoystickInput = GetHOTASJoystickInput();
	
	// Map normalized joystick input to gyroscope rotation using struct's mapping function
	// JoystickInput.Y = Pitch (forward/backward on stick = pitch rotation)
	// JoystickInput.X = Roll (left/right on stick = roll rotation)
	// Use FGyroState::FromNormalized() to handle cumulative rotation (follows same pattern as FTiltState::FromNormalized())
	float MaxRotationSpeed = Config.GyroscopeConfig.MaxRotationSpeed;
	FGyroState GyroState = FGyroState::FromNormalized(
		JoystickInput.Y,  // NormalizedPitch
		JoystickInput.X,  // NormalizedRoll
		MaxRotationSpeed,
		DeltaTime,
		CurrentGyroPitch,
		CurrentGyroRoll
	);

	// Update current rotation angles from the struct (for next frame's cumulative rotation)
	CurrentGyroPitch = GyroState.Pitch;
	CurrentGyroRoll = GyroState.Roll;

	// Send gyroscope rotation command as struct packet
	SendGyroStruct(GyroState, 102);  // Channel 102 for gyro structs
}

FVector2D U2DOFGyroPlatformController::GetHOTASJoystickInput() const
{
	if (!bHOTASConnected || !Config.GyroscopeConfig.bEnableJoystick)
	{
		return FVector2D::ZeroVector;
	}
	return HOTASJoystickInput * Config.GyroscopeConfig.JoystickSensitivity;
}

float U2DOFGyroPlatformController::GetHOTASThrottleInput() const
{
	if (!bHOTASConnected || !Config.GyroscopeConfig.bEnableThrottle)
	{
		return 0.0f;
	}
	return HOTASThrottleInput * Config.GyroscopeConfig.ThrottleSensitivity;
}

float U2DOFGyroPlatformController::GetHOTASPedalInput() const
{
	if (!bHOTASConnected || !Config.GyroscopeConfig.bEnablePedals)
	{
		return 0.0f;
	}
	return HOTASPedalInput;
}

bool U2DOFGyroPlatformController::IsHOTASConnected() const
{
	return bHOTASConnected;
}

bool U2DOFGyroPlatformController::InitializeHOTAS()
{
	if (Config.GyroscopeConfig.HOTASType == EHoloCadeHOTASType::None)
	{
		return false;
	}

	// Create Enhanced Input Actions and Mapping Context programmatically
	CreateHOTASInputActions();

	// Bind to Enhanced Input Component
	BindHOTASInputActions();

	// Add Input Mapping Context to Enhanced Input Subsystem
	AActor* Owner = GetOwner();
	if (Owner)
	{
		APlayerController* PC = Owner->GetWorld()->GetFirstPlayerController();
		if (PC && PC->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
			{
				if (HOTASInputMappingContext)
				{
					Subsystem->AddMappingContext(HOTASInputMappingContext, 10); // Priority 10 for HOTAS
					UE_LOG(LogTemp, Log, TEXT("2DOFGyroPlatformController: Added HOTAS Input Mapping Context"));
				}
			}
		}
	}

	bHOTASConnected = true;
	UE_LOG(LogTemp, Log, TEXT("2DOFGyroPlatformController: HOTAS initialized successfully (Enhanced Input)"));
	return true;
}

void U2DOFGyroPlatformController::CreateHOTASInputActions()
{
	// Create Input Actions programmatically
	if (!HOTASInputAction_Pitch)
	{
		HOTASInputAction_Pitch = NewObject<UInputAction>(this, UInputAction::StaticClass(), TEXT("HOTAS_Pitch"));
		HOTASInputAction_Pitch->ValueType = EInputActionValueType::Axis1D;
	}

	if (!HOTASInputAction_Roll)
	{
		HOTASInputAction_Roll = NewObject<UInputAction>(this, UInputAction::StaticClass(), TEXT("HOTAS_Roll"));
		HOTASInputAction_Roll->ValueType = EInputActionValueType::Axis1D;
	}

	if (Config.GyroscopeConfig.bEnableThrottle && !HOTASInputAction_Throttle)
	{
		HOTASInputAction_Throttle = NewObject<UInputAction>(this, UInputAction::StaticClass(), TEXT("HOTAS_Throttle"));
		HOTASInputAction_Throttle->ValueType = EInputActionValueType::Axis1D;
	}

	if (Config.GyroscopeConfig.bEnablePedals && !HOTASInputAction_Pedals)
	{
		HOTASInputAction_Pedals = NewObject<UInputAction>(this, UInputAction::StaticClass(), TEXT("HOTAS_Pedals"));
		HOTASInputAction_Pedals->ValueType = EInputActionValueType::Axis1D;
	}

	// Create Input Mapping Context
	if (!HOTASInputMappingContext)
	{
		HOTASInputMappingContext = NewObject<UHOTASInputMappingContext>(this, UHOTASInputMappingContext::StaticClass(), TEXT("HOTAS_MappingContext"));

		// Map axes using configurable axis indices (defaults work for most devices)
		// Users can adjust these in the editor if their HOTAS uses different axis numbers
		
		// Map Pitch to configured axis (default: Axis 1)
		HOTASInputMappingContext->AddMapping(HOTASInputAction_Pitch, GetAxisKey(Config.GyroscopeConfig.PitchAxisIndex));

		// Map Roll to configured axis (default: Axis 0)
		HOTASInputMappingContext->AddMapping(HOTASInputAction_Roll, GetAxisKey(Config.GyroscopeConfig.RollAxisIndex));

		// Map Throttle to configured axis (default: Axis 2)
		if (Config.GyroscopeConfig.bEnableThrottle && HOTASInputAction_Throttle)
		{
			HOTASInputMappingContext->AddMapping(HOTASInputAction_Throttle, GetAxisKey(Config.GyroscopeConfig.ThrottleAxisIndex));
		}

		// Map Pedals to configured axis (default: Axis 3)
		if (Config.GyroscopeConfig.bEnablePedals && HOTASInputAction_Pedals)
		{
			HOTASInputMappingContext->AddMapping(HOTASInputAction_Pedals, GetAxisKey(Config.GyroscopeConfig.PedalsAxisIndex));
		}

		UE_LOG(LogTemp, Log, TEXT("2DOFGyroPlatformController: Created HOTAS Input Actions and Mapping Context"));
	}
}

void U2DOFGyroPlatformController::BindHOTASInputActions()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(Owner->InputComponent);
	if (!EIC)
	{
		// Try to get from PlayerController
		APlayerController* PC = Owner->GetWorld()->GetFirstPlayerController();
		if (PC)
		{
			EIC = Cast<UEnhancedInputComponent>(PC->InputComponent);
		}
	}

	if (!EIC)
	{
		UE_LOG(LogTemp, Warning, TEXT("2DOFGyroPlatformController: Enhanced Input Component not found. HOTAS input will not work."));
		return;
	}

	// Bind Pitch axis (continuous triggering)
	if (HOTASInputAction_Pitch)
	{
		EIC->BindAction(HOTASInputAction_Pitch, ETriggerEvent::Triggered, this, &U2DOFGyroPlatformController::OnHOTASPitchChanged);
	}

	// Bind Roll axis (continuous triggering)
	if (HOTASInputAction_Roll)
	{
		EIC->BindAction(HOTASInputAction_Roll, ETriggerEvent::Triggered, this, &U2DOFGyroPlatformController::OnHOTASRollChanged);
	}

	// Bind Throttle axis
	if (Config.GyroscopeConfig.bEnableThrottle && HOTASInputAction_Throttle)
	{
		EIC->BindAction(HOTASInputAction_Throttle, ETriggerEvent::Triggered, this, &U2DOFGyroPlatformController::OnHOTASThrottleChanged);
	}

	// Bind Pedals axis
	if (Config.GyroscopeConfig.bEnablePedals && HOTASInputAction_Pedals)
	{
		EIC->BindAction(HOTASInputAction_Pedals, ETriggerEvent::Triggered, this, &U2DOFGyroPlatformController::OnHOTASPedalsChanged);
	}

	UE_LOG(LogTemp, Log, TEXT("2DOFGyroPlatformController: Bound HOTAS Input Actions to Enhanced Input Component"));
}

FKey U2DOFGyroPlatformController::GetAxisKey(int32 AxisIndex) const
{
	// Map axis index to GenericUSBController axis key
	// Unreal supports GenericUSBController_Axis0 through GenericUSBController_Axis15
	// Use FKey constructor with string name since EKeys enum doesn't have these
	FString AxisKeyName = FString::Printf(TEXT("GenericUSBController_Axis%d"), AxisIndex);
	FKey AxisKey = FKey(*AxisKeyName);
	
	// Verify the key is valid
	if (!AxisKey.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("2DOFGyroPlatformController: Invalid axis index %d, defaulting to Axis0"), AxisIndex);
		return FKey(TEXT("GenericUSBController_Axis0"));
	}
	
	return AxisKey;
}

void U2DOFGyroPlatformController::OnHOTASPitchChanged(const FInputActionValue& Value)
{
	if (!bHOTASConnected)
	{
		return;
	}

	float PitchValue = Value.Get<float>();
	
	// Apply axis inversion if configured
	if (Config.GyroscopeConfig.bInvertPitchAxis)
	{
		PitchValue *= -1.0f;
	}

	HOTASJoystickInput.Y = PitchValue;
}

void U2DOFGyroPlatformController::OnHOTASRollChanged(const FInputActionValue& Value)
{
	if (!bHOTASConnected)
	{
		return;
	}

	float RollValue = Value.Get<float>();
	
	// Apply axis inversion if configured
	if (Config.GyroscopeConfig.bInvertRollAxis)
	{
		RollValue *= -1.0f;
	}

	HOTASJoystickInput.X = RollValue;
}

void U2DOFGyroPlatformController::OnHOTASThrottleChanged(const FInputActionValue& Value)
{
	if (!bHOTASConnected || !Config.GyroscopeConfig.bEnableThrottle)
	{
		return;
	}

	HOTASThrottleInput = Value.Get<float>();
}

void U2DOFGyroPlatformController::OnHOTASPedalsChanged(const FInputActionValue& Value)
{
	if (!bHOTASConnected || !Config.GyroscopeConfig.bEnablePedals)
	{
		return;
	}

	HOTASPedalInput = Value.Get<float>();
}

