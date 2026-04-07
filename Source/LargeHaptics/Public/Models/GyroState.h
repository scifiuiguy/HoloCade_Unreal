// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "GyroState.generated.h"

/**
 * Gyroscope state (continuous rotation pitch and roll)
 * 
 * Data model for efficient struct-based UDP transmission of gyroscope rotation.
 * Used by 2DOF Flight Sim (continuous rotation gyroscope with no limit switches).
 * 
 * This is a Model (M) in MVC architecture - pure data structure with built-in mapping functions.
 * Designed for UDP transport via HoloCade binary protocol (channel-agnostic struct packets).
 * 
 * Storage: Degrees (unlimited - allows negative values and values beyond 360°)
 * Input: Normalized joystick values (-1.0 to +1.0) or direct degrees
 * Output: Degrees for hardware control (servo motors with continuous rotation)
 * 
 * Continuous Rotation:
 * - No limit switches - can rotate infinitely in either direction
 * - Negative values = counter-clockwise rotation
 * - Positive values = clockwise rotation
 * - Values beyond 360° represent multiple full rotations
 * - Example: 450° = 1.25 rotations clockwise, -90° = 0.25 rotations counter-clockwise
 */
USTRUCT(BlueprintType)
struct LARGEHAPTICS_API FGyroState
{
	GENERATED_BODY()

	/** Pitch angle in degrees (unlimited - continuous rotation, no clamping) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Haptics|Gyro")
	float Pitch = 0.0f;

	/** Roll angle in degrees (unlimited - continuous rotation, no clamping) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Haptics|Gyro")
	float Roll = 0.0f;

	FGyroState() : Pitch(0.0f), Roll(0.0f) {}
	FGyroState(float InPitch, float InRoll) : Pitch(InPitch), Roll(InRoll) {}

	/**
	 * Create from normalized joystick input (-1.0 to +1.0)
	 * Maps to degrees based on rotation speed and delta time.
	 * @param NormalizedPitch - Joystick Y axis (-1.0 = full backward rotation, +1.0 = full forward rotation)
	 * @param NormalizedRoll - Joystick X axis (-1.0 = full left rotation, +1.0 = full right rotation)
	 * @param MaxRotationSpeedDegreesPerSecond - Maximum rotation speed (e.g., 60°/s)
	 * @param DeltaTime - Time since last update (seconds)
	 * @param CurrentPitch - Current pitch angle (for cumulative rotation)
	 * @param CurrentRoll - Current roll angle (for cumulative rotation)
	 * @return GyroState struct with degrees updated based on normalized input and rotation speed
	 */
	static FGyroState FromNormalized(float NormalizedPitch, float NormalizedRoll, float MaxRotationSpeedDegreesPerSecond, float DeltaTime, float CurrentPitch = 0.0f, float CurrentRoll = 0.0f)
	{
		FGyroState Gyro;
		float PitchDelta = FMath::Clamp(NormalizedPitch, -1.0f, 1.0f) * MaxRotationSpeedDegreesPerSecond * DeltaTime;
		float RollDelta = FMath::Clamp(NormalizedRoll, -1.0f, 1.0f) * MaxRotationSpeedDegreesPerSecond * DeltaTime;
		Gyro.Pitch = CurrentPitch + PitchDelta;
		Gyro.Roll = CurrentRoll + RollDelta;
		return Gyro;
	}

	/**
	 * Convert to normalized joystick values (-1.0 to +1.0)
	 * Note: This requires knowing the rotation speed and delta time, so it's less useful than ToNormalizedVelocity()
	 * @param MaxRotationSpeedDegreesPerSecond - Maximum rotation speed (e.g., 60°/s)
	 * @param DeltaTime - Time since last update (seconds)
	 * @param PreviousPitch - Previous pitch angle
	 * @param PreviousRoll - Previous roll angle
	 * @return Vector2D with normalized values (X = Roll, Y = Pitch) representing rotation velocity
	 */
	FVector2D ToNormalizedVelocity(float MaxRotationSpeedDegreesPerSecond, float DeltaTime, float PreviousPitch, float PreviousRoll) const
	{
		FVector2D Normalized;
		if (DeltaTime > 0.0f && MaxRotationSpeedDegreesPerSecond > 0.0f)
		{
			float PitchVelocity = (Pitch - PreviousPitch) / DeltaTime;
			float RollVelocity = (Roll - PreviousRoll) / DeltaTime;
			Normalized.X = FMath::Clamp(RollVelocity / MaxRotationSpeedDegreesPerSecond, -1.0f, 1.0f);
			Normalized.Y = FMath::Clamp(PitchVelocity / MaxRotationSpeedDegreesPerSecond, -1.0f, 1.0f);
		}
		else
		{
			Normalized = FVector2D::ZeroVector;
		}
		return Normalized;
	}

	/**
	 * Convert to 0-360 degree range (for display/UI purposes)
	 * Wraps values to 0-360 range while preserving rotation direction information
	 * @return Vector2D with wrapped values (X = Roll 0-360, Y = Pitch 0-360)
	 */
	FVector2D ToWrapped360() const
	{
		FVector2D Wrapped;
		Wrapped.X = FMath::Fmod(Roll + 360.0f, 360.0f);
		Wrapped.Y = FMath::Fmod(Pitch + 360.0f, 360.0f);
		return Wrapped;
	}

	/**
	 * Convert pitch to radians
	 */
	float GetPitchRadians() const { return FMath::DegreesToRadians(Pitch); }

	/**
	 * Convert roll to radians
	 */
	float GetRollRadians() const { return FMath::DegreesToRadians(Roll); }

	/**
	 * Get number of full rotations (for pitch)
	 * @return Number of complete 360° rotations (can be negative for counter-clockwise)
	 */
	int32 GetPitchFullRotations() const { return FMath::FloorToInt(Pitch / 360.0f); }

	/**
	 * Get number of full rotations (for roll)
	 * @return Number of complete 360° rotations (can be negative for counter-clockwise)
	 */
	int32 GetRollFullRotations() const { return FMath::FloorToInt(Roll / 360.0f); }
};

