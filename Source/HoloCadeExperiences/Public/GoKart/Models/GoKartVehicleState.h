// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "GoKartThrottleState.h"
#include "GoKartVehicleState.generated.h"

/**
 * GoKart vehicle state
 *
 * Data model for position, velocity, track progress, and current item.
 * Used for UDP transmission to/from GoKart ECU and for game logic.
 */
USTRUCT(BlueprintType)
struct HOLOCADEEXPERIENCES_API FGoKartVehicleState
{
	GENERATED_BODY()

	/** Current world position of the kart */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GoKart|State")
	FVector Position = FVector::ZeroVector;

	/** Current velocity of the kart */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GoKart|State")
	FVector Velocity = FVector::ZeroVector;

	/** Progress along the track spline (0.0-1.0 or distance in cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GoKart|State")
	float TrackProgress = 0.0f;

	/** ID of the currently held item (0 if none) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GoKart|State")
	int32 CurrentItemID = 0;

	/** ECU connection status */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GoKart|State")
	bool bECUConnected = false;

	/** Shield active state */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GoKart|State")
	bool bShieldActive = false;

	/** Current throttle state */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GoKart|State")
	FGoKartThrottleState ThrottleState;

	/** Last time ECU update was received */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GoKart|State")
	float LastECUUpdateTime = 0.0f;

	/** Timestamp when state occurred (milliseconds since boot) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GoKart|State")
	int32 Timestamp = 0;

	FGoKartVehicleState()
	{
		Position = FVector::ZeroVector;
		Velocity = FVector::ZeroVector;
		TrackProgress = 0.0f;
		CurrentItemID = 0;
		bECUConnected = false;
		bShieldActive = false;
		LastECUUpdateTime = 0.0f;
		Timestamp = 0;
	}
};

