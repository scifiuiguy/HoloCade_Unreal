// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HeadMountedDisplayTypes.h"
#include "SuperheroFlight/Models/SuperheroFlightGestureState.h"
#include "SuperheroFlight/Models/SuperheroFlightGameState.h"
#include "FlightHandsController.generated.h"

// Forward declarations
class APlayerController;
class UCameraComponent;
class IXRTrackingSystem;
class IHandTracker;

/**
 * Flight Hands Controller
 * 
 * Client-side component (runs on HMD) that converts 10-finger/arm gestures into control events.
 * Analyzes HMD position relative to hands to determine flight direction and speed.
 * 
 * Gesture Detection:
 * 1. Fist vs Open Hand - Both fists closed = flight motion, single hand release = hover/stop
 * 2. HMD-to-Hands Vector - Distance/worldspace-relative angle between HMD and hands center
 * 3. Flight Speed Throttle - Normalized distance between HMD and hands (attenuated by armLength)
 * 4. Virtual Altitude - Raycast from HMD to landable surfaces
 * 
 * Replication:
 * - Gesture events replicated to server via Unreal Replication
 * - NOOP: Multiplayer replication is mostly NOOP for initial pass (documented as NOOP)
 */
UCLASS(ClassGroup=(HoloCade), meta=(BlueprintSpawnableComponent))
class HOLOCADEEXPERIENCES_API UFlightHandsController : public UActorComponent
{
	GENERATED_BODY()

public:
	UFlightHandsController();
	virtual ~UFlightHandsController();

	/**
	 * Initialize gesture controller
	 * @param InPlayerController - Player controller for HMD access
	 * @return True if initialization successful
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|SuperheroFlight|Gesture")
	bool InitializeGestureController(APlayerController* InPlayerController);

	/**
	 * Get current gesture state
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|SuperheroFlight|Gesture")
	FSuperheroFlightGestureState GetGestureState() const { return CurrentGestureState; }

	/**
	 * Get current flight mode (determined by gesture analysis)
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|SuperheroFlight|Gesture")
	ESuperheroFlightGameState GetCurrentFlightMode() const { return CurrentGestureState.CurrentFlightMode; }

	// =====================================
	// Server-Side Parameters (Exposed in Command Console)
	// =====================================

	/** Angle threshold for transition from up to forward (degrees) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|SuperheroFlight|Gesture|Parameters")
	float UpToForwardAngle = 45.0f;

	/** Angle threshold for transition from forward to down (degrees) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|SuperheroFlight|Gesture|Parameters")
	float ForwardToDownAngle = 45.0f;

	/** Player arm length (inches, auto-calibrated from player height, manually adjustable) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|SuperheroFlight|Gesture|Parameters")
	float ArmLength = 28.0f;  // Default ~28 inches

	/** Virtual altitude raycast distance (inches) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|SuperheroFlight|Gesture|Parameters")
	float VirtualAltitudeRaycastDistance = 600.0f;  // 50 feet default

	/** 
	 * Only process gestures for locally controlled pawns (multiplayer safety)
	 * 
	 * When true (default): Only the local player's gestures are processed.
	 * When false: All players' gestures are processed (useful for debugging or experiences that need to track all players).
	 * 
	 * Note: In multiplayer, Unreal's XR system only provides hand tracking data for the local player.
	 * Setting this to false will still only process local player gestures, but won't skip remote pawns.
	 * This is primarily useful for single-player or when you want to explicitly allow processing on any pawn.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|SuperheroFlight|Multiplayer")
	bool bOnlyProcessLocalPlayer = true;

	/** Get HMD world position */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|SuperheroFlight|Gesture")
	FVector GetHMDPosition() const;

	/** Get left hand world position */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|SuperheroFlight|Gesture")
	FVector GetLeftHandPosition() const;

	/** Get right hand world position */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|SuperheroFlight|Gesture")
	FVector GetRightHandPosition() const;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	/** Player controller reference */
	UPROPERTY()
	TObjectPtr<APlayerController> PlayerController;

	/** Cached XR tracking system (for HMD and hand tracking) */
	mutable IXRTrackingSystem* XRSystem = nullptr;

	/** Cached hand tracker */
	mutable IHandTracker* HandTracker = nullptr;

	/** Current gesture state */
	FSuperheroFlightGestureState CurrentGestureState;

	/** Last gesture state (for detecting changes) */
	FSuperheroFlightGestureState LastGestureState;

	/** Update gesture state from HMD and hand tracking */
	void UpdateGestureState(float DeltaTime);

	/** Detect fist state (both hands closed vs single hand release) */
	void DetectFistState();

	/** Calculate gesture direction vector (HMD to hands center) */
	void CalculateGestureDirection();

	/** Calculate flight speed throttle (normalized by arm extension) */
	void CalculateFlightSpeedThrottle();

	/** Calculate virtual altitude (raycast to landable surfaces) */
	void CalculateVirtualAltitude();

	/** Determine flight mode from gesture state */
	void DetermineFlightMode();

	/** Check if hand is in fist state (uses Unreal's native hand tracking) */
	bool IsHandFistClosed(bool bLeftHand) const;

	/** Get XR tracking system */
	IXRTrackingSystem* GetXRSystem() const;

	/** Get hand tracker */
	IHandTracker* GetHandTracker() const;

	/** Get hand node transform from Unreal's native APIs */
	FTransform GetHandNodeTransform(bool bLeftHand, EHandKeypoint Keypoint) const;

	/** Check if this component should process gestures (only for locally controlled pawns) */
	bool ShouldProcessGestures() const;

	/** Raycast for landable surfaces (NOOP: will use collision system) */
	bool RaycastForLandableSurface(const FVector& Start, const FVector& Direction, float Distance, FVector& OutHitPoint) const;
};

