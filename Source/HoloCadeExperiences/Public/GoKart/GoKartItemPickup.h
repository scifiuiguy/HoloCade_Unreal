// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GoKart/Models/GoKartItemDefinition.h"
#include "GoKartItemPickup.generated.h"

// Forward declarations
class AGoKartTrackSpline;
class AGoKartItemActor;

/**
 * GoKart Item Pickup System Component
 * 
 * Manages item spawning, pickup detection, and item lifecycle along the track.
 * Items are spawned at configurable positions along the track spline.
 * 
 * Supports hitbox detection for pickup and projectile blueprints.
 */
UCLASS(ClassGroup=(HoloCade), meta=(BlueprintSpawnableComponent))
class HOLOCADEEXPERIENCES_API UGoKartItemPickup : public UActorComponent
{
	GENERATED_BODY()

public:
	UGoKartItemPickup();

	/** Item definitions available for spawning */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GoKart|Items")
	TArray<TObjectPtr<UGoKartItemDefinition>> ItemDefinitions;

	/** Spawned item actors */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GoKart|Items")
	TArray<TObjectPtr<AGoKartItemActor>> SpawnedItems;

	/** Distance between item spawns along track (in cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GoKart|Items|Spawning", meta = (ClampMin = "100.0"))
	float ItemSpawnInterval = 500.0f; // 5 meters

	/** Item respawn time after pickup (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GoKart|Items|Spawning", meta = (ClampMin = "1.0"))
	float ItemRespawnTime = 10.0f;

	/**
	 * Initialize items along track
	 * @param TrackSpline - Track to spawn items along
	 */
	UFUNCTION(BlueprintCallable, Category = "GoKart|Items")
	void InitializeItems(AGoKartTrackSpline* TrackSpline);

	/**
	 * Regenerate items (call when track changes)
	 */
	UFUNCTION(BlueprintCallable, Category = "GoKart|Items")
	void RegenerateItems();

	/**
	 * Spawn item at specific distance along track
	 * @param Distance - Distance along track in cm
	 * @param ItemDefinition - Item definition to spawn
	 * @return Spawned item actor, or nullptr if failed
	 */
	UFUNCTION(BlueprintCallable, Category = "GoKart|Items")
	AGoKartItemActor* SpawnItemAtDistance(float Distance, UGoKartItemDefinition* ItemDefinition);

	/**
	 * Handle item pickup (called by item actor when player picks it up)
	 * @param ItemActor - Item that was picked up
	 * @param PlayerID - ID of player who picked it up
	 */
	UFUNCTION(BlueprintCallable, Category = "GoKart|Items")
	void OnItemPickedUp(AGoKartItemActor* ItemActor, int32 PlayerID);

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	/** Current track spline */
	UPROPERTY()
	TObjectPtr<AGoKartTrackSpline> CurrentTrackSpline;

	/** Spawn items along track at regular intervals */
	void SpawnItemsAlongTrack(AGoKartTrackSpline* TrackSpline);
};

