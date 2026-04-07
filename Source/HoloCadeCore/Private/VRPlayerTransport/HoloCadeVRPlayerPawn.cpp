// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "VRPlayerTransport/HoloCadeVRPlayerPawn.h"
#include "VRPlayerTransport/VRPlayerReplicationComponent.h"
#include "HoloCadeHandGestureRecognizer.h"

AHoloCadeVRPlayerPawn::AHoloCadeVRPlayerPawn(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Create VR replication component
	VRReplicationComponent = CreateDefaultSubobject<UHoloCadeVRPlayerReplicationComponent>(TEXT("VRReplicationComponent"));
	
	bAutoCreateHandGestureRecognizer = false;
}

void AHoloCadeVRPlayerPawn::BeginPlay()
{
	Super::BeginPlay();

	// Optionally create hand gesture recognizer if requested
	if (bAutoCreateHandGestureRecognizer)
	{
		if (!GetHandGestureRecognizer())
		{
			UHoloCadeHandGestureRecognizer* GestureRecognizer = NewObject<UHoloCadeHandGestureRecognizer>(this, TEXT("HandGestureRecognizer"));
			if (GestureRecognizer)
			{
				GestureRecognizer->RegisterComponent();
				
				// Auto-initialize if we have a player controller
				if (APlayerController* PC = GetController<APlayerController>())
				{
					GestureRecognizer->InitializeRecognizer(PC);
				}
			}
		}
	}
}

UHoloCadeHandGestureRecognizer* AHoloCadeVRPlayerPawn::GetHandGestureRecognizer() const
{
	return FindComponentByClass<UHoloCadeHandGestureRecognizer>();
}

