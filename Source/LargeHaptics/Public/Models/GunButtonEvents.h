// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "GunButtonEvents.generated.h"

/**
 * Gun button events (fast updates, sent on state change)
 * 
 * Data model for efficient struct-based UDP transmission of button states from all 4 gun stations.
 * Used by GunshipExperience for low-latency button event handling.
 * 
 * This is a Model (M) in MVC architecture - pure data structure.
 * Designed for UDP transport via HoloCade binary protocol (Channel 310).
 * 
 * Binary compatibility: Must match firmware struct exactly:
 * - bool Button0State[4] (4 bytes)
 * - bool Button1State[4] (4 bytes)
 * - unsigned long Timestamp (4 bytes, uint32)
 * Total: 12 bytes
 * 
 * Update rate: Configurable (default 20 Hz / 50ms)
 */
USTRUCT(BlueprintType)
struct LARGEHAPTICS_API FGunButtonEvents
{
	GENERATED_BODY()

	/** Left thumb button state per station (0-3) - Not Blueprint-exposed (arrays not supported) */
	bool Button0State[4];

	/** Right thumb button state per station (0-3) - Not Blueprint-exposed (arrays not supported) */
	bool Button1State[4];

	/** Timestamp when events occurred (milliseconds since boot) - Not Blueprint-exposed (uint32 not supported) */
	uint32 Timestamp = 0;

	FGunButtonEvents()
	{
		FMemory::Memset(Button0State, 0, sizeof(Button0State));
		FMemory::Memset(Button1State, 0, sizeof(Button1State));
		Timestamp = 0;
	}
};

