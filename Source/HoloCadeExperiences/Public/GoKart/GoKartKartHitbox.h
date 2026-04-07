// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "GoKartKartHitbox.generated.h"

// Forward declarations
class UStaticMeshComponent;

/**
 * GoKart Kart Hitbox Actor
 * 
 * Blueprint-friendly actor representing kart collision hitbox.
 * Used for:
 * - Projectile collision detection
 * - Kart-to-kart collision detection (audio, particle, throttle effects)
 * - Real-world physics handles most kart collision, but we need hitboxes for game events
 * 
 * This is a separate actor that can be attached to the kart or positioned manually.
 * Blueprint can extend this for custom hitbox shapes and behaviors.
 */
UCLASS(BlueprintType, Blueprintable, ClassGroup=(HoloCade))
class HOLOCADEEXPERIENCES_API AGoKartKartHitbox : public AActor
{
	GENERATED_BODY()
	
public:
	AGoKartKartHitbox();

	/** Hitbox collision component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GoKart|Kart|Hitbox")
	TObjectPtr<UBoxComponent> HitboxCollision;

	/** Debug visualization mesh (optional) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GoKart|Kart|Hitbox|Debug")
	TObjectPtr<UStaticMeshComponent> DebugMesh;

	/** Player/Kart ID this hitbox belongs to */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GoKart|Kart|Hitbox")
	int32 KartID = INDEX_NONE;

	/** Whether to show debug visualization */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GoKart|Kart|Hitbox|Debug")
	bool bShowDebugVisualization = false;

	/**
	 * Handle collision with another kart
	 * @param OtherKart - Other kart that was hit
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "GoKart|Kart|Hitbox")
	void OnKartCollision(AGoKartKartHitbox* OtherKart);

	/**
	 * Handle collision with projectile
	 * @param Projectile - Projectile that hit this kart
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "GoKart|Kart|Hitbox")
	void OnProjectileHit(class AGoKartProjectileActor* Projectile);

protected:
	virtual void BeginPlay() override;

	/** Handle overlap with another kart */
	UFUNCTION()
	void OnHitboxOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** Handle hit by projectile */
	UFUNCTION()
	void OnHitboxHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
};

