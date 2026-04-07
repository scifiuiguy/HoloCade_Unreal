// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "GunTelemetry.generated.h"

/**
 * Gun telemetry (slow updates, sent periodically)
 * 
 * Data model for efficient struct-based UDP transmission of telemetry from all 4 gun stations.
 * Used by GunshipExperience for monitoring gun system health, temperatures, and firing state.
 * 
 * This is a Model (M) in MVC architecture - pure data structure.
 * Designed for UDP transport via HoloCade binary protocol (Channel 311).
 * 
 * Binary compatibility: Must match firmware struct exactly:
 * - float ActiveSolenoidTemp[4] (16 bytes)
 * - float DriverModuleTemp[4] (16 bytes)
 * - uint8 ActiveSolenoidID[4] (4 bytes)
 * - uint8 NumSolenoids[4] (4 bytes)
 * - bool ThermalShutdown[4] (4 bytes)
 * - float PWMThrottle[4] (16 bytes)
 * - bool FireCommandActive[4] (4 bytes)
 * - float FireIntensity[4] (16 bytes)
 * - uint32 FireDuration[4] (16 bytes)
 * - bool PlaySessionActive (1 byte, may be padded to 4)
 * - bool CanFire[4] (4 bytes)
 * - bool StationConnected[4] (4 bytes)
 * - uint32 Timestamp (4 bytes)
 * Total: ~113 bytes (with padding)
 * 
 * Update rate: Configurable (default 1 Hz / 1000ms)
 */
USTRUCT(BlueprintType)
struct LARGEHAPTICS_API FGunTelemetry
{
	GENERATED_BODY()

	/** Temperature of active solenoid per station (°C) - Not Blueprint-exposed (arrays not supported) */
	float ActiveSolenoidTemp[4];

	/** PWM driver module temperature per station (°C) - Not Blueprint-exposed (arrays not supported) */
	float DriverModuleTemp[4];

	/** Currently active solenoid ID per station (0 to N-1) - Not Blueprint-exposed (arrays not supported) */
	uint8 ActiveSolenoidID[4];

	/** Total number of solenoids per station (N) - Not Blueprint-exposed (arrays not supported) */
	uint8 NumSolenoids[4];

	/** Thermal shutdown active per station - Not Blueprint-exposed (bool arrays not supported) */
	bool ThermalShutdown[4];

	/** Current PWM throttle factor per station (0.5-1.0) - Not Blueprint-exposed (arrays not supported) */
	float PWMThrottle[4];

	/** Currently firing per station - Not Blueprint-exposed (bool arrays not supported) */
	bool FireCommandActive[4];

	/** Current fire intensity per station (0.0-1.0) - Not Blueprint-exposed (arrays not supported) */
	float FireIntensity[4];

	/** Fire pulse duration per station (milliseconds) - Not Blueprint-exposed (uint32 arrays not supported) */
	uint32 FireDuration[4];

	/** Play session authorization (same for all stations) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Gunship|Guns|System")
	bool PlaySessionActive = false;

	/** Computed: Can fire per station (PlaySessionActive && !ThermalShutdown) - Not Blueprint-exposed (bool arrays not supported) */
	bool CanFire[4];

	/** Station is sending telemetry (not timed out) - Not Blueprint-exposed (bool arrays not supported) */
	bool StationConnected[4];

	/** Timestamp when telemetry was collected (milliseconds since boot) - Not Blueprint-exposed (uint32 not supported) */
	uint32 Timestamp = 0;

	FGunTelemetry()
	{
		FMemory::Memset(ActiveSolenoidTemp, 0, sizeof(ActiveSolenoidTemp));
		FMemory::Memset(DriverModuleTemp, 0, sizeof(DriverModuleTemp));
		FMemory::Memset(ActiveSolenoidID, 0, sizeof(ActiveSolenoidID));
		FMemory::Memset(NumSolenoids, 0, sizeof(NumSolenoids));
		FMemory::Memset(ThermalShutdown, 0, sizeof(ThermalShutdown));
		FMemory::Memset(PWMThrottle, 0, sizeof(PWMThrottle));
		FMemory::Memset(FireCommandActive, 0, sizeof(FireCommandActive));
		FMemory::Memset(FireIntensity, 0, sizeof(FireIntensity));
		FMemory::Memset(FireDuration, 0, sizeof(FireDuration));
		PlaySessionActive = false;
		FMemory::Memset(CanFire, 0, sizeof(CanFire));
		FMemory::Memset(StationConnected, 0, sizeof(StationConnected));
		Timestamp = 0;
	}
};

