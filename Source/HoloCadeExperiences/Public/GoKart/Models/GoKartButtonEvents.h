// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "GoKartButtonEvents.generated.h"

/**
 * GoKart button events (fast updates, sent on state change)
 *
 * Data model for efficient struct-based UDP transmission of button states from GoKart ECU.
 *
 * This is a Model (M) in MVC architecture - pure data structure.
 * Designed for UDP transport via HoloCade binary protocol (Channel 310).
 *
 * Binary compatibility: Must match firmware struct exactly.
 */
USTRUCT(BlueprintType)
struct HOLOCADEEXPERIENCES_API FGoKartButtonEvents
{
	GENERATED_BODY()

	/** Horn button state */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GoKart|Button")
	bool HornButtonState = false;

	/** Horn LED state (reflects button press) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GoKart|Button")
	bool HornLEDState = false;

	/** Shield button state (long-press for shield function) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GoKart|Button")
	bool ShieldButtonState = false;

	/** Timestamp when events occurred (milliseconds since boot) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GoKart|Button")
	int32 Timestamp = 0; // Changed from uint32 to int32 for Blueprint compatibility

	FGoKartButtonEvents()
	{
		HornButtonState = false;
		HornLEDState = false;
		ShieldButtonState = false;
		Timestamp = 0;
	}
};

