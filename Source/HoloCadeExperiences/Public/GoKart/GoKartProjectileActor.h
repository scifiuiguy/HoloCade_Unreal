// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SphereComponent.h"
#include "GoKart/Models/GoKartItemDefinition.h"
#include "GoKartProjectileActor.generated.h"

// Forward declarations
class UStaticMeshComponent;
class UGoKartBarrierSystem;

/**
 * GoKart Projectile Actor
 * 
 * Represents a projectile fired by a player.
 * Uses Unreal's physics system (ProjectileMovementComponent + RigidBody simulation).
 * 
 * Supports hitbox detection for collision with barriers, karts, and other projectiles.
 * 
 * Blueprint-friendly for level design.
 */
UCLASS(BlueprintType, ClassGroup=(HoloCade))
class HOLOCADEEXPERIENCES_API AGoKartProjectileActor : public AActor
{
	GENERATED_BODY()
	
public:
	AGoKartProjectileActor();

	/** Projectile mesh component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GoKart|Projectile")
	TObjectPtr<UStaticMeshComponent> ProjectileMesh;

	/** Projectile collision sphere (hitbox) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GoKart|Projectile")
	TObjectPtr<USphereComponent> ProjectileHitbox;

	/** Projectile movement component (physics simulation) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GoKart|Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	/** Item definition this projectile represents */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GoKart|Projectile")
	TObjectPtr<UGoKartItemDefinition> ItemDefinition;

	/** Player ID who fired this projectile */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GoKart|Projectile")
	int32 FiredByPlayerID = INDEX_NONE;

	/**
	 * Initialize projectile from item definition
	 * @param Definition - Item definition to use
	 * @param StartLocation - Starting location
	 * @param StartVelocity - Initial velocity vector
	 * @param PlayerID - ID of player who fired it
	 */
	UFUNCTION(BlueprintCallable, Category = "GoKart|Projectile")
	void InitializeProjectile(UGoKartItemDefinition* Definition, const FVector& StartLocation, const FVector& StartVelocity, int32 PlayerID);

	/**
	 * Handle collision with barrier (bounce)
	 * @param HitLocation - Location where barrier was hit
	 * @param HitNormal - Normal of barrier surface
	 */
	UFUNCTION(BlueprintCallable, Category = "GoKart|Projectile")
	void OnBarrierHit(const FVector& HitLocation, const FVector& HitNormal);

	/**
	 * Handle collision with kart
	 * @param HitKart - Kart that was hit
	 */
	UFUNCTION(BlueprintCallable, Category = "GoKart|Projectile")
	void OnKartHit(AActor* HitKart);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	/** Handle overlap with kart hitbox */
	UFUNCTION()
	void OnProjectileHitboxOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** Handle collision with barrier */
	UFUNCTION()
	void OnProjectileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

private:
	/** Lifetime timer */
	float LifetimeTimer = 0.0f;

	/** Maximum lifetime in seconds */
	float MaxLifetime = 5.0f;
};

