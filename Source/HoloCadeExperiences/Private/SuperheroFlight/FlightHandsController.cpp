// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "SuperheroFlight/FlightHandsController.h"
#include "HoloCadeExperiences.h"
#include "Engine/Engine.h"
#include "IXRTrackingSystem.h"
#include "IHandTracker.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "HeadMountedDisplayTypes.h"
#include "Features/IModularFeatures.h"

UFlightHandsController::UFlightHandsController()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.0f; // Tick every frame for gesture detection
	UpToForwardAngle = 45.0f;
	ForwardToDownAngle = 45.0f;
	ArmLength = 28.0f;
	VirtualAltitudeRaycastDistance = 600.0f;
}

UFlightHandsController::~UFlightHandsController()
{
}

void UFlightHandsController::BeginPlay()
{
	Super::BeginPlay();
	// NOOP: Gesture controller initialization should be called explicitly by experience
}

void UFlightHandsController::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (PlayerController)
	{
		UpdateGestureState(DeltaTime);
	}
}

bool UFlightHandsController::InitializeGestureController(APlayerController* InPlayerController)
{
	if (!InPlayerController)
	{
		UE_LOG(LogSuperheroFlight, Error, TEXT("FlightHandsController: Invalid player controller"));
		return false;
	}

	PlayerController = InPlayerController;

	// Get XR system (for HMD and hand tracking)
	XRSystem = GetXRSystem();
	if (!XRSystem)
	{
		UE_LOG(LogSuperheroFlight, Warning, TEXT("FlightHandsController: XR system not available - using fallback methods"));
	}

	// Get hand tracker
	HandTracker = GetHandTracker();
	if (!HandTracker)
	{
		UE_LOG(LogSuperheroFlight, Warning, TEXT("FlightHandsController: Hand tracker not available - hand tracking will use fallback methods"));
	}

	UE_LOG(LogSuperheroFlight, Log, TEXT("FlightHandsController: Initialized"));
	return true;
}

void UFlightHandsController::UpdateGestureState(float DeltaTime)
{
	// Only process gestures for locally controlled pawns (multiplayer safety)
	if (!ShouldProcessGestures())
	{
		return;
	}

	LastGestureState = CurrentGestureState;

	// Detect fist state
	DetectFistState();

	// Calculate gesture direction
	CalculateGestureDirection();

	// Calculate flight speed throttle
	CalculateFlightSpeedThrottle();

	// Calculate virtual altitude
	CalculateVirtualAltitude();

	// Determine flight mode
	DetermineFlightMode();

	// NOOP: Replicate gesture state to server (multiplayer replication)
}

void UFlightHandsController::DetectFistState()
{
	// Use Unreal's native hand tracking APIs directly
	if (HandTracker)
	{
		CurrentGestureState.bLeftFistClosed = IsHandFistClosed(true);
		CurrentGestureState.bRightFistClosed = IsHandFistClosed(false);
	}
	else
	{
		// Fallback: Assume both fists closed for testing (when hand tracking not available)
		CurrentGestureState.bLeftFistClosed = true;
		CurrentGestureState.bRightFistClosed = true;
		UE_LOG(LogSuperheroFlight, Verbose, TEXT("FlightHandsController: Hand tracking not available - using fallback fist detection"));
	}

	CurrentGestureState.bBothFistsClosed = CurrentGestureState.bLeftFistClosed && CurrentGestureState.bRightFistClosed;
}

void UFlightHandsController::CalculateGestureDirection()
{
	FVector HMDPos = GetHMDPosition();
	FVector LeftHandPos = GetLeftHandPosition();
	FVector RightHandPos = GetRightHandPosition();

	// Calculate center point between hands
	FVector HandsCenter = (LeftHandPos + RightHandPos) * 0.5f;

	// Calculate vector from HMD to hands center
	FVector HMDToHands = (HandsCenter - HMDPos).GetSafeNormal();
	CurrentGestureState.GestureDirection = HMDToHands;

	// Calculate angle relative to world ground plane (up vector)
	FVector WorldUp = FVector::UpVector;
	float DotProduct = FVector::DotProduct(HMDToHands, WorldUp);
	CurrentGestureState.GestureAngle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(DotProduct, -1.0f, 1.0f)));
}

void UFlightHandsController::CalculateFlightSpeedThrottle()
{
	FVector HMDPos = GetHMDPosition();
	FVector LeftHandPos = GetLeftHandPosition();
	FVector RightHandPos = GetRightHandPosition();

	// Calculate center point between hands
	FVector HandsCenter = (LeftHandPos + RightHandPos) * 0.5f;

	// Calculate distance between HMD and hands center
	float Distance = FVector::Dist(HMDPos, HandsCenter);

	// Normalize by arm length (convert inches to cm: 1 inch = 2.54 cm)
	float DistanceInches = Distance / 2.54f;
	CurrentGestureState.FlightSpeedThrottle = FMath::Clamp(DistanceInches / ArmLength, 0.0f, 1.0f);
}

void UFlightHandsController::CalculateVirtualAltitude()
{
	FVector HMDPos = GetHMDPosition();
	FVector WorldDown = -FVector::UpVector;

	FVector HitPoint;
	if (RaycastForLandableSurface(HMDPos, WorldDown, VirtualAltitudeRaycastDistance, HitPoint))
	{
		// Calculate distance from HMD to hit point (in inches)
		float Distance = FVector::Dist(HMDPos, HitPoint);
		CurrentGestureState.VirtualAltitude = Distance / 2.54f;  // Convert cm to inches
	}
	else
	{
		CurrentGestureState.VirtualAltitude = -1.0f;  // No landable surface found
	}
}

void UFlightHandsController::DetermineFlightMode()
{
	// If both fists not closed, player is in hovering mode (or standing if on ground)
	if (!CurrentGestureState.bBothFistsClosed)
	{
		if (CurrentGestureState.VirtualAltitude > 0.0f && CurrentGestureState.VirtualAltitude < 12.0f)
		{
			// Player is close to landable surface, transition to standing
			CurrentGestureState.CurrentFlightMode = ESuperheroFlightGameState::Standing;
		}
		else
		{
			CurrentGestureState.CurrentFlightMode = ESuperheroFlightGameState::Hovering;
		}
		return;
	}

	// Both fists closed - determine flight direction from gesture angle
	float Angle = CurrentGestureState.GestureAngle;

	if (Angle < UpToForwardAngle)
	{
		// Arms pointing up
		CurrentGestureState.CurrentFlightMode = ESuperheroFlightGameState::FlightUp;
	}
	else if (Angle < (UpToForwardAngle + ForwardToDownAngle))
	{
		// Arms pointing forward
		CurrentGestureState.CurrentFlightMode = ESuperheroFlightGameState::FlightForward;
	}
	else
	{
		// Arms pointing down
		CurrentGestureState.CurrentFlightMode = ESuperheroFlightGameState::FlightDown;
	}
}

FVector UFlightHandsController::GetHMDPosition() const
{
	// Use Unreal's native XR system directly
	IXRTrackingSystem* System = GetXRSystem();
	if (System)
	{
		FQuat HMDOrientation;
		FVector HMDPosition;
		if (System->GetCurrentPose(IXRTrackingSystem::HMDDeviceId, HMDOrientation, HMDPosition))
		{
			return HMDPosition;
		}
	}

	// Fallback: Use player controller camera position
	if (PlayerController)
	{
		FVector CameraLocation;
		FRotator CameraRotation;
		PlayerController->GetPlayerViewPoint(CameraLocation, CameraRotation);
		return CameraLocation;
	}

	// Last resort: Use pawn location
	if (PlayerController && PlayerController->GetPawn())
	{
		return PlayerController->GetPawn()->GetActorLocation();
	}

	return FVector::ZeroVector;
}

FVector UFlightHandsController::GetLeftHandPosition() const
{
	// Use Unreal's native hand tracking APIs directly
	IHandTracker* Tracker = GetHandTracker();
	if (Tracker)
	{
		// Try wrist first
		FTransform WristTransform = GetHandNodeTransform(true, EHandKeypoint::Wrist);
		if (!WristTransform.Equals(FTransform::Identity))
		{
			return WristTransform.GetLocation();
		}

		// Fallback to hand center (middle knuckle/MCP)
		FTransform HandCenterTransform = GetHandNodeTransform(true, EHandKeypoint::MiddleMetacarpal);
		if (!HandCenterTransform.Equals(FTransform::Identity))
		{
			return HandCenterTransform.GetLocation();
		}
	}

	// Fallback: Return offset position for testing (when hand tracking not available)
	FVector HMDPos = GetHMDPosition();
	return HMDPos + FVector(-20.0f, 0.0f, -10.0f);  // Left side, slightly lower
}

FVector UFlightHandsController::GetRightHandPosition() const
{
	// Use Unreal's native hand tracking APIs directly
	IHandTracker* Tracker = GetHandTracker();
	if (Tracker)
	{
		// Try wrist first
		FTransform WristTransform = GetHandNodeTransform(false, EHandKeypoint::Wrist);
		if (!WristTransform.Equals(FTransform::Identity))
		{
			return WristTransform.GetLocation();
		}

		// Fallback to hand center (middle knuckle/MCP)
		FTransform HandCenterTransform = GetHandNodeTransform(false, EHandKeypoint::MiddleMetacarpal);
		if (!HandCenterTransform.Equals(FTransform::Identity))
		{
			return HandCenterTransform.GetLocation();
		}
	}

	// Fallback: Return offset position for testing (when hand tracking not available)
	FVector HMDPos = GetHMDPosition();
	return HMDPos + FVector(20.0f, 0.0f, -10.0f);  // Right side, slightly lower
}

bool UFlightHandsController::IsHandFistClosed(bool bLeftHand) const
{
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

	const float FistThreshold = 2.0f; // 2 inches (~5cm)
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
		
		if (DistanceToCenter < FistThreshold)
		{
			FingersClosed++;
		}
	}

	// Consider hand closed if at least 4 out of 5 fingertips are close to center
	return FingersClosed >= 4;
}

IXRTrackingSystem* UFlightHandsController::GetXRSystem() const
{
	if (!XRSystem && GEngine && GEngine->XRSystem.IsValid())
	{
		XRSystem = GEngine->XRSystem.Get();
	}
	return XRSystem;
}

IHandTracker* UFlightHandsController::GetHandTracker() const
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

FTransform UFlightHandsController::GetHandNodeTransform(bool bLeftHand, EHandKeypoint Keypoint) const
{
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

bool UFlightHandsController::ShouldProcessGestures() const
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
		if (PlayerController)
		{
			return PlayerController->IsLocalController();
		}
		// If no pawn and no controller, assume single-player (process gestures)
		return true;
	}

	// In multiplayer, only process gestures for locally controlled pawns
	// IsLocallyControlled() returns true only for the local player's pawn
	// This prevents remote players' gesture recognizers from firing events
	return OwnerPawn->IsLocallyControlled();
}

bool UFlightHandsController::RaycastForLandableSurface(const FVector& Start, const FVector& Direction, float Distance, FVector& OutHitPoint) const
{
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());

	if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, Start + Direction * Distance, ECC_WorldStatic, QueryParams))
	{
		// Check if hit surface is marked as "landable" using collision tags
		if (HitResult.GetActor() && HitResult.GetActor()->ActorHasTag(TEXT("Landable")))
		{
			OutHitPoint = HitResult.ImpactPoint;
			return true;
		}
	}

	return false;
}

