// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GoKartItemDefinition.generated.h"

/**
 * GoKart item definition
 *
 * Blueprint-friendly data asset defining an item type with pickup, display, and projectile assets.
 * Used by level designers to configure items in the experience.
 */
UCLASS(BlueprintType)
class HOLOCADEEXPERIENCES_API UGoKartItemDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Unique item ID */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GoKart|Item")
	int32 ItemID = INDEX_NONE;

	/** Item name for debugging/UI */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GoKart|Item")
	FString ItemName = TEXT("Unnamed Item");

	/** Pickup asset (static mesh or blueprint class for item pickup representation) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GoKart|Item")
	TSoftObjectPtr<UObject> PickupAsset;

	/** Display asset (static mesh or blueprint class for item display when held) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GoKart|Item")
	TSoftObjectPtr<UObject> DisplayAsset;

	/** Projectile asset (static mesh or blueprint class for projectile representation) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GoKart|Item")
	TSoftObjectPtr<UObject> ProjectileAsset;

	/** Projectile speed in m/s */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GoKart|Item|Projectile", meta = (ClampMin = "1.0"))
	float ProjectileSpeed = 20.0f;

	/** Projectile lifetime in seconds */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GoKart|Item|Projectile", meta = (ClampMin = "0.1"))
	float ProjectileLifetime = 5.0f;

	/** Projectile damage value */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GoKart|Item|Projectile")
	float ProjectileDamage = 1.0f;

	/** Whether this item can be used as a shield (held behind kart) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GoKart|Item")
	bool bCanBeShield = false;

	UGoKartItemDefinition()
	{
		ItemID = INDEX_NONE;
		ItemName = TEXT("Unnamed Item");
		PickupAsset = nullptr;
		DisplayAsset = nullptr;
		ProjectileAsset = nullptr;
		ProjectileSpeed = 20.0f;
		ProjectileLifetime = 5.0f;
		ProjectileDamage = 1.0f;
		bCanBeShield = false;
	}
};

