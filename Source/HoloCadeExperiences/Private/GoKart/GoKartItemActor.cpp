// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "GoKart/GoKartItemActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "GoKart/GoKartItemPickup.h"
#include "HoloCadeExperiences.h"

AGoKartItemActor::AGoKartItemActor()
{
	PrimaryActorTick.bCanEverTick = false;

	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
	RootComponent = PickupMesh;

	PickupHitbox = CreateDefaultSubobject<USphereComponent>(TEXT("PickupHitbox"));
	PickupHitbox->SetupAttachment(RootComponent);
	PickupHitbox->SetSphereRadius(50.0f); // 50cm pickup radius
	PickupHitbox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PickupHitbox->SetCollisionObjectType(ECollisionChannel::ECC_GameTraceChannel1); // Custom channel for items
	PickupHitbox->SetCollisionResponseToAllChannels(ECR_Ignore);
	PickupHitbox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	bIsPickedUp = false;
	TrackDistance = 0.0f;
}

void AGoKartItemActor::BeginPlay()
{
	Super::BeginPlay();

	// Setup overlap detection
	if (PickupHitbox)
	{
		PickupHitbox->OnComponentBeginOverlap.AddDynamic(this, &AGoKartItemActor::OnPickupHitboxOverlap);
	}
}

void AGoKartItemActor::InitializeFromDefinition(UGoKartItemDefinition* Definition)
{
	// NOOP: Will load and set pickup mesh from PickupAsset
	// Will configure projectile properties from definition
	ItemDefinition = Definition;
}

void AGoKartItemActor::OnPickedUp(int32 PlayerID)
{
	if (bIsPickedUp)
	{
		return;
	}

	bIsPickedUp = true;

	// Hide pickup mesh
	if (PickupMesh)
	{
		PickupMesh->SetVisibility(false);
		PickupMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// Disable hitbox
	if (PickupHitbox)
	{
		PickupHitbox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// Notify item pickup system
	// NOOP: Will find GoKartItemPickup component and notify it

	// Schedule respawn
	GetWorld()->GetTimerManager().SetTimer(RespawnTimerHandle, this, &AGoKartItemActor::Respawn, 10.0f, false);
}

void AGoKartItemActor::Respawn()
{
	bIsPickedUp = false;

	// Show pickup mesh
	if (PickupMesh)
	{
		PickupMesh->SetVisibility(true);
		PickupMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}

	// Enable hitbox
	if (PickupHitbox)
	{
		PickupHitbox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
}

void AGoKartItemActor::OnPickupHitboxOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// NOOP: Will detect player overlap and trigger pickup
	// Check if OtherActor is a player/kart
	// Call OnPickedUp with player ID
}

