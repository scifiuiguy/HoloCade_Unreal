// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "ScissorLiftState.generated.h"

/**
 * Scissor lift state (Y and Z translations only)
 * 
 * Data model for efficient struct-based UDP transmission of scissor lift position.
 * Used by 4DOF motion platforms: Gunship, MovingPlatform, CarSim.
 * 
 * This is a Model (M) in MVC architecture - pure data structure with built-in mapping functions.
 * Designed for UDP transport via HoloCade binary protocol (channel-agnostic struct packets).
 * 
 * Storage: Centimeters (clamped to hardware limits)
 * Input: Normalized values (-1.0 to +1.0)
 * Output: Centimeters for hardware control
 */
USTRUCT(BlueprintType)
struct LARGEHAPTICS_API FScissorLiftState
{
	GENERATED_BODY()

	/** Y translation in cm (forward/reverse, positive = forward, stored value) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Haptics|ScissorLift")
	float TranslationY = 0.0f;

	/** Z translation in cm (up/down, positive = up, stored value) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Haptics|ScissorLift")
	float TranslationZ = 0.0f;

	FScissorLiftState() : TranslationY(0.0f), TranslationZ(0.0f) {}
	FScissorLiftState(float InY, float InZ) : TranslationY(InY), TranslationZ(InZ) {}

	/**
	 * Create from normalized input (-1.0 to +1.0)
	 * @param NormalizedY - Forward/reverse (-1.0 = full reverse, +1.0 = full forward)
	 * @param NormalizedZ - Up/down (-1.0 = full down, +1.0 = full up)
	 * @param MaxTranslationY - Maximum Y translation in cm (hardware limit)
	 * @param MaxTranslationZ - Maximum Z translation in cm (hardware limit)
	 * @return ScissorLiftState struct with cm mapped from normalized input
	 */
	static FScissorLiftState FromNormalized(float NormalizedY, float NormalizedZ, float MaxTranslationY, float MaxTranslationZ)
	{
		FScissorLiftState State;
		State.TranslationY = FMath::Clamp(NormalizedY, -1.0f, 1.0f) * MaxTranslationY;
		State.TranslationZ = FMath::Clamp(NormalizedZ, -1.0f, 1.0f) * MaxTranslationZ;
		return State;
	}

	/**
	 * Convert to normalized values (-1.0 to +1.0)
	 * @param MaxTranslationY - Maximum Y translation in cm (hardware limit)
	 * @param MaxTranslationZ - Maximum Z translation in cm (hardware limit)
	 * @return Vector2D with normalized values (X = TranslationY, Y = TranslationZ)
	 */
	FVector2D ToNormalized(float MaxTranslationY, float MaxTranslationZ) const
	{
		FVector2D Normalized;
		Normalized.X = (MaxTranslationY > 0.0f) ? FMath::Clamp(TranslationY / MaxTranslationY, -1.0f, 1.0f) : 0.0f;
		Normalized.Y = (MaxTranslationZ > 0.0f) ? FMath::Clamp(TranslationZ / MaxTranslationZ, -1.0f, 1.0f) : 0.0f;
		return Normalized;
	}
};

