// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "GoKart/GoKartKartHitbox.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GoKart/GoKartProjectileActor.h"
#include "HoloCadeExperiences.h"

AGoKartKartHitbox::AGoKartKartHitbox()
{
	PrimaryActorTick.bCanEverTick = false;

	HitboxCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("HitboxCollision"));
	RootComponent = HitboxCollision;
	HitboxCollision->SetBoxExtent(FVector(50.0f, 100.0f, 50.0f)); // Default kart size (1m x 2m x 1m)
	HitboxCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HitboxCollision->SetCollisionObjectType(ECollisionChannel::ECC_Pawn);
	HitboxCollision->SetCollisionResponseToAllChannels(ECR_Block);
	HitboxCollision->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Block); // Projectiles

	DebugMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DebugMesh"));
	DebugMesh->SetupAttachment(RootComponent);
	DebugMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DebugMesh->SetVisibility(false);

	KartID = INDEX_NONE;
	bShowDebugVisualization = false;
}

void AGoKartKartHitbox::BeginPlay()
{
	Super::BeginPlay();

	// Setup collision callbacks
	if (HitboxCollision)
	{
		HitboxCollision->OnComponentBeginOverlap.AddDynamic(this, &AGoKartKartHitbox::OnHitboxOverlap);
		HitboxCollision->OnComponentHit.AddDynamic(this, &AGoKartKartHitbox::OnHitboxHit);
	}

	// Show debug mesh if enabled
	if (DebugMesh && bShowDebugVisualization)
	{
		DebugMesh->SetVisibility(true);
	}
}

// Note: OnKartCollision and OnProjectileHit are BlueprintImplementableEvent
// They don't need _Implementation - Blueprint will implement them

void AGoKartKartHitbox::OnHitboxOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// NOOP: Will detect overlap with other kart hitboxes
	// Check if OtherActor is AGoKartKartHitbox
	// Call OnKartCollision
}

void AGoKartKartHitbox::OnHitboxHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// NOOP: Will detect hit by projectile
	// Check if OtherActor is AGoKartProjectileActor
	// Call OnProjectileHit
}

