// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "HoloCadeServerManagerGameMode.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"

AHoloCadeServerManagerGameMode::AHoloCadeServerManagerGameMode()
{
	// Set default pawn to none (we don't need a player character for server manager)
	DefaultPawnClass = nullptr;
}

void AHoloCadeServerManagerGameMode::BeginPlay()
{
	Super::BeginPlay();

	// Create and display the server manager UI
	if (ServerManagerWidgetClass)
	{
		APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
		if (PC)
		{
			ServerManagerWidget = CreateWidget<UUserWidget>(PC, ServerManagerWidgetClass);
			if (ServerManagerWidget)
			{
				ServerManagerWidget->AddToViewport();
				
				// Set input mode to UI only
				PC->SetInputMode(FInputModeUIOnly());
				PC->bShowMouseCursor = true;

				UE_LOG(LogTemp, Log, TEXT("HoloCadeServerManager: UI initialized"));
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("HoloCadeServerManager: ServerManagerWidgetClass not set!"));
	}
}

