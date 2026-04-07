// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "SuperheroFlightGameState.generated.h"

/**
 * Superhero Flight Experience Game States
 * 
 * Five distinct flight modes for the dual-winch suspended harness system.
 */
UENUM(BlueprintType)
enum class ESuperheroFlightGameState : uint8
{
	/** Standing Mode - Player upright, feet on ground */
	Standing UMETA(DisplayName = "Standing"),
	
	/** Hovering Mode - Player lifted to airHeight, upright position */
	Hovering UMETA(DisplayName = "Hovering"),
	
	/** Flight-Up Mode - Player lifted to airHeight, upright, arms pointing up */
	FlightUp UMETA(DisplayName = "Flight Up"),
	
	/** Flight-Forward Mode - Player lifted to proneHeight, prone position, arms pointing forward */
	FlightForward UMETA(DisplayName = "Flight Forward"),
	
	/** Flight-Down Mode - Player lifted to airHeight, upright, arms pointing down */
	FlightDown UMETA(DisplayName = "Flight Down")
};

