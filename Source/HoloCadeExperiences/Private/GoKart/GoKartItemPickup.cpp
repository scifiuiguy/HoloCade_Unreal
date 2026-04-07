// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "GoKart/GoKartItemPickup.h"
#include "GoKart/GoKartTrackSpline.h"
#include "GoKart/GoKartItemActor.h"
#include "HoloCadeExperiences.h"

UGoKartItemPickup::UGoKartItemPickup()
{
	PrimaryComponentTick.bCanEverTick = true;
	ItemSpawnInterval = 500.0f;
	ItemRespawnTime = 10.0f;
}

void UGoKartItemPickup::BeginPlay()
{
	Super::BeginPlay();
	// NOOP: Item initialization should be called explicitly by experience
}

void UGoKartItemPickup::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	// NOOP: Will handle item respawn timers
}

void UGoKartItemPickup::InitializeItems(AGoKartTrackSpline* TrackSpline)
{
	if (!TrackSpline)
	{
		UE_LOG(LogGoKart, Error, TEXT("GoKartItemPickup: Invalid track spline"));
		return;
	}

	CurrentTrackSpline = TrackSpline;
	SpawnItemsAlongTrack(TrackSpline);
}

void UGoKartItemPickup::RegenerateItems()
{
	// Clear existing items
	for (AGoKartItemActor* Item : SpawnedItems)
	{
		if (IsValid(Item))
		{
			Item->Destroy();
		}
	}
	SpawnedItems.Empty();

	// Regenerate
	if (CurrentTrackSpline)
	{
		SpawnItemsAlongTrack(CurrentTrackSpline);
	}
}

AGoKartItemActor* UGoKartItemPickup::SpawnItemAtDistance(float Distance, UGoKartItemDefinition* ItemDefinition)
{
	// NOOP: Will spawn item actor at specified distance along track
	// Item actor will have hitbox detection for pickup
	return nullptr;
}

void UGoKartItemPickup::OnItemPickedUp(AGoKartItemActor* ItemActor, int32 PlayerID)
{
	// NOOP: Will handle item pickup:
	// - Remove item from world
	// - Add item to player's inventory
	// - Schedule respawn after ItemRespawnTime
	// - Trigger pickup effects (audio, visual)
}

void UGoKartItemPickup::SpawnItemsAlongTrack(AGoKartTrackSpline* TrackSpline)
{
	// NOOP: Will spawn items at regular intervals along track
	// Uses ItemSpawnInterval to determine spacing
	// Randomly selects from ItemDefinitions
}

