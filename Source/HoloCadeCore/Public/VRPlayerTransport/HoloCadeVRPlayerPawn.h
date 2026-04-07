// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "HoloCadeVRPlayerPawn.generated.h"

// Forward declarations
class UHoloCadeVRPlayerReplicationComponent;
class UHoloCadeHandGestureRecognizer;

/**
 * AHoloCadeVRPlayerPawn
 * 
 * Base pawn class for VR players in HoloCade experiences.
 * Automatically includes VR replication component for multiplayer hand/HMD tracking.
 * 
 * This is an optional convenience class - you can also add UHoloCadeVRPlayerReplicationComponent
 * to any existing pawn class if you prefer.
 * 
 * Usage:
 * 1. Create a Blueprint child of this pawn class
 * 2. Add your VR player mesh/representation
 * 3. The replication component is automatically included
 * 4. Add UHoloCadeHandGestureRecognizer if you need gesture recognition
 * 
 * The pawn will automatically replicate HMD and hand tracking data to all clients.
 */
UCLASS(Blueprintable, ClassGroup=(HoloCade))
class HOLOCADECORE_API AHoloCadeVRPlayerPawn : public APawn
{
	GENERATED_BODY()

public:
	AHoloCadeVRPlayerPawn(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;

	/**
	 * Get the VR replication component
	 * @return The VR replication component (nullptr if not found)
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|VRPlayer")
	UHoloCadeVRPlayerReplicationComponent* GetVRReplicationComponent() const { return VRReplicationComponent; }

	/**
	 * Get the hand gesture recognizer component (if added)
	 * @return The hand gesture recognizer component (nullptr if not found)
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|VRPlayer")
	UHoloCadeHandGestureRecognizer* GetHandGestureRecognizer() const;

protected:
	/** VR replication component - automatically replicates HMD and hand tracking data */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HoloCade|VRPlayer|Components")
	TObjectPtr<UHoloCadeVRPlayerReplicationComponent> VRReplicationComponent;

	/** Whether to automatically create a hand gesture recognizer component */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|VRPlayer|Config")
	bool bAutoCreateHandGestureRecognizer = false;
};

