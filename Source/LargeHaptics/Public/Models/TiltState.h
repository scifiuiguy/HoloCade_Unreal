// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "TiltState.generated.h"

/**
 * Tilt state (pitch and roll only)
 * 
 * Data model for efficient struct-based UDP transmission of platform tilt.
 * Used by 4DOF motion platforms: Gunship, MovingPlatform, CarSim.
 * 
 * This is a Model (M) in MVC architecture - pure data structure with built-in mapping functions.
 * Designed for UDP transport via HoloCade binary protocol (channel-agnostic struct packets).
 * 
 * Storage: Degrees (clamped to hardware limits)
 * Input: Normalized joystick values (-1.0 to +1.0)
 * Output: Degrees for hardware control
 */
USTRUCT(BlueprintType)
struct LARGEHAPTICS_API FTiltState
{
	GENERATED_BODY()

	/** Pitch angle in degrees (stored value, clamped to hardware limits) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Haptics|Tilt")
	float Pitch = 0.0f;

	/** Roll angle in degrees (stored value, clamped to hardware limits) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Haptics|Tilt")
	float Roll = 0.0f;

	FTiltState() : Pitch(0.0f), Roll(0.0f) {}
	FTiltState(float InPitch, float InRoll) : Pitch(InPitch), Roll(InRoll) {}

	/**
	 * Create from normalized joystick input (-1.0 to +1.0)
	 * @param NormalizedPitch - Joystick Y axis (-1.0 = full backward, +1.0 = full forward)
	 * @param NormalizedRoll - Joystick X axis (-1.0 = full left, +1.0 = full right)
	 * @param MaxPitchDegrees - Maximum pitch angle in degrees (hardware limit)
	 * @param MaxRollDegrees - Maximum roll angle in degrees (hardware limit)
	 * @return Tilt struct with degrees mapped from normalized input
	 */
	static FTiltState FromNormalized(float NormalizedPitch, float NormalizedRoll, float MaxPitchDegrees, float MaxRollDegrees)
	{
		FTiltState Tilt;
		Tilt.Pitch = FMath::Clamp(NormalizedPitch, -1.0f, 1.0f) * MaxPitchDegrees;
		Tilt.Roll = FMath::Clamp(NormalizedRoll, -1.0f, 1.0f) * MaxRollDegrees;
		return Tilt;
	}

	/**
	 * Convert to normalized joystick values (-1.0 to +1.0)
	 * @param MaxPitchDegrees - Maximum pitch angle in degrees (hardware limit)
	 * @param MaxRollDegrees - Maximum roll angle in degrees (hardware limit)
	 * @return Vector2D with normalized values (X = Roll, Y = Pitch)
	 */
	FVector2D ToNormalized(float MaxPitchDegrees, float MaxRollDegrees) const
	{
		FVector2D Normalized;
		Normalized.X = (MaxRollDegrees > 0.0f) ? FMath::Clamp(Roll / MaxRollDegrees, -1.0f, 1.0f) : 0.0f;
		Normalized.Y = (MaxPitchDegrees > 0.0f) ? FMath::Clamp(Pitch / MaxPitchDegrees, -1.0f, 1.0f) : 0.0f;
		return Normalized;
	}

	/**
	 * Convert pitch to radians
	 */
	float GetPitchRadians() const { return FMath::DegreesToRadians(Pitch); }

	/**
	 * Convert roll to radians
	 */
	float GetRollRadians() const { return FMath::DegreesToRadians(Roll); }
};

