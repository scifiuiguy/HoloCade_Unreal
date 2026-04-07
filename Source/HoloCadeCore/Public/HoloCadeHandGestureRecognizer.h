// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HeadMountedDisplayTypes.h"
#include "HoloCadeHandGestureRecognizer.generated.h"

// Forward declarations
class IXRTrackingSystem;
class IHandTracker;
class APlayerController;
class UHoloCadeVRPlayerReplicationComponent;

/**
 * Hand gesture types that can be recognized
 */
UENUM(BlueprintType)
enum class EHoloCadeHandGesture : uint8
{
	None UMETA(DisplayName = "None"),
	FistClosed UMETA(DisplayName = "Fist Closed"),
	HandOpen UMETA(DisplayName = "Hand Open"),
	Pointing UMETA(DisplayName = "Pointing"),
	ThumbsUp UMETA(DisplayName = "Thumbs Up"),
	PeaceSign UMETA(DisplayName = "Peace Sign")
};

/**
 * Delegate fired when a gesture is detected
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnHandGestureDetected, bool, bLeftHand, EHoloCadeHandGesture, Gesture, float, Confidence);

/**
 * UHoloCadeHandGestureRecognizer
 * 
 * Recognizes hand gestures using Unreal's native hand tracking APIs (IHandTracker, IXRTrackingSystem).
 * Maps gestures to delegates for easy integration with experience templates.
 * 
 * Uses Unreal's native OpenXR hand tracking - no wrapper components needed.
 */
UCLASS(ClassGroup=(HoloCade), meta=(BlueprintSpawnableComponent))
class HOLOCADECORE_API UHoloCadeHandGestureRecognizer : public UActorComponent
{
	GENERATED_BODY()

public:
	UHoloCadeHandGestureRecognizer();
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/**
	 * Initialize gesture recognizer
	 * @param InPlayerController - Player controller for multiplayer context
	 * @return True if initialization successful
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|HandGesture")
	bool InitializeRecognizer(APlayerController* InPlayerController);

	/**
	 * Check if a specific hand is in fist state
	 * @param bLeftHand - True for left hand, false for right hand
	 * @return True if hand is closed (fist)
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|HandGesture")
	bool IsHandFistClosed(bool bLeftHand) const;

	/**
	 * Get wrist position for a hand
	 * @param bLeftHand - True for left hand, false for right hand
	 * @return World-space position of wrist, or zero vector if not tracking
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|HandGesture")
	FVector GetWristPosition(bool bLeftHand) const;

	/**
	 * Get hand center position (middle knuckle/MCP joint)
	 * @param bLeftHand - True for left hand, false for right hand
	 * @return World-space position of hand center, or zero vector if not tracking
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|HandGesture")
	FVector GetHandCenterPosition(bool bLeftHand) const;

	/**
	 * Get all fingertip positions
	 * @param bLeftHand - True for left hand, false for right hand
	 * @param OutPositions - Array to populate with fingertip positions (5 elements: thumb, index, middle, ring, pinky)
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|HandGesture")
	void GetFingertipPositions(bool bLeftHand, TArray<FVector>& OutPositions) const;

	/**
	 * Get current detected gesture for a hand
	 * @param bLeftHand - True for left hand, false for right hand
	 * @return Currently detected gesture
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|HandGesture")
	EHoloCadeHandGesture GetCurrentGesture(bool bLeftHand) const;

	/**
	 * Check if hand tracking is currently active
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|HandGesture")
	bool IsHandTrackingActive() const;

	/**
	 * Check if this component is processing gestures for the local player
	 * In multiplayer, only locally controlled pawns should process gestures
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|HandGesture")
	bool IsProcessingForLocalPlayer() const;

	/** Delegate fired when a gesture is detected */
	UPROPERTY(BlueprintAssignable, Category = "HoloCade|HandGesture")
	FOnHandGestureDetected OnHandGestureDetected;

	// ========================================
	// Configuration Parameters
	// ========================================

	/** 
	 * Only process gestures for locally controlled pawns (multiplayer safety)
	 * 
	 * When true (default): Only the local player's gestures are processed using OpenXR APIs.
	 * When false: All players' gestures are processed. For local players, uses OpenXR APIs. 
	 *             For remote players, uses replicated data from UHoloCadeVRPlayerReplicationComponent.
	 * 
	 * **VR Player Transport Integration**: When this is false and UHoloCadeVRPlayerReplicationComponent is present,
	 * the recognizer will automatically use replicated hand tracking data for remote players, enabling gesture
	 * recognition for all players in multiplayer experiences.
	 * 
	 * This is useful for experiences that need to track gestures from all players (e.g., collaborative experiences,
	 * gesture-based interactions between players, or experiences that need to detect gestures from live actors).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|HandGesture|Multiplayer")
	bool bOnlyProcessLocalPlayer = true;

	/** Fist detection threshold - fingertips must be within this distance (inches) of hand center */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|HandGesture|Parameters")
	float FistDetectionThreshold = 2.0f;  // 2 inches (~5cm)

	/** Minimum number of fingertips that must be close to center for fist detection (out of 5) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|HandGesture|Parameters", meta = (ClampMin = "1", ClampMax = "5"))
	int32 MinFingersClosedForFist = 4;

	/** Update rate for gesture recognition (Hz) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|HandGesture|Parameters", meta = (ClampMin = "1.0", ClampMax = "120.0"))
	float UpdateRate = 60.0f;

protected:
	/** Cached XR tracking system */
	mutable IXRTrackingSystem* XRSystem = nullptr;

	/** Cached hand tracker */
	mutable IHandTracker* HandTracker = nullptr;

	/** Cached player controller (for multiplayer context) */
	UPROPERTY()
	TObjectPtr<APlayerController> CachedPlayerController;

	/** Current detected gestures */
	EHoloCadeHandGesture LeftHandGesture = EHoloCadeHandGesture::None;
	EHoloCadeHandGesture RightHandGesture = EHoloCadeHandGesture::None;

	/** Internal timer for update rate control */
	float UpdateTimer = 0.0f;

	/** Get the XR tracking system */
	IXRTrackingSystem* GetXRSystem() const;

	/** Get the hand tracker */
	IHandTracker* GetHandTracker() const;

	/** Get hand node transform from Unreal's native APIs or replicated data */
	FTransform GetHandNodeTransform(bool bLeftHand, EHandKeypoint Keypoint) const;

	/** Get the VR replication component from the owner (if available) */
	UHoloCadeVRPlayerReplicationComponent* GetVRReplicationComponent() const;

	/** Update gesture recognition */
	void UpdateGestureRecognition(float DeltaTime);

	/** Detect gesture for a specific hand */
	EHoloCadeHandGesture DetectGesture(bool bLeftHand) const;

	/** Check if this component should process gestures (only for locally controlled pawns) */
	bool ShouldProcessGestures() const;
};

