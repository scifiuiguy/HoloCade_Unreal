// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "GoKart/GoKartBarrierActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "HoloCadeExperiences.h"

AGoKartBarrierActor::AGoKartBarrierActor()
{
	PrimaryActorTick.bCanEverTick = false;

	BarrierMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BarrierMesh"));
	RootComponent = BarrierMesh;
	BarrierMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	BarrierMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	BarrierMesh->SetVisibility(false); // Never visible to players (passthrough/AR)

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->SetupAttachment(RootComponent);
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionBox->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);

	bShowDebugVisualization = false;
}

void AGoKartBarrierActor::BeginPlay()
{
	Super::BeginPlay();
	// NOOP: Debug visualization setup if bShowDebugVisualization is true
}

void AGoKartBarrierActor::InitializeBarrier(const FVector& Location, const FRotator& Rotation, float Width, float Height)
{
	// NOOP: Will set barrier mesh size and position
	// Create vertical planar mesh at specified location/rotation
	// Set collision box to match mesh bounds
	SetActorLocation(Location);
	SetActorRotation(Rotation);
}

