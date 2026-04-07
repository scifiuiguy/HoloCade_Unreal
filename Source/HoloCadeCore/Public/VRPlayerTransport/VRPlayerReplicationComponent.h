// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HeadMountedDisplayTypes.h"
#include "VRPlayerTransport/XRReplicatedData.h"
#include "VRPlayerReplicationComponent.generated.h"

// Forward declarations
class IXRTrackingSystem;
class IHandTracker;
class APawn;

/**
 * UHoloCadeVRPlayerReplicationComponent
 * 
 * Captures OpenXR HMD and hand tracking data from the local player and replicates it
 * to the server, which then replicates it to all clients. This enables remote players
 * to see other players' VR representations (head and hands) in real-time.
 * 
 * This component is experience-agnostic and works with all HoloCade experience templates.
 * 
 * Usage:
 * 1. Add this component to your VR player pawn
 * 2. The component automatically captures OpenXR data on the local client
 * 3. Data is replicated to server, then to all clients
 * 4. Other components (like HoloCadeHandGestureRecognizer) can query replicated data
 * 
 * Integration with HoloCadeHandGestureRecognizer:
 * - When bOnlyProcessLocalPlayer is false, the recognizer will use replicated data
 *   for remote players instead of OpenXR APIs (which only work for local player)
 */
UCLASS(ClassGroup=(HoloCade), meta=(BlueprintSpawnableComponent))
class HOLOCADECORE_API UHoloCadeVRPlayerReplicationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHoloCadeVRPlayerReplicationComponent();
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/**
	 * Get the replicated XR data for this player
	 * @return The current replicated XR data (HMD + hands)
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|VRReplication")
	const FHoloCadeXRReplicatedData& GetReplicatedXRData() const { return ReplicatedXRData; }

	/**
	 * Get HMD transform from replicated data
	 * @return HMD transform, or identity if not tracked
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|VRReplication")
	FTransform GetReplicatedHMDTransform() const;

	/**
	 * Get hand keypoint transform from replicated data
	 * @param bLeftHand - True for left hand, false for right hand
	 * @param Keypoint - The hand keypoint to retrieve
	 * @return Hand keypoint transform, or identity if not tracked
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|VRReplication")
	FTransform GetReplicatedHandKeypointTransform(bool bLeftHand, EHandKeypoint Keypoint) const;

	/**
	 * Check if hand tracking is active for a specific hand
	 * @param bLeftHand - True for left hand, false for right hand
	 * @return True if hand tracking is active
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|VRReplication")
	bool IsHandTrackingActive(bool bLeftHand) const;

	/**
	 * Check if this component is capturing data for the local player
	 * @return True if this is the local player's component
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|VRReplication")
	bool IsLocalPlayer() const;

	// ========================================
	// Configuration
	// ========================================

	/** Update rate for XR data capture and replication (Hz). Higher = smoother but more bandwidth. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|VRReplication|Config", meta = (ClampMin = "10.0", ClampMax = "120.0"))
	float ReplicationUpdateRate = 60.0f;

	/** Whether to enable XR data replication. Set to false to disable replication (e.g., single-player). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|VRReplication|Config")
	bool bEnableReplication = true;

protected:
	// ========================================
	// Replicated Properties
	// ========================================

	/** Replicated XR data (HMD + hand tracking) */
	UPROPERTY(ReplicatedUsing = OnRep_ReplicatedXRData, BlueprintReadOnly, Category = "HoloCade|VRReplication")
	FHoloCadeXRReplicatedData ReplicatedXRData;

	/** Replication callback when XR data is received from server */
	UFUNCTION()
	void OnRep_ReplicatedXRData();

	// ========================================
	// Internal State
	// ========================================

	/** Cached XR tracking system */
	mutable IXRTrackingSystem* XRSystem = nullptr;

	/** Cached hand tracker */
	mutable IHandTracker* HandTracker = nullptr;

	/** Internal timer for update rate control */
	float UpdateTimer = 0.0f;

	/** Whether this component is on the local player's pawn */
	bool bIsLocalPlayer = false;

	// ========================================
	// Internal Methods
	// ========================================

	/** Capture OpenXR data from local player and update ReplicatedXRData */
	void CaptureAndReplicateXRData();

	/** Get the XR tracking system */
	IXRTrackingSystem* GetXRSystem() const;

	/** Get the hand tracker */
	IHandTracker* GetHandTracker() const;

	/** Capture HMD transform from OpenXR */
	void CaptureHMDTransform(FHoloCadeXRReplicatedData& OutData);

	/** Capture hand tracking data from OpenXR */
	void CaptureHandTrackingData(FHoloCadeXRReplicatedData& OutData);

	/** Capture a specific hand keypoint */
	void CaptureHandKeypoint(EControllerHand Hand, EHandKeypoint Keypoint, FReplicatedHandData& OutHandData);
};

