// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "SuperheroFlight/GestureDebugger.h"
#include "SuperheroFlight/FlightHandsController.h"
#include "HoloCadeExperiences.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Engine/Canvas.h"
#include "Kismet/GameplayStatics.h"

UGestureDebugger::UGestureDebugger()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.0f; // Tick every frame for debug visualization
	bDebugEnabled = false;
}

UGestureDebugger::~UGestureDebugger()
{
}

void UGestureDebugger::BeginPlay()
{
	Super::BeginPlay();
	// NOOP: Debugger initialization should be called explicitly by experience
}

void UGestureDebugger::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bDebugEnabled && FlightHandsController)
	{
		DrawDebugVisualization();
	}
}

bool UGestureDebugger::InitializeDebugger(UFlightHandsController* InFlightHandsController)
{
	if (!InFlightHandsController)
	{
		UE_LOG(LogSuperheroFlight, Error, TEXT("GestureDebugger: Invalid flight hands controller"));
		return false;
	}

	FlightHandsController = InFlightHandsController;
	UE_LOG(LogSuperheroFlight, Log, TEXT("GestureDebugger: Initialized"));
	return true;
}

void UGestureDebugger::SetDebugEnabled(bool bEnabled)
{
	bDebugEnabled = bEnabled;
}

void UGestureDebugger::DrawDebugVisualization()
{
	DrawHandPositions();
	DrawGestureVectors();
	DrawAngleThresholds();
	DrawVirtualAltitudeRaycast();
	DrawHUDText();
}

void UGestureDebugger::DrawHandPositions()
{
	if (!FlightHandsController || !GetWorld())
	{
		return;
	}

	FSuperheroFlightGestureState GestureState = FlightHandsController->GetGestureState();
	
	// Get actual hand positions from FlightHandsController
	FVector HMDPos = FlightHandsController->GetHMDPosition();
	FVector LeftHandPos = FlightHandsController->GetLeftHandPosition();
	FVector RightHandPos = FlightHandsController->GetRightHandPosition();

	// Draw hand positions as spheres
	DrawDebugSphere(GetWorld(), LeftHandPos, 5.0f, 12, FColor::Blue, false, 0.0f, 0, 2.0f);
	DrawDebugSphere(GetWorld(), RightHandPos, 5.0f, 12, FColor::Red, false, 0.0f, 0, 2.0f);

	// Draw line from HMD to each hand
	DrawDebugLine(GetWorld(), HMDPos, LeftHandPos, FColor::Blue, false, 0.0f, 0, 1.0f);
	DrawDebugLine(GetWorld(), HMDPos, RightHandPos, FColor::Red, false, 0.0f, 0, 1.0f);

	// Draw center point between hands
	FVector HandsCenter = (LeftHandPos + RightHandPos) * 0.5f;
	DrawDebugSphere(GetWorld(), HandsCenter, 3.0f, 12, FColor::Yellow, false, 0.0f, 0, 2.0f);
}

void UGestureDebugger::DrawGestureVectors()
{
	if (!FlightHandsController || !GetWorld())
	{
		return;
	}

	FSuperheroFlightGestureState GestureState = FlightHandsController->GetGestureState();
	FVector HMDPos = FlightHandsController->GetHMDPosition();
	FVector LeftHandPos = FlightHandsController->GetLeftHandPosition();
	FVector RightHandPos = FlightHandsController->GetRightHandPosition();
	FVector HandsCenter = (LeftHandPos + RightHandPos) * 0.5f;

	// Draw gesture direction vector (from HMD to hands center)
	FVector GestureDir = GestureState.GestureDirection * 50.0f;  // Scale for visibility
	DrawDebugLine(GetWorld(), HMDPos, HMDPos + GestureDir, FColor::Green, false, 0.0f, 0, 2.0f);
	DrawDebugLine(GetWorld(), HMDPos, HMDPos + GestureDir, FColor::Green, false, 0.0f, 0, 2.0f);
}

void UGestureDebugger::DrawAngleThresholds()
{
	if (!FlightHandsController || !GetWorld())
	{
		return;
	}

	FSuperheroFlightGestureState GestureState = FlightHandsController->GetGestureState();
	float UpToForwardAngle = FlightHandsController->UpToForwardAngle;
	float ForwardToDownAngle = FlightHandsController->ForwardToDownAngle;

	FVector HMDPos = FlightHandsController->GetHMDPosition();
	FVector LeftHandPos = FlightHandsController->GetLeftHandPosition();
	FVector RightHandPos = FlightHandsController->GetRightHandPosition();
	FVector HandsCenter = (LeftHandPos + RightHandPos) * 0.5f;

	// Draw line from head to average point between hands (current gesture direction)
	FVector HMDToHands = (HandsCenter - HMDPos).GetSafeNormal();
	
	// Draw current gesture direction line (green)
	DrawDebugLine(GetWorld(), HMDPos, HMDPos + HMDToHands * 50.0f, FColor::Green, false, 0.0f, 0, 2.0f);

	// Draw line from head to point representing the up-to-forward angle threshold
	// Angle is measured from world up vector (0° = up, 90° = horizontal, 180° = down)
	// Rotate from up vector towards forward by UpToForwardAngle degrees
	FVector WorldUp = FVector::UpVector;
	FVector ThresholdDirection = WorldUp.RotateAngleAxis(UpToForwardAngle, FVector::RightVector);
	FVector ThresholdPoint = HMDPos + ThresholdDirection * 50.0f;
	DrawDebugLine(GetWorld(), HMDPos, ThresholdPoint, FColor::Yellow, false, 0.0f, 0, 1.0f);

	// Draw line from head to point representing the forward-to-down angle threshold
	// Rotate from up vector by (UpToForwardAngle + ForwardToDownAngle) degrees
	FVector ForwardToDownThresholdDirection = WorldUp.RotateAngleAxis(UpToForwardAngle + ForwardToDownAngle, FVector::RightVector);
	FVector ForwardToDownThresholdPoint = HMDPos + ForwardToDownThresholdDirection * 50.0f;
	DrawDebugLine(GetWorld(), HMDPos, ForwardToDownThresholdPoint, FColor::Orange, false, 0.0f, 0, 1.0f);
}

void UGestureDebugger::DrawVirtualAltitudeRaycast()
{
	if (!FlightHandsController || !GetWorld())
	{
		return;
	}

	FSuperheroFlightGestureState GestureState = FlightHandsController->GetGestureState();
	
	if (GestureState.VirtualAltitude > 0.0f)
	{
		FVector HMDPos = FlightHandsController->GetHMDPosition();
		FVector WorldDown = -FVector::UpVector;
		float Distance = GestureState.VirtualAltitude * 2.54f;  // Convert inches to cm

		// Draw raycast line
		DrawDebugLine(GetWorld(), HMDPos, HMDPos + WorldDown * Distance, FColor::Cyan, false, 0.0f, 0, 1.0f);
		
		// Draw hit point
		FVector HitPoint = HMDPos + WorldDown * Distance;
		DrawDebugSphere(GetWorld(), HitPoint, 10.0f, 12, FColor::Cyan, false, 0.0f, 0, 2.0f);
	}
}

void UGestureDebugger::DrawHUDText()
{
	// NOOP: Will draw HUD text overlay showing:
	// - Current flight mode
	// - Arm extension percentage
	// - Virtual altitude
	// - Gesture angle
	// - Fist states
	// This requires UMG HUD widget implementation
}

