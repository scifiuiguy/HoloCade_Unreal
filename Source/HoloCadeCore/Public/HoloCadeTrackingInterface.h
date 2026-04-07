// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "HoloCadeTrackingInterface.generated.h"

/**
 * Enumeration of supported tracking systems in HoloCade
 */
UENUM(BlueprintType)
enum class EHoloCadeTrackingSystem : uint8
{
	SteamVRTrackers UMETA(DisplayName = "SteamVR Trackers"),
	CustomOptical UMETA(DisplayName = "Custom Optical"),
	CustomUWB UMETA(DisplayName = "Custom UWB"),
	CustomUltrasonic UMETA(DisplayName = "Custom Ultrasonic"),
	None UMETA(DisplayName = "None")
};

/**
 * Configuration for HoloCade tracking system
 */
USTRUCT(BlueprintType)
struct FHoloCadeTrackingConfig
{
	GENERATED_BODY()

	/** Which tracking system to use */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Tracking")
	EHoloCadeTrackingSystem TrackingSystem = EHoloCadeTrackingSystem::SteamVRTrackers;

	/** Number of tracking devices to expect */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Tracking")
	int32 ExpectedDeviceCount = 0;
};

// This class does not need to be modified.
UINTERFACE(MinimalAPI, BlueprintType)
class UHoloCadeTrackingInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface for HoloCade 6DOF tracking abstraction
 * 
 * This interface provides a unified API for working with different 6DOF tracking systems.
 * Default implementation uses SteamVR trackers. Future versions can add custom tracking solutions.
 */
class HOLOCADECORE_API IHoloCadeTrackingInterface
{
	GENERATED_BODY()

public:
	/**
	 * Initialize the tracking system
	 * @param Config - Configuration settings for tracking
	 * @return true if initialization was successful
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "HoloCade|Tracking")
	bool InitializeTracking(const FHoloCadeTrackingConfig& Config);

	/**
	 * Get the transform of a tracked device by index
	 * @param DeviceIndex - Index of the tracking device
	 * @param OutTransform - The world-space transform of the device
	 * @return true if the device was found and is tracking
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "HoloCade|Tracking")
	bool GetTrackedDeviceTransform(int32 DeviceIndex, FTransform& OutTransform) const;

	/**
	 * Get the number of currently tracked devices
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "HoloCade|Tracking")
	int32 GetTrackedDeviceCount() const;

	/**
	 * Check if a specific device is currently tracking
	 * @param DeviceIndex - Index of the tracking device to check
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "HoloCade|Tracking")
	bool IsDeviceTracking(int32 DeviceIndex) const;
};




