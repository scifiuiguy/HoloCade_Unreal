// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AIFacemaskLiveActorHUD.generated.h"

// Forward declarations
class UTextBlock;
class UImage;
class UVerticalBox;
class UCanvasPanel;

/**
 * Minimal HUD Widget Container
 * 
 * This is a minimal widget class required by UWidgetComponent.
 * All UI creation and update logic is handled by UAIFacemaskLiveActorHUDComponent.
 * 
 * The component creates all widget elements procedurally and manages them directly.
 */
UCLASS()
class HOLOCADEEXPERIENCES_API UAIFacemaskLiveActorHUD : public UUserWidget
{
	GENERATED_BODY()

public:
	// Public members for component to access directly
	UPROPERTY()
	class UTextBlock* NarrativeTargetTextBlock;

	UPROPERTY()
	class UTextBlock* ImprovResponseTextBlock;

	UPROPERTY()
	class UTextBlock* TransitionTextBlock;

	UPROPERTY()
	class UTextBlock* StateInfoTextBlock;

	UPROPERTY()
	class UImage* ForwardArrowImage;

	UPROPERTY()
	class UImage* BackwardArrowImage;

	UPROPERTY()
	class UVerticalBox* TextContentPanel;

	UPROPERTY()
	class UCanvasPanel* RootPanel;

	// Color properties (component will set these)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|AIFacemask HUD|Colors")
	FLinearColor QueuedTextColor = FLinearColor(0.5f, 0.5f, 1.0f, 1.0f);  // Light blue for queued

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|AIFacemask HUD|Colors")
	FLinearColor SpokenTextColor = FLinearColor(0.7f, 0.7f, 0.7f, 1.0f);  // Gray for spoken
};
