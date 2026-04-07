// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "HoloCadeHandGestureRecognizer.h"
#include "VRPlayerTransport/VRPlayerReplicationComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "IXRTrackingSystem.h"
#include "IHandTracker.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Actor.h"
#include "HeadMountedDisplayTypes.h"
#include "Features/IModularFeatures.h"

UHoloCadeHandGestureRecognizer::UHoloCadeHandGestureRecognizer()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	XRSystem = nullptr;
	HandTracker = nullptr;
	CachedPlayerController = nullptr;
	UpdateTimer = 0.0f;
	bOnlyProcessLocalPlayer = true;  // Default: Only process local player (multiplayer safety)
	FistDetectionThreshold = 2.0f;
	MinFingersClosedForFist = 4;
	UpdateRate = 60.0f;
	LeftHandGesture = EHoloCadeHandGesture::None;
	RightHandGesture = EHoloCadeHandGesture::None;
}

void UHoloCadeHandGestureRecognizer::BeginPlay()
{
	Super::BeginPlay();
	// Auto-initialize if we have a player controller
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		InitializeRecognizer(PC);
	}
}

void UHoloCadeHandGestureRecognizer::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateTimer += DeltaTime;
	float UpdateInterval = 1.0f / UpdateRate;

	if (UpdateTimer >= UpdateInterval)
	{
		UpdateGestureRecognition(UpdateTimer);
		UpdateTimer = 0.0f;
	}
}

bool UHoloCadeHandGestureRecognizer::InitializeRecognizer(APlayerController* InPlayerController)
{
	CachedPlayerController = InPlayerController;

	// Get XR system
	XRSystem = GetXRSystem();
	if (!XRSystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("HoloCadeHandGestureRecognizer: XR system not available"));
		return false;
	}

	// Get hand tracker
	HandTracker = GetHandTracker();
	if (!HandTracker)
	{
		UE_LOG(LogTemp, Warning, TEXT("HoloCadeHandGestureRecognizer: Hand tracker not available"));
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("HoloCadeHandGestureRecognizer: Initialized"));
	return true;
}

bool UHoloCadeHandGestureRecognizer::IsHandFistClosed(bool bLeftHand) const
{
	EControllerHand ControllerHand = bLeftHand ? EControllerHand::Left : EControllerHand::Right;
	IHandTracker* Tracker = GetHandTracker();
	if (!Tracker)
	{
		return false;
	}

	// Get hand center (middle knuckle/MCP)
	FTransform HandCenterTransform = GetHandNodeTransform(bLeftHand, EHandKeypoint::MiddleMetacarpal);
	if (HandCenterTransform.Equals(FTransform::Identity))
	{
		return false; // Hand not tracking
	}

	FVector HandCenter = HandCenterTransform.GetLocation();

	// Get all fingertip positions
	TArray<EHandKeypoint> FingertipKeypoints = {
		EHandKeypoint::ThumbTip,
		EHandKeypoint::IndexTip,
		EHandKeypoint::MiddleTip,
		EHandKeypoint::RingTip,
		EHandKeypoint::LittleTip
	};

	int32 FingersClosed = 0;
	for (EHandKeypoint Keypoint : FingertipKeypoints)
	{
		FTransform TipTransform = GetHandNodeTransform(bLeftHand, Keypoint);
		if (TipTransform.Equals(FTransform::Identity))
		{
			continue; // Tip not tracking
		}

		FVector TipPos = TipTransform.GetLocation();
		float DistanceToCenter = FVector::Dist(TipPos, HandCenter);
		
		if (DistanceToCenter < FistDetectionThreshold)
		{
			FingersClosed++;
		}
	}

	return FingersClosed >= MinFingersClosedForFist;
}

FVector UHoloCadeHandGestureRecognizer::GetWristPosition(bool bLeftHand) const
{
	FTransform WristTransform = GetHandNodeTransform(bLeftHand, EHandKeypoint::Wrist);
	return WristTransform.GetLocation();
}

FVector UHoloCadeHandGestureRecognizer::GetHandCenterPosition(bool bLeftHand) const
{
	FTransform HandCenterTransform = GetHandNodeTransform(bLeftHand, EHandKeypoint::MiddleMetacarpal);
	return HandCenterTransform.GetLocation();
}

void UHoloCadeHandGestureRecognizer::GetFingertipPositions(bool bLeftHand, TArray<FVector>& OutPositions) const
{
	OutPositions.Empty(5);
	
	TArray<EHandKeypoint> FingertipKeypoints = {
		EHandKeypoint::ThumbTip,
		EHandKeypoint::IndexTip,
		EHandKeypoint::MiddleTip,
		EHandKeypoint::RingTip,
		EHandKeypoint::LittleTip
	};

	for (EHandKeypoint Keypoint : FingertipKeypoints)
	{
		FTransform TipTransform = GetHandNodeTransform(bLeftHand, Keypoint);
		OutPositions.Add(TipTransform.GetLocation());
	}
}

EHoloCadeHandGesture UHoloCadeHandGestureRecognizer::GetCurrentGesture(bool bLeftHand) const
{
	return bLeftHand ? LeftHandGesture : RightHandGesture;
}

bool UHoloCadeHandGestureRecognizer::IsHandTrackingActive() const
{
	return GetHandTracker() != nullptr;
}

bool UHoloCadeHandGestureRecognizer::IsProcessingForLocalPlayer() const
{
	return ShouldProcessGestures();
}

IXRTrackingSystem* UHoloCadeHandGestureRecognizer::GetXRSystem() const
{
	if (!XRSystem && GEngine && GEngine->XRSystem.IsValid())
	{
		XRSystem = GEngine->XRSystem.Get();
	}
	return XRSystem;
}

IHandTracker* UHoloCadeHandGestureRecognizer::GetHandTracker() const
{
	if (!HandTracker)
	{
		// Access IHandTracker via IModularFeatures (not via IXRTrackingSystem)
		IModularFeatures& ModularFeatures = IModularFeatures::Get();
		FName HandTrackerFeatureName = IHandTracker::GetModularFeatureName();
		if (ModularFeatures.IsModularFeatureAvailable(HandTrackerFeatureName))
		{
			HandTracker = static_cast<IHandTracker*>(ModularFeatures.GetModularFeatureImplementation(HandTrackerFeatureName, 0));
		}
	}
	return HandTracker;
}

FTransform UHoloCadeHandGestureRecognizer::GetHandNodeTransform(bool bLeftHand, EHandKeypoint Keypoint) const
{
	// Check if we should use replicated data for remote players
	// When bOnlyProcessLocalPlayer is false, we can process gestures for remote players using replicated data
	if (!bOnlyProcessLocalPlayer || !ShouldProcessGestures())
	{
		// Try to get replicated data from VR replication component
		if (UHoloCadeVRPlayerReplicationComponent* ReplicationComp = GetVRReplicationComponent())
		{
			// If this is not the local player, use replicated data
			if (!ReplicationComp->IsLocalPlayer())
			{
				FTransform ReplicatedTransform = ReplicationComp->GetReplicatedHandKeypointTransform(bLeftHand, Keypoint);
				if (!ReplicatedTransform.Equals(FTransform::Identity))
				{
					return ReplicatedTransform;
				}
			}
		}
	}
	
	// For local player or when replication component is not available, use OpenXR APIs
	IHandTracker* Tracker = GetHandTracker();
	if (!Tracker)
	{
		return FTransform::Identity;
	}

	EControllerHand ControllerHand = bLeftHand ? EControllerHand::Left : EControllerHand::Right;
	FTransform OutTransform;
	float OutRadius;
	
	if (Tracker->GetKeypointState(ControllerHand, Keypoint, OutTransform, OutRadius))
	{
		return OutTransform;
	}

	return FTransform::Identity;
}

UHoloCadeVRPlayerReplicationComponent* UHoloCadeHandGestureRecognizer::GetVRReplicationComponent() const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	// Try to get the component from the owner
	return Owner->FindComponentByClass<UHoloCadeVRPlayerReplicationComponent>();
}

void UHoloCadeHandGestureRecognizer::UpdateGestureRecognition(float DeltaTime)
{
	// Check if we should process gestures
	// When bOnlyProcessLocalPlayer is true, only process for local player
	// When false, process for all players (using replicated data for remote players)
	if (bOnlyProcessLocalPlayer && !ShouldProcessGestures())
	{
		return;
	}

	// Check if we have tracking data available
	// For local player: check OpenXR APIs
	// For remote player: check replicated data
	bool bHasTrackingData = false;
	if (ShouldProcessGestures())
	{
		// Local player - check OpenXR
		bHasTrackingData = IsHandTrackingActive();
	}
	else
	{
		// Remote player - check replicated data
		if (UHoloCadeVRPlayerReplicationComponent* ReplicationComp = GetVRReplicationComponent())
		{
			bHasTrackingData = ReplicationComp->IsHandTrackingActive(true) || ReplicationComp->IsHandTrackingActive(false);
		}
	}

	if (!bHasTrackingData)
	{
		return;
	}

	// Detect gestures for both hands
	EHoloCadeHandGesture NewLeftGesture = DetectGesture(true);
	EHoloCadeHandGesture NewRightGesture = DetectGesture(false);

	// Fire delegates if gestures changed
	if (NewLeftGesture != LeftHandGesture)
	{
		OnHandGestureDetected.Broadcast(true, NewLeftGesture, 1.0f);
		LeftHandGesture = NewLeftGesture;
	}

	if (NewRightGesture != RightHandGesture)
	{
		OnHandGestureDetected.Broadcast(false, NewRightGesture, 1.0f);
		RightHandGesture = NewRightGesture;
	}
}

EHoloCadeHandGesture UHoloCadeHandGestureRecognizer::DetectGesture(bool bLeftHand) const
{
	// For now, just detect fist vs open hand
	// Future: Add more gesture recognition (pointing, thumbs up, peace sign, etc.)
	
	if (IsHandFistClosed(bLeftHand))
	{
		return EHoloCadeHandGesture::FistClosed;
	}
	
	return EHoloCadeHandGesture::HandOpen;
}

bool UHoloCadeHandGestureRecognizer::ShouldProcessGestures() const
{
	// If configured to process all players, skip the local-only check
	if (!bOnlyProcessLocalPlayer)
	{
		return true;
	}

	// Get the owner pawn
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return false;
	}

	// Check if owner is a pawn
	APawn* OwnerPawn = Cast<APawn>(Owner);
	if (!OwnerPawn)
	{
		// If not a pawn, check if we have a player controller reference
		if (CachedPlayerController)
		{
			return CachedPlayerController->IsLocalController();
		}
		// If no pawn and no controller, assume single-player (process gestures)
		return true;
	}

	// In multiplayer, only process gestures for locally controlled pawns
	// IsLocallyControlled() returns true only for the local player's pawn
	// This prevents remote players' gesture recognizers from firing events
	return OwnerPawn->IsLocallyControlled();
}

