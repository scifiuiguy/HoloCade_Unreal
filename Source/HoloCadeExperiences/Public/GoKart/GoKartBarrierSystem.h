// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GoKartBarrierSystem.generated.h"

// Forward declarations
class AGoKartTrackSpline;
class AGoKartBarrierActor;

/**
 * GoKart Barrier System Component
 * 
 * Manages barrier collision detection for projectiles.
 * Barriers are vertical planar meshes equidistant from track spline on both sides.
 * 
 * Barriers are NOT visible to players (passthrough/AR experience).
 * They are synced with real-world barrier surfaces by Ops Tech.
 * 
 * Used for:
 * - Projectile collision detection (bounce off barriers)
 * - Particle effect occlusion
 * - Debug visualization of collision hitboxes
 */
UCLASS(ClassGroup=(HoloCade), meta=(BlueprintSpawnableComponent))
class HOLOCADEEXPERIENCES_API UGoKartBarrierSystem : public UActorComponent
{
	GENERATED_BODY()

public:
	UGoKartBarrierSystem();

	/** Spawned barrier actors */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GoKart|Barriers")
	TArray<TObjectPtr<AGoKartBarrierActor>> BarrierActors;

	/** Whether to show debug barrier visualization */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GoKart|Barriers|Debug")
	bool bShowDebugBarriers = false;

	/**
	 * Initialize barriers from track
	 * @param TrackSpline - Track to generate barriers for
	 * @param TrackWidth - Distance from center to barrier on each side (cm)
	 * @param BarrierHeight - Height of barriers (cm)
	 */
	UFUNCTION(BlueprintCallable, Category = "GoKart|Barriers")
	void InitializeBarriers(AGoKartTrackSpline* TrackSpline, float TrackWidth, float BarrierHeight);

	/**
	 * Regenerate barriers (call when track changes)
	 */
	UFUNCTION(BlueprintCallable, Category = "GoKart|Barriers")
	void RegenerateBarriers();

	/**
	 * Check if projectile hit a barrier
	 * @param StartLocation - Projectile start location
	 * @param EndLocation - Projectile end location
	 * @param OutHitLocation - Location where barrier was hit
	 * @param OutHitNormal - Normal of barrier surface at hit point
	 * @return True if barrier was hit
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GoKart|Barriers")
	bool CheckProjectileBarrierHit(const FVector& StartLocation, const FVector& EndLocation, FVector& OutHitLocation, FVector& OutHitNormal) const;

protected:
	virtual void BeginPlay() override;

private:
	/** Current track spline */
	UPROPERTY()
	TObjectPtr<AGoKartTrackSpline> CurrentTrackSpline;

	/** Track width used for barrier generation */
	float CurrentTrackWidth = 200.0f;

	/** Barrier height used for generation */
	float CurrentBarrierHeight = 100.0f;

	/** Generate barrier actors along track */
	void GenerateBarrierActors(AGoKartTrackSpline* TrackSpline, float TrackWidth, float BarrierHeight);
};

