// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Examples.h"
#include "HoloCadeServerManagerGameMode.generated.h"

/**
 * Server Manager Game Mode
 * 
 * Special game mode for the HoloCade Server Manager application.
 * Provides a UMG interface for:
 * - Starting/stopping dedicated game servers
 * - Monitoring server status (player count, experience state)
 * - Configuring Omniverse Audio2Face integration
 * - Viewing real-time logs
 * 
 * This runs on the dedicated server PC with a monitor/GUI.
 */
UCLASS()
class EXAMPLES_API AHoloCadeServerManagerGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AHoloCadeServerManagerGameMode();

protected:
	virtual void BeginPlay() override;

public:
	/** Widget class for the server manager UI */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HoloCade|Server Manager")
	TSubclassOf<class UUserWidget> ServerManagerWidgetClass;

	/** Instance of the server manager UI */
	UPROPERTY(BlueprintReadOnly, Category = "HoloCade|Server Manager")
	TObjectPtr<class UUserWidget> ServerManagerWidget;
};



