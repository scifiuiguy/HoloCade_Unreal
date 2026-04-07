// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GoKart/Models/GoKartItemDefinition.h"
#include "GoKartItemActor.generated.h"

// Forward declarations
class UStaticMeshComponent;
class USphereComponent;
class UGoKartItemPickup;

/**
 * GoKart Item Actor
 * 
 * Represents an item in the world that can be picked up by players.
 * Contains pickup, display, and projectile asset references.
 * 
 * Blueprint-friendly for level design.
 * Supports hitbox detection for pickup.
 */
UCLASS(BlueprintType, ClassGroup=(HoloCade))
class HOLOCADEEXPERIENCES_API AGoKartItemActor : public AActor
{
	GENERATED_BODY()
	
public:
	AGoKartItemActor();

	/** Item definition (pickup, display, projectile assets) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GoKart|Item")
	TObjectPtr<UGoKartItemDefinition> ItemDefinition;

	/** Pickup mesh component (visible in world) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GoKart|Item")
	TObjectPtr<UStaticMeshComponent> PickupMesh;

	/** Pickup hitbox (sphere collision for detection) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GoKart|Item")
	TObjectPtr<USphereComponent> PickupHitbox;

	/** Whether item is currently picked up */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GoKart|Item")
	bool bIsPickedUp = false;

	/** Distance along track where this item is located (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GoKart|Item")
	float TrackDistance = 0.0f;

	/**
	 * Initialize item from definition
	 * @param Definition - Item definition to use
	 */
	UFUNCTION(BlueprintCallable, Category = "GoKart|Item")
	void InitializeFromDefinition(UGoKartItemDefinition* Definition);

	/**
	 * Handle pickup by player
	 * @param PlayerID - ID of player who picked it up
	 */
	UFUNCTION(BlueprintCallable, Category = "GoKart|Item")
	void OnPickedUp(int32 PlayerID);

	/**
	 * Respawn item after respawn timer
	 */
	UFUNCTION(BlueprintCallable, Category = "GoKart|Item")
	void Respawn();

protected:
	virtual void BeginPlay() override;

	/** Handle overlap with player hitbox */
	UFUNCTION()
	void OnPickupHitboxOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	/** Respawn timer handle */
	FTimerHandle RespawnTimerHandle;
};

