// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GoKartBarrierActor.generated.h"

// Forward declarations
class UStaticMeshComponent;
class UBoxComponent;

/**
 * GoKart Barrier Actor
 * 
 * Represents a barrier segment along the track.
 * Vertical planar mesh used for projectile collision detection.
 * 
 * NOT visible to players (passthrough/AR experience).
 * Synced with real-world barrier surfaces by Ops Tech.
 */
UCLASS(BlueprintType, ClassGroup=(HoloCade))
class HOLOCADEEXPERIENCES_API AGoKartBarrierActor : public AActor
{
	GENERATED_BODY()
	
public:
	AGoKartBarrierActor();

	/** Barrier mesh component (vertical planar mesh, collision only) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GoKart|Barrier")
	TObjectPtr<UStaticMeshComponent> BarrierMesh;

	/** Collision box for projectile detection */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GoKart|Barrier")
	TObjectPtr<UBoxComponent> CollisionBox;

	/** Whether to show debug visualization */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GoKart|Barrier|Debug")
	bool bShowDebugVisualization = false;

	/**
	 * Initialize barrier at position
	 * @param Location - World location
	 * @param Rotation - World rotation (normal should face track center)
	 * @param Width - Barrier width in cm
	 * @param Height - Barrier height in cm
	 */
	UFUNCTION(BlueprintCallable, Category = "GoKart|Barrier")
	void InitializeBarrier(const FVector& Location, const FRotator& Rotation, float Width, float Height);

protected:
	virtual void BeginPlay() override;
};

