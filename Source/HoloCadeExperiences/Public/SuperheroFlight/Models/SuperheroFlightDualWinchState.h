// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "SuperheroFlightGameState.h"
#include "SuperheroFlightDualWinchState.generated.h"

/**
 * Dual-Winch State Telemetry
 * 
 * Real-time state of both front and rear winches.
 * Sent from ECU to game engine on Channel 310 at 20 Hz (50ms).
 */
USTRUCT(BlueprintType)
struct HOLOCADEEXPERIENCES_API FSuperheroFlightDualWinchState
{
	GENERATED_BODY()

	/** Front winch position (inches, relative to standingGroundHeight) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperheroFlight|Winch")
	float FrontWinchPosition = 0.0f;

	/** Front winch speed (inches/second) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperheroFlight|Winch")
	float FrontWinchSpeed = 0.0f;

	/** Front winch tension (load cell reading in lbs or N) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperheroFlight|Winch")
	float FrontWinchTension = 0.0f;

	/** Rear winch position (inches, relative to standingGroundHeight) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperheroFlight|Winch")
	float RearWinchPosition = 0.0f;

	/** Rear winch speed (inches/second) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperheroFlight|Winch")
	float RearWinchSpeed = 0.0f;

	/** Rear winch tension (load cell reading in lbs or N) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperheroFlight|Winch")
	float RearWinchTension = 0.0f;

	/** Current game state (0=standing, 1=hovering, 2=flight-up, 3=flight-forward, 4=flight-down) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperheroFlight|State")
	int32 GameState = 0;

	/** Safety state (true = safe to operate, false = safety interlock active) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperheroFlight|State")
	bool SafetyState = true;

	/** Timestamp when state was captured (milliseconds since boot) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperheroFlight|State")
	int32 Timestamp = 0;

	FSuperheroFlightDualWinchState()
	{
		FrontWinchPosition = 0.0f;
		FrontWinchSpeed = 0.0f;
		FrontWinchTension = 0.0f;
		RearWinchPosition = 0.0f;
		RearWinchSpeed = 0.0f;
		RearWinchTension = 0.0f;
		GameState = 0;
		SafetyState = true;
		Timestamp = 0;
	}
};

