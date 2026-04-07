// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "SuperheroFlightGameState.h"
#include "SuperheroFlightGestureState.generated.h"

/**
 * Gesture State
 * 
 * Current gesture state from FlightHandsController (client-side).
 * Replicated to server for winch control.
 */
USTRUCT(BlueprintType)
struct HOLOCADEEXPERIENCES_API FSuperheroFlightGestureState
{
	GENERATED_BODY()

	/** Both fists closed (true = flight motion enabled, false = hover/stop) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperheroFlight|Gesture")
	bool bBothFistsClosed = false;

	/** Left fist closed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperheroFlight|Gesture")
	bool bLeftFistClosed = false;

	/** Right fist closed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperheroFlight|Gesture")
	bool bRightFistClosed = false;

	/** Gesture direction vector (normalized, from HMD to hands center) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperheroFlight|Gesture")
	FVector GestureDirection = FVector::UpVector;

	/** Angle relative to world ground plane (degrees, 0=up, 90=forward, 180=down) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperheroFlight|Gesture")
	float GestureAngle = 0.0f;

	/** Flight speed throttle (0.0-1.0, normalized by arm extension) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperheroFlight|Gesture")
	float FlightSpeedThrottle = 0.0f;

	/** Virtual altitude (distance to landable surface, inches) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperheroFlight|Gesture")
	float VirtualAltitude = 0.0f;

	/** Current flight mode (determined by gesture analysis) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperheroFlight|Gesture")
	ESuperheroFlightGameState CurrentFlightMode = ESuperheroFlightGameState::Standing;

	FSuperheroFlightGestureState()
	{
		bBothFistsClosed = false;
		bLeftFistClosed = false;
		bRightFistClosed = false;
		GestureDirection = FVector::UpVector;
		GestureAngle = 0.0f;
		FlightSpeedThrottle = 0.0f;
		VirtualAltitude = 0.0f;
		CurrentFlightMode = ESuperheroFlightGameState::Standing;
	}
};

