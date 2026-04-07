// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "GoKart/GoKartProjectileActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GoKart/GoKartBarrierSystem.h"
#include "HoloCadeExperiences.h"

AGoKartProjectileActor::AGoKartProjectileActor()
{
	PrimaryActorTick.bCanEverTick = true;

	ProjectileHitbox = CreateDefaultSubobject<USphereComponent>(TEXT("ProjectileHitbox"));
	RootComponent = ProjectileHitbox;
	ProjectileHitbox->SetSphereRadius(10.0f); // 10cm radius
	ProjectileHitbox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ProjectileHitbox->SetCollisionObjectType(ECollisionChannel::ECC_GameTraceChannel2); // Custom channel for projectiles
	ProjectileHitbox->SetCollisionResponseToAllChannels(ECR_Block);
	ProjectileHitbox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	ProjectileMesh->SetupAttachment(RootComponent);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = ProjectileHitbox;
	ProjectileMovement->InitialSpeed = 2000.0f; // 20 m/s default
	ProjectileMovement->MaxSpeed = 5000.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = true;
	ProjectileMovement->Bounciness = 0.8f;
	ProjectileMovement->Friction = 0.1f;

	FiredByPlayerID = INDEX_NONE;
	MaxLifetime = 5.0f;
	LifetimeTimer = 0.0f;
}

void AGoKartProjectileActor::BeginPlay()
{
	Super::BeginPlay();

	// Setup collision callbacks
	if (ProjectileHitbox)
	{
		ProjectileHitbox->OnComponentBeginOverlap.AddDynamic(this, &AGoKartProjectileActor::OnProjectileHitboxOverlap);
		ProjectileHitbox->OnComponentHit.AddDynamic(this, &AGoKartProjectileActor::OnProjectileHit);
	}
}

void AGoKartProjectileActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Check lifetime
	LifetimeTimer += DeltaTime;
	if (LifetimeTimer >= MaxLifetime)
	{
		Destroy();
	}
}

void AGoKartProjectileActor::InitializeProjectile(UGoKartItemDefinition* Definition, const FVector& StartLocation, const FVector& StartVelocity, int32 PlayerID)
{
	// NOOP: Will initialize projectile from definition:
	// - Load projectile mesh from ProjectileAsset
	// - Set projectile speed from definition
	// - Set lifetime from definition
	// - Set initial velocity
	// - Set fired by player ID

	ItemDefinition = Definition;
	FiredByPlayerID = PlayerID;
	SetActorLocation(StartLocation);

	if (ProjectileMovement)
	{
		ProjectileMovement->Velocity = StartVelocity;
		if (Definition)
		{
			ProjectileMovement->InitialSpeed = Definition->ProjectileSpeed * 100.0f; // Convert m/s to cm/s
			MaxLifetime = Definition->ProjectileLifetime;
		}
	}
}

void AGoKartProjectileActor::OnBarrierHit(const FVector& HitLocation, const FVector& HitNormal)
{
	// NOOP: Will handle barrier bounce:
	// - Reflect velocity based on hit normal
	// - Play bounce sound/particle effect
	// - Reduce velocity slightly (energy loss)

	if (ProjectileMovement)
	{
		FVector CurrentVelocity = ProjectileMovement->Velocity;
		FVector ReflectedVelocity = CurrentVelocity - 2.0f * FVector::DotProduct(CurrentVelocity, HitNormal) * HitNormal;
		ProjectileMovement->Velocity = ReflectedVelocity * 0.8f; // 80% energy retention
	}
}

void AGoKartProjectileActor::OnKartHit(AActor* HitKart)
{
	// NOOP: Will handle kart hit:
	// - Apply damage if ItemDefinition has damage value
	// - Play hit sound/particle effect
	// - Destroy projectile
	// - Trigger throttle effect on hit kart (if applicable)

	Destroy();
}

void AGoKartProjectileActor::OnProjectileHitboxOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// NOOP: Will detect overlap with kart hitbox
	// Check if OtherActor is a kart
	// Call OnKartHit
}

void AGoKartProjectileActor::OnProjectileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// NOOP: Will detect collision with barriers
	// Check if OtherActor is a barrier
	// Call OnBarrierHit with hit location and normal
}

