// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SuperheroFlight/Models/SuperheroFlightGestureState.h"
#include "SuperheroFlight/Models/SuperheroFlightGameState.h"
#include "GestureDebugger.generated.h"

// Forward declarations
class UFlightHandsController;

/**
 * Gesture Debugger
 * 
 * HMD HUD component that provides visualization for Ops Tech:
 * - Hand positions (debug rays)
 * - Normalized center point between hands
 * - Gesture direction vectors
 * - Transition angle thresholds (upToForwardAngle, forwardToDownAngle)
 * - Current flight mode
 * - Arm extension percentage
 * - Virtual altitude raycast visualization
 * 
 * Helps Ops Tech calibrate gesture sensitivity and verify player control.
 */
UCLASS(ClassGroup=(HoloCade), meta=(BlueprintSpawnableComponent))
class HOLOCADEEXPERIENCES_API UGestureDebugger : public UActorComponent
{
	GENERATED_BODY()

public:
	UGestureDebugger();
	virtual ~UGestureDebugger();

	/**
	 * Initialize gesture debugger
	 * @param InFlightHandsController - Flight hands controller to debug
	 * @return True if initialization successful
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|SuperheroFlight|Debug")
	bool InitializeDebugger(UFlightHandsController* InFlightHandsController);

	/**
	 * Enable/disable debug visualization
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|SuperheroFlight|Debug")
	void SetDebugEnabled(bool bEnabled);

	/**
	 * Check if debug visualization is enabled
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|SuperheroFlight|Debug")
	bool IsDebugEnabled() const { return bDebugEnabled; }

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	/** Flight hands controller reference */
	UPROPERTY()
	TObjectPtr<UFlightHandsController> FlightHandsController;

	/** Whether debug visualization is enabled */
	bool bDebugEnabled = false;

	/** Draw debug visualization */
	void DrawDebugVisualization();

	/** Draw hand positions and rays */
	void DrawHandPositions();

	/** Draw gesture direction vectors */
	void DrawGestureVectors();

	/** Draw angle thresholds */
	void DrawAngleThresholds();

	/** Draw virtual altitude raycast */
	void DrawVirtualAltitudeRaycast();

	/** Draw HUD text information */
	void DrawHUDText();
};

