// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GoKartTrackGenerator.generated.h"

// Forward declarations
class AGoKartTrackSpline;
class UGoKartBarrierSystem;

/**
 * GoKart Track Generator Component
 * 
 * Procedurally generates track geometry and barriers from spline.
 * 
 * Note: Mesh rendering is DEBUG ONLY - track is never visible to players.
 * This is a passthrough/AR experience where the real world is the track.
 * 
 * Generated geometry is used for:
 * - Debug visualization (editor/debugging only)
 * - Barrier collision detection (vertical planar meshes)
 * - Particle effect occlusion
 * 
 * Barriers are equidistant from spline on both sides.
 */
UCLASS(ClassGroup=(HoloCade), meta=(BlueprintSpawnableComponent))
class HOLOCADEEXPERIENCES_API UGoKartTrackGenerator : public UActorComponent
{
	GENERATED_BODY()

public:
	UGoKartTrackGenerator();

	/** Track width in cm (distance from center to barrier on each side) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GoKart|Track|Generation", meta = (ClampMin = "100.0"))
	float TrackWidth = 200.0f; // 2 meters total width

	/** Barrier height in cm */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GoKart|Track|Barriers", meta = (ClampMin = "50.0"))
	float BarrierHeight = 100.0f; // 1 meter tall barriers

	/** Whether to show debug mesh visualization */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GoKart|Track|Debug")
	bool bShowDebugMesh = false;

	/**
	 * Generate track from spline
	 * @param TrackSpline - Spline to generate track from
	 * @return True if generation successful
	 */
	UFUNCTION(BlueprintCallable, Category = "GoKart|Track|Generation")
	bool GenerateTrack(AGoKartTrackSpline* TrackSpline);

	/**
	 * Regenerate track (call when spline changes)
	 */
	UFUNCTION(BlueprintCallable, Category = "GoKart|Track|Generation")
	void RegenerateTrack();

	/**
	 * Get current track spline
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GoKart|Track")
	AGoKartTrackSpline* GetCurrentTrackSpline() const { return CurrentTrackSpline; }

protected:
	virtual void BeginPlay() override;

private:
	/** Current track spline being used */
	UPROPERTY()
	TObjectPtr<AGoKartTrackSpline> CurrentTrackSpline;

	/** Debug mesh component (only visible in editor/debug) */
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> DebugMeshComponent;

	/** Generate barrier meshes (vertical planar meshes equidistant from spline) */
	void GenerateBarriers(AGoKartTrackSpline* TrackSpline);

	/** Create debug visualization mesh */
	void CreateDebugMesh(AGoKartTrackSpline* TrackSpline);
};

