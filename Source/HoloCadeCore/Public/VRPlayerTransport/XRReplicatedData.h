// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "HeadMountedDisplayTypes.h"
#include "XRReplicatedData.generated.h"

/**
 * Replicated hand keypoint transform data
 * 
 * Stores position, rotation, and tracking state for a single hand keypoint.
 * Used for efficient network replication of OpenXR hand tracking data.
 */
USTRUCT(BlueprintType)
struct HOLOCADECORE_API FReplicatedHandKeypoint
{
	GENERATED_BODY()

	/** World-space position of the keypoint */
	UPROPERTY(BlueprintReadOnly)
	FVector Position = FVector::ZeroVector;

	/** World-space rotation of the keypoint */
	UPROPERTY(BlueprintReadOnly)
	FRotator Rotation = FRotator::ZeroRotator;

	/** Whether this keypoint is currently being tracked */
	UPROPERTY(BlueprintReadOnly)
	bool bIsTracked = false;

	/** Radius of the keypoint (for collision/sphere representation) */
	UPROPERTY(BlueprintReadOnly)
	float Radius = 0.0f;

	FReplicatedHandKeypoint()
		: Position(FVector::ZeroVector)
		, Rotation(FRotator::ZeroRotator)
		, bIsTracked(false)
		, Radius(0.0f)
	{
	}

	FReplicatedHandKeypoint(const FVector& InPosition, const FRotator& InRotation, bool bInIsTracked, float InRadius)
		: Position(InPosition)
		, Rotation(InRotation)
		, bIsTracked(bInIsTracked)
		, Radius(InRadius)
	{
	}

	/** Convert to FTransform */
	FTransform ToTransform() const
	{
		return FTransform(Rotation, Position);
	}
};

/**
 * Replicated data for a single hand (left or right)
 * 
 * Stores all hand keypoint transforms for efficient replication.
 * Unreal's EHandKeypoint enum has ~26 keypoints per hand, but we replicate
 * the most commonly used ones for gesture recognition and future extensibility.
 */
USTRUCT(BlueprintType)
struct HOLOCADECORE_API FReplicatedHandData
{
	GENERATED_BODY()

	/** Wrist transform */
	UPROPERTY(BlueprintReadOnly)
	FReplicatedHandKeypoint Wrist;

	/** Hand center (middle metacarpal/MCP joint) */
	UPROPERTY(BlueprintReadOnly)
	FReplicatedHandKeypoint HandCenter;

	/** Thumb tip */
	UPROPERTY(BlueprintReadOnly)
	FReplicatedHandKeypoint ThumbTip;

	/** Index finger tip */
	UPROPERTY(BlueprintReadOnly)
	FReplicatedHandKeypoint IndexTip;

	/** Middle finger tip */
	UPROPERTY(BlueprintReadOnly)
	FReplicatedHandKeypoint MiddleTip;

	/** Ring finger tip */
	UPROPERTY(BlueprintReadOnly)
	FReplicatedHandKeypoint RingTip;

	/** Little (pinky) finger tip */
	UPROPERTY(BlueprintReadOnly)
	FReplicatedHandKeypoint LittleTip;

	/** Whether hand tracking is active for this hand */
	UPROPERTY(BlueprintReadOnly)
	bool bIsHandTrackingActive = false;

	FReplicatedHandData()
		: bIsHandTrackingActive(false)
	{
	}

	/**
	 * Get a specific keypoint by enum value
	 * @param Keypoint - The hand keypoint to retrieve
	 * @return The replicated keypoint data, or default if not found
	 */
	const FReplicatedHandKeypoint* GetKeypoint(EHandKeypoint Keypoint) const;

	/**
	 * Set a specific keypoint by enum value
	 * @param Keypoint - The hand keypoint to set
	 * @param KeypointData - The data to set
	 */
	void SetKeypoint(EHandKeypoint Keypoint, const FReplicatedHandKeypoint& KeypointData);
};

/**
 * Complete XR replicated data for a VR player
 * 
 * Contains HMD transform and both hand tracking data.
 * This structure is replicated from client to server, then from server to all clients.
 */
USTRUCT(BlueprintType)
struct HOLOCADECORE_API FHoloCadeXRReplicatedData
{
	GENERATED_BODY()

	/** HMD world-space position */
	UPROPERTY(BlueprintReadOnly)
	FVector HMDPosition = FVector::ZeroVector;

	/** HMD world-space rotation */
	UPROPERTY(BlueprintReadOnly)
	FRotator HMDRotation = FRotator::ZeroRotator;

	/** Whether HMD tracking is active */
	UPROPERTY(BlueprintReadOnly)
	bool bIsHMDTracked = false;

	/** Left hand tracking data */
	UPROPERTY(BlueprintReadOnly)
	FReplicatedHandData LeftHand;

	/** Right hand tracking data */
	UPROPERTY(BlueprintReadOnly)
	FReplicatedHandData RightHand;

	/** Timestamp when this data was captured (server time) */
	UPROPERTY(BlueprintReadOnly)
	float ServerTimeStamp = 0.0f;

	FHoloCadeXRReplicatedData()
		: HMDPosition(FVector::ZeroVector)
		, HMDRotation(FRotator::ZeroRotator)
		, bIsHMDTracked(false)
		, ServerTimeStamp(0.0f)
	{
	}

	/** Get HMD transform */
	FTransform GetHMDTransform() const
	{
		return FTransform(HMDRotation, HMDPosition);
	}
};

