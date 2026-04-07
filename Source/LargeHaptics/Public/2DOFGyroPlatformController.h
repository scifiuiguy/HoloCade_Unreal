// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "HapticPlatformController.h"
#include "Models/GyroState.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "HOTASInputMappingContext.h"
#include "2DOFGyroPlatformController.generated.h"

// Forward declarations
class UInputAction;
class UEnhancedInputComponent;

/**
 * 2DOF Gyroscope Platform Controller
 * 
 * Specialized controller for 2DOF continuous rotation gyroscopes that use:
 * - Continuous pitch rotation (servo motor, no limit switches)
 * - Continuous roll rotation (servo motor, no limit switches)
 * 
 * Used by Experience Genre Templates:
 * - FlightSimExperience (single-player flight simulator with HOTAS)
 * 
 * This subclass provides struct-based transmission methods for efficient
 * UDP communication with hardware ECUs that support continuous rotation.
 */
UCLASS(ClassGroup=(HoloCade), meta=(BlueprintSpawnableComponent))
class LARGEHAPTICS_API U2DOFGyroPlatformController : public UHapticPlatformController
{
	GENERATED_BODY()

public:
	U2DOFGyroPlatformController();

	/**
	 * Send gyroscope state (pitch and roll with continuous rotation) as a struct packet
	 * @param GyroState - The gyroscope state to send (unlimited degrees)
	 * @param Channel - Channel number for the struct packet (default: 102 for gyro structs)
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Haptics|2DOF Gyro")
	void SendGyroStruct(const FGyroState& GyroState, int32 Channel = 102);

	/**
	 * Send continuous rotation from normalized joystick input
	 * @param NormalizedPitch - Joystick Y axis (-1.0 = full backward rotation, +1.0 = full forward rotation)
	 * @param NormalizedRoll - Joystick X axis (-1.0 = full left rotation, +1.0 = full right rotation)
	 * @param DeltaTime - Time since last update (seconds)
	 * @param CurrentPitch - Current pitch angle (for cumulative rotation)
	 * @param CurrentRoll - Current roll angle (for cumulative rotation)
	 * @param Channel - Channel number for the struct packet (default: 102)
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Haptics|2DOF Gyro")
	void SendGyroFromNormalized(float NormalizedPitch, float NormalizedRoll, float DeltaTime, float CurrentPitch = 0.0f, float CurrentRoll = 0.0f, int32 Channel = 102);

	/**
	 * Get current HOTAS joystick input (X, Y axes)
	 * @return Vector2D with X (roll) and Y (pitch) input from joystick (-1.0 to 1.0)
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Haptics|2DOF Gyro|HOTAS")
	FVector2D GetHOTASJoystickInput() const;

	/**
	 * Get current HOTAS throttle input
	 * @return Throttle value (0.0 to 1.0)
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Haptics|2DOF Gyro|HOTAS")
	float GetHOTASThrottleInput() const;

	/**
	 * Get current HOTAS pedal input (if enabled)
	 * @return Pedal value (-1.0 to 1.0, left to right)
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Haptics|2DOF Gyro|HOTAS")
	float GetHOTASPedalInput() const;

	/**
	 * Check if HOTAS is connected and responding
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Haptics|2DOF Gyro|HOTAS")
	bool IsHOTASConnected() const;

public:
	/**
	 * Initialize the 2DOF gyroscope platform
	 * @param InConfig - Configuration settings
	 * @return true if initialization was successful
	 */
	bool InitializePlatform(const FHapticPlatformConfig& InConfig);

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	/** Current gyroscope rotation angles (for cumulative rotation) */
	float CurrentGyroPitch = 0.0f;
	float CurrentGyroRoll = 0.0f;

	/** HOTAS joystick input cache */
	FVector2D HOTASJoystickInput = FVector2D::ZeroVector;

	/** HOTAS throttle input cache */
	float HOTASThrottleInput = 0.0f;

	/** HOTAS pedal input cache */
	float HOTASPedalInput = 0.0f;

	/** Whether HOTAS is connected */
	bool bHOTASConnected = false;

	/** Enhanced Input Actions for HOTAS (created programmatically) */
	TObjectPtr<UInputAction> HOTASInputAction_Pitch = nullptr;
	TObjectPtr<UInputAction> HOTASInputAction_Roll = nullptr;
	TObjectPtr<UInputAction> HOTASInputAction_Throttle = nullptr;
	TObjectPtr<UInputAction> HOTASInputAction_Pedals = nullptr;

	/** Enhanced Input Mapping Context for HOTAS (created programmatically) */
	TObjectPtr<UHOTASInputMappingContext> HOTASInputMappingContext = nullptr;

	/**
	 * Process HOTAS input and send gyroscope commands
	 * Called from TickComponent when HOTAS is connected
	 */
	void ProcessHOTASInputToGyro(float DeltaTime);

	/**
	 * Initialize HOTAS controller connection (Enhanced Input)
	 */
	bool InitializeHOTAS();

	/**
	 * Create Enhanced Input Actions and Mapping Context for HOTAS (programmatically)
	 */
	void CreateHOTASInputActions();

	/**
	 * Bind HOTAS Input Actions to Enhanced Input Component
	 */
	void BindHOTASInputActions();

	/**
	 * Helper to get EKeys enum for GenericUSBController axis by index
	 * @param AxisIndex - Axis number (0-15)
	 * @return FKey for the specified axis
	 */
	FKey GetAxisKey(int32 AxisIndex) const;

	/**
	 * Enhanced Input callbacks for HOTAS axes
	 */
	void OnHOTASPitchChanged(const FInputActionValue& Value);
	void OnHOTASRollChanged(const FInputActionValue& Value);
	void OnHOTASThrottleChanged(const FInputActionValue& Value);
	void OnHOTASPedalsChanged(const FInputActionValue& Value);
};

