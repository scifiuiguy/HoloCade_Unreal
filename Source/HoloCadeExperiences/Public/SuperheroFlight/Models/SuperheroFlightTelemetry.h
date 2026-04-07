// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "SuperheroFlightTelemetry.generated.h"

/**
 * Superhero Flight System Telemetry
 * 
 * System health, temperatures, fault states, and redundancy status.
 * Sent from ECU to game engine on Channel 311 at 1 Hz (1000ms).
 */
USTRUCT(BlueprintType)
struct HOLOCADEEXPERIENCES_API FSuperheroFlightTelemetry
{
	GENERATED_BODY()

	/** Front winch motor temperature (°C) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperheroFlight|Telemetry")
	float FrontWinchMotorTemp = 0.0f;

	/** Rear winch motor temperature (°C) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperheroFlight|Telemetry")
	float RearWinchMotorTemp = 0.0f;

	/** Front winch fault detected */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperheroFlight|Telemetry")
	bool FrontWinchFault = false;

	/** Rear winch fault detected */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperheroFlight|Telemetry")
	bool RearWinchFault = false;

	/** Winch redundancy status (true = both winches operational, false = degraded mode) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperheroFlight|Telemetry")
	bool WinchRedundancyStatus = true;

	/** System voltage (V) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperheroFlight|Telemetry")
	float SystemVoltage = 0.0f;

	/** System current (A) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperheroFlight|Telemetry")
	float SystemCurrent = 0.0f;

	/** Timestamp when telemetry was captured (milliseconds since boot) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperheroFlight|Telemetry")
	int32 Timestamp = 0;

	FSuperheroFlightTelemetry()
	{
		FrontWinchMotorTemp = 0.0f;
		RearWinchMotorTemp = 0.0f;
		FrontWinchFault = false;
		RearWinchFault = false;
		WinchRedundancyStatus = true;
		SystemVoltage = 0.0f;
		SystemCurrent = 0.0f;
		Timestamp = 0;
	}
};

