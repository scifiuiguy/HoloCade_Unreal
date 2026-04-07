// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "GoKart/GoKartTrackGenerator.h"
#include "GoKart/GoKartTrackSpline.h"
#include "GoKart/GoKartBarrierSystem.h"
#include "Components/StaticMeshComponent.h"
#include "HoloCadeExperiences.h"

UGoKartTrackGenerator::UGoKartTrackGenerator()
{
	PrimaryComponentTick.bCanEverTick = false;
	TrackWidth = 200.0f;
	BarrierHeight = 100.0f;
	bShowDebugMesh = false;
}

void UGoKartTrackGenerator::BeginPlay()
{
	Super::BeginPlay();
	// NOOP: Track generation should be called explicitly by experience
}

bool UGoKartTrackGenerator::GenerateTrack(AGoKartTrackSpline* TrackSpline)
{
	if (!TrackSpline || !TrackSpline->SplineComponent)
	{
		UE_LOG(LogGoKart, Error, TEXT("GoKartTrackGenerator: Invalid track spline"));
		return false;
	}

	CurrentTrackSpline = TrackSpline;

	// Generate barriers (vertical planar meshes)
	GenerateBarriers(TrackSpline);

	// Create debug mesh if enabled
	if (bShowDebugMesh)
	{
		CreateDebugMesh(TrackSpline);
	}

	UE_LOG(LogGoKart, Log, TEXT("GoKartTrackGenerator: Generated track from spline '%s'"), *TrackSpline->TrackName);
	return true;
}

void UGoKartTrackGenerator::RegenerateTrack()
{
	if (CurrentTrackSpline)
	{
		GenerateTrack(CurrentTrackSpline);
	}
}

void UGoKartTrackGenerator::GenerateBarriers(AGoKartTrackSpline* TrackSpline)
{
	// NOOP: Will generate vertical planar meshes equidistant from spline on both sides
	// These meshes are used for:
	// - Projectile collision detection
	// - Particle effect occlusion
	// - NOT visible to players (passthrough/AR experience)
	
	// Implementation will:
	// 1. Sample spline at regular intervals
	// 2. Calculate perpendicular vectors at each point
	// 3. Create vertical planar meshes at TrackWidth distance on each side
	// 4. Register with BarrierSystem for collision detection
}

void UGoKartTrackGenerator::CreateDebugMesh(AGoKartTrackSpline* TrackSpline)
{
	// NOOP: Will create debug visualization mesh
	// Only visible in editor or when bShowDebugMesh is true
	// Used for level design and debugging only
}

