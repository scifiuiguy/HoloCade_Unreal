// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "GoKartThrottleState.generated.h"

/**
 * GoKart throttle state
 *
 * Data model for throttle input, boost/reduction, and final output.
 * Used for UDP transmission to/from GoKart ECU.
 */
USTRUCT(BlueprintType)
struct HOLOCADEEXPERIENCES_API FGoKartThrottleState
{
	GENERATED_BODY()

	/** Raw throttle input from pedal (0.0-1.0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GoKart|Throttle")
	float RawThrottleInput = 0.0f;

	/** Throttle boost/reduction multiplier (e.g., 1.2 for boost, 0.8 for reduction) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GoKart|Throttle")
	float ThrottleMultiplier = 1.0f;

	/** Final throttle output to motor (0.0-1.0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GoKart|Throttle")
	float FinalThrottleOutput = 0.0f;

	/** Timestamp when state occurred (milliseconds since boot) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GoKart|Throttle")
	int32 Timestamp = 0;

	FGoKartThrottleState()
	{
		RawThrottleInput = 0.0f;
		ThrottleMultiplier = 1.0f;
		FinalThrottleOutput = 0.0f;
		Timestamp = 0;
	}
};

