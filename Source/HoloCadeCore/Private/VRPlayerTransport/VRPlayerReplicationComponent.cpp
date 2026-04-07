// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "VRPlayerTransport/VRPlayerReplicationComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "IXRTrackingSystem.h"
#include "IHandTracker.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HeadMountedDisplayTypes.h"
#include "Features/IModularFeatures.h"
#include "Net/UnrealNetwork.h"

UHoloCadeVRPlayerReplicationComponent::UHoloCadeVRPlayerReplicationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	
	XRSystem = nullptr;
	HandTracker = nullptr;
	UpdateTimer = 0.0f;
	bIsLocalPlayer = false;
	bEnableReplication = true;
	ReplicationUpdateRate = 60.0f;
	
	// Enable replication
	SetIsReplicatedByDefault(true);
}

void UHoloCadeVRPlayerReplicationComponent::BeginPlay()
{
	Super::BeginPlay();

	// Determine if this is the local player's component
	APawn* OwnerPawn = GetOwner<APawn>();
	if (OwnerPawn)
	{
		bIsLocalPlayer = OwnerPawn->IsLocallyControlled();
	}
	else
	{
		// If not attached to a pawn, assume not local player
		bIsLocalPlayer = false;
	}

	// Only capture OpenXR data on the local player's client
	if (bIsLocalPlayer)
	{
		// Initialize XR system and hand tracker
		GetXRSystem();
		GetHandTracker();
	}
}

void UHoloCadeVRPlayerReplicationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bEnableReplication)
	{
		return;
	}

	// Only capture and replicate on the local player's client
	if (!bIsLocalPlayer)
	{
		return;
	}

	// Update timer for rate control
	UpdateTimer += DeltaTime;
	float UpdateInterval = 1.0f / ReplicationUpdateRate;

	if (UpdateTimer >= UpdateInterval)
	{
		CaptureAndReplicateXRData();
		UpdateTimer = 0.0f;
	}
}

void UHoloCadeVRPlayerReplicationComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Replicate XR data to all clients
	DOREPLIFETIME(UHoloCadeVRPlayerReplicationComponent, ReplicatedXRData);
}

void UHoloCadeVRPlayerReplicationComponent::OnRep_ReplicatedXRData()
{
	// Called when replicated XR data is received from server
	// This is where you could fire delegates or update visual representations
	// For now, the data is automatically available via GetReplicatedXRData()
}

FTransform UHoloCadeVRPlayerReplicationComponent::GetReplicatedHMDTransform() const
{
	return ReplicatedXRData.GetHMDTransform();
}

FTransform UHoloCadeVRPlayerReplicationComponent::GetReplicatedHandKeypointTransform(bool bLeftHand, EHandKeypoint Keypoint) const
{
	const FReplicatedHandData& HandData = bLeftHand ? ReplicatedXRData.LeftHand : ReplicatedXRData.RightHand;
	const FReplicatedHandKeypoint* KeypointData = HandData.GetKeypoint(Keypoint);
	
	if (KeypointData && KeypointData->bIsTracked)
	{
		return KeypointData->ToTransform();
	}
	
	return FTransform::Identity;
}

bool UHoloCadeVRPlayerReplicationComponent::IsHandTrackingActive(bool bLeftHand) const
{
	const FReplicatedHandData& HandData = bLeftHand ? ReplicatedXRData.LeftHand : ReplicatedXRData.RightHand;
	return HandData.bIsHandTrackingActive;
}

bool UHoloCadeVRPlayerReplicationComponent::IsLocalPlayer() const
{
	return bIsLocalPlayer;
}

void UHoloCadeVRPlayerReplicationComponent::CaptureAndReplicateXRData()
{
	// Only capture on local player's client
	if (!bIsLocalPlayer)
	{
		return;
	}

	// Create new data structure
	FHoloCadeXRReplicatedData NewData;

	// Capture HMD transform
	CaptureHMDTransform(NewData);

	// Capture hand tracking data
	CaptureHandTrackingData(NewData);

	// Set server timestamp (will be set by server when replicated)
	if (UWorld* World = GetWorld())
	{
		if (AGameStateBase* GameState = World->GetGameState())
		{
			NewData.ServerTimeStamp = GameState->GetServerWorldTimeSeconds();
		}
	}

	// Update replicated data (will be sent to server, then to all clients)
	ReplicatedXRData = NewData;
}

IXRTrackingSystem* UHoloCadeVRPlayerReplicationComponent::GetXRSystem() const
{
	if (!XRSystem && GEngine && GEngine->XRSystem.IsValid())
	{
		XRSystem = GEngine->XRSystem.Get();
	}
	return XRSystem;
}

IHandTracker* UHoloCadeVRPlayerReplicationComponent::GetHandTracker() const
{
	if (!HandTracker)
	{
		// Access IHandTracker via IModularFeatures
		IModularFeatures& ModularFeatures = IModularFeatures::Get();
		FName HandTrackerFeatureName = IHandTracker::GetModularFeatureName();
		if (ModularFeatures.IsModularFeatureAvailable(HandTrackerFeatureName))
		{
			HandTracker = static_cast<IHandTracker*>(ModularFeatures.GetModularFeatureImplementation(HandTrackerFeatureName, 0));
		}
	}
	return HandTracker;
}

void UHoloCadeVRPlayerReplicationComponent::CaptureHMDTransform(FHoloCadeXRReplicatedData& OutData)
{
	IXRTrackingSystem* System = GetXRSystem();
	if (!System)
	{
		OutData.bIsHMDTracked = false;
		return;
	}

	FQuat HMDOrientation;
	FVector HMDPosition;
	if (System->GetCurrentPose(IXRTrackingSystem::HMDDeviceId, HMDOrientation, HMDPosition))
	{
		OutData.HMDPosition = HMDPosition;
		OutData.HMDRotation = HMDOrientation.Rotator();
		OutData.bIsHMDTracked = true;
	}
	else
	{
		OutData.bIsHMDTracked = false;
	}
}

void UHoloCadeVRPlayerReplicationComponent::CaptureHandTrackingData(FHoloCadeXRReplicatedData& OutData)
{
	IHandTracker* Tracker = GetHandTracker();
	if (!Tracker)
	{
		OutData.LeftHand.bIsHandTrackingActive = false;
		OutData.RightHand.bIsHandTrackingActive = false;
		return;
	}

	// Capture left hand
	bool bLeftHandActive = false;
	CaptureHandKeypoint(EControllerHand::Left, EHandKeypoint::Wrist, OutData.LeftHand);
	if (OutData.LeftHand.Wrist.bIsTracked)
	{
		bLeftHandActive = true;
	}
	CaptureHandKeypoint(EControllerHand::Left, EHandKeypoint::MiddleMetacarpal, OutData.LeftHand);
	CaptureHandKeypoint(EControllerHand::Left, EHandKeypoint::ThumbTip, OutData.LeftHand);
	CaptureHandKeypoint(EControllerHand::Left, EHandKeypoint::IndexTip, OutData.LeftHand);
	CaptureHandKeypoint(EControllerHand::Left, EHandKeypoint::MiddleTip, OutData.LeftHand);
	CaptureHandKeypoint(EControllerHand::Left, EHandKeypoint::RingTip, OutData.LeftHand);
	CaptureHandKeypoint(EControllerHand::Left, EHandKeypoint::LittleTip, OutData.LeftHand);
	OutData.LeftHand.bIsHandTrackingActive = bLeftHandActive;

	// Capture right hand
	bool bRightHandActive = false;
	CaptureHandKeypoint(EControllerHand::Right, EHandKeypoint::Wrist, OutData.RightHand);
	if (OutData.RightHand.Wrist.bIsTracked)
	{
		bRightHandActive = true;
	}
	CaptureHandKeypoint(EControllerHand::Right, EHandKeypoint::MiddleMetacarpal, OutData.RightHand);
	CaptureHandKeypoint(EControllerHand::Right, EHandKeypoint::ThumbTip, OutData.RightHand);
	CaptureHandKeypoint(EControllerHand::Right, EHandKeypoint::IndexTip, OutData.RightHand);
	CaptureHandKeypoint(EControllerHand::Right, EHandKeypoint::MiddleTip, OutData.RightHand);
	CaptureHandKeypoint(EControllerHand::Right, EHandKeypoint::RingTip, OutData.RightHand);
	CaptureHandKeypoint(EControllerHand::Right, EHandKeypoint::LittleTip, OutData.RightHand);
	OutData.RightHand.bIsHandTrackingActive = bRightHandActive;
}

void UHoloCadeVRPlayerReplicationComponent::CaptureHandKeypoint(EControllerHand Hand, EHandKeypoint Keypoint, FReplicatedHandData& OutHandData)
{
	IHandTracker* Tracker = GetHandTracker();
	if (!Tracker)
	{
		return;
	}

	FTransform OutTransform;
	float OutRadius;
	if (Tracker->GetKeypointState(Hand, Keypoint, OutTransform, OutRadius))
	{
		FReplicatedHandKeypoint KeypointData(
			OutTransform.GetLocation(),
			OutTransform.Rotator(),
			true,
			OutRadius
		);
		OutHandData.SetKeypoint(Keypoint, KeypointData);
	}
	else
	{
		// Keypoint not tracked
		FReplicatedHandKeypoint KeypointData(
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			false,
			0.0f
		);
		OutHandData.SetKeypoint(Keypoint, KeypointData);
	}
}

