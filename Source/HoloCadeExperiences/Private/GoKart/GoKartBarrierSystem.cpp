// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "GoKart/GoKartBarrierSystem.h"
#include "GoKart/GoKartTrackSpline.h"
#include "GoKart/GoKartBarrierActor.h"
#include "HoloCadeExperiences.h"

UGoKartBarrierSystem::UGoKartBarrierSystem()
{
	PrimaryComponentTick.bCanEverTick = false;
	bShowDebugBarriers = false;
	CurrentTrackWidth = 200.0f;
	CurrentBarrierHeight = 100.0f;
}

void UGoKartBarrierSystem::BeginPlay()
{
	Super::BeginPlay();
	// NOOP: Barrier initialization should be called explicitly by experience
}

void UGoKartBarrierSystem::InitializeBarriers(AGoKartTrackSpline* TrackSpline, float TrackWidth, float BarrierHeight)
{
	if (!TrackSpline)
	{
		UE_LOG(LogGoKart, Error, TEXT("GoKartBarrierSystem: Invalid track spline"));
		return;
	}

	CurrentTrackSpline = TrackSpline;
	CurrentTrackWidth = TrackWidth;
	CurrentBarrierHeight = BarrierHeight;

	GenerateBarrierActors(TrackSpline, TrackWidth, BarrierHeight);
}

void UGoKartBarrierSystem::RegenerateBarriers()
{
	// Clear existing barriers
	for (AGoKartBarrierActor* Barrier : BarrierActors)
	{
		if (IsValid(Barrier))
		{
			Barrier->Destroy();
		}
	}
	BarrierActors.Empty();

	// Regenerate
	if (CurrentTrackSpline)
	{
		GenerateBarrierActors(CurrentTrackSpline, CurrentTrackWidth, CurrentBarrierHeight);
	}
}

bool UGoKartBarrierSystem::CheckProjectileBarrierHit(const FVector& StartLocation, const FVector& EndLocation, FVector& OutHitLocation, FVector& OutHitNormal) const
{
	// NOOP: Will perform line trace against all barrier actors
	// Returns hit location and normal for projectile bounce calculation
	return false;
}

void UGoKartBarrierSystem::GenerateBarrierActors(AGoKartTrackSpline* TrackSpline, float TrackWidth, float BarrierHeight)
{
	// NOOP: Will generate vertical planar mesh barriers equidistant from spline on both sides
	// Barriers are:
	// - NOT visible to players (passthrough/AR)
	// - Used for collision detection only
	// - Synced with real-world barriers by Ops Tech
	// - Equidistant from spline center on both sides
}

