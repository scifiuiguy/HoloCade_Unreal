// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "AIFacemask/AIFacemaskLiveActorHUDComponent.h"
#include "AIFacemask/AIFacemaskLiveActorHUD.h"
#include "AIFacemask/AIFacemaskScriptManager.h"
#include "AIFacemask/AIFacemaskImprovManager.h"
#include "AIFacemask/AIFacemaskExperience.h"
#include "Components/WidgetComponent.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/VerticalBox.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/PanelWidget.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Styling/CoreStyle.h"

UAIFacemaskLiveActorHUDComponent::UAIFacemaskLiveActorHUDComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	
	bIsInitialized = false;
	bIsVisible = true;
	
	// Set default widget class (can be overridden in Blueprint)
	HUDWidgetClass = UAIFacemaskLiveActorHUD::StaticClass();
}

void UAIFacemaskLiveActorHUDComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// Only create HUD on client (not on server)
	if (GetWorld()->GetNetMode() == NM_DedicatedServer)
	{
		UE_LOG(LogTemp, Log, TEXT("UAIFacemaskLiveActorHUDComponent: Running on dedicated server, HUD not created"));
		return;
	}

	// Find ScriptManager and ImprovManager on the same actor
	AActor* Owner = GetOwner();
	if (Owner)
	{
		UAIFacemaskScriptManager* FoundScriptManager = Owner->FindComponentByClass<UAIFacemaskScriptManager>();
		UAIFacemaskImprovManager* FoundImprovManager = Owner->FindComponentByClass<UAIFacemaskImprovManager>();
		
		if (FoundScriptManager && FoundImprovManager)
		{
			InitializeHUD(FoundScriptManager, FoundImprovManager);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("UAIFacemaskLiveActorHUDComponent: ScriptManager or ImprovManager not found on owner actor"));
		}
	}
}

void UAIFacemaskLiveActorHUDComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Clean up widget component
	if (WidgetComponent)
	{
		WidgetComponent->DestroyComponent();
		WidgetComponent = nullptr;
	}
	
	Super::EndPlay(EndPlayReason);
}

void UAIFacemaskLiveActorHUDComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsInitialized || !bIsVisible)
	{
		return;
	}

	// Update HUD display every frame (or on-demand via events)
	UpdateHUDDisplay();
}

bool UAIFacemaskLiveActorHUDComponent::InitializeHUD(UAIFacemaskScriptManager* InScriptManager, UAIFacemaskImprovManager* InImprovManager)
{
	if (bIsInitialized)
	{
		UE_LOG(LogTemp, Warning, TEXT("UAIFacemaskLiveActorHUDComponent: Already initialized"));
		return true;
	}

	if (!InScriptManager || !InImprovManager)
	{
		UE_LOG(LogTemp, Error, TEXT("UAIFacemaskLiveActorHUDComponent: Cannot initialize - ScriptManager or ImprovManager is null"));
		return false;
	}

	ScriptManager = InScriptManager;
	ImprovManager = InImprovManager;

	// Create widget component and attach to camera
	CreateWidgetComponent();

	if (WidgetComponent && HUDWidget)
	{
		bIsInitialized = true;
		UE_LOG(LogTemp, Log, TEXT("UAIFacemaskLiveActorHUDComponent: Initialized successfully"));
		return true;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("UAIFacemaskLiveActorHUDComponent: Failed to create widget component or HUD widget"));
		return false;
	}
}

void UAIFacemaskLiveActorHUDComponent::UpdateHUDDisplay()
{
	if (!bIsInitialized || !HUDWidget)
	{
		return;
	}

	// Get current state from managers
	FString NarrativeTargetSentence;
	bool bNarrativeTargetSpoken = false;
	FName CurrentStateName = NAME_None;
	int32 CurrentStateIndex = -1;
	
	GetCurrentStateFromScriptManager(NarrativeTargetSentence, bNarrativeTargetSpoken, CurrentStateName, CurrentStateIndex);

	FString ImprovResponse;
	bool bImprovResponseSpoken = false;
	FString BufferedTransition;
	bool bTransitionSpoken = false;
	GetCurrentStateFromImprovManager(ImprovResponse, bImprovResponseSpoken, BufferedTransition, bTransitionSpoken);

	bool bForwardPressed = false;
	bool bBackwardPressed = false;
	GetButtonPressStates(bForwardPressed, bBackwardPressed);

	// Update HUD display directly (all logic is in component now)
	UpdateTextDisplay(ImprovResponse, bImprovResponseSpoken, NarrativeTargetSentence, bNarrativeTargetSpoken, BufferedTransition, bTransitionSpoken);
	UpdateArrowButtons(bForwardPressed, bBackwardPressed);
	UpdateStateInfo(CurrentStateName, CurrentStateIndex);
}

void UAIFacemaskLiveActorHUDComponent::SetHUDVisible(bool bVisible)
{
	bIsVisible = bVisible;
	
	if (WidgetComponent)
	{
		WidgetComponent->SetVisibility(bVisible);
	}
}

void UAIFacemaskLiveActorHUDComponent::CreateWidgetComponent()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	// Find camera component
	UCameraComponent* CameraComponent = FindCameraComponent();
	if (!CameraComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("UAIFacemaskLiveActorHUDComponent: Cannot create widget - no camera component found"));
		return;
	}

	// Create widget component
	WidgetComponent = NewObject<UWidgetComponent>(Owner, UWidgetComponent::StaticClass(), TEXT("LiveActorHUDWidgetComponent"));
	if (!WidgetComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("UAIFacemaskLiveActorHUDComponent: Failed to create widget component"));
		return;
	}

	// Attach to camera (or root if no camera found)
	if (CameraComponent)
	{
		WidgetComponent->SetupAttachment(CameraComponent);
	}
	else
	{
		WidgetComponent->SetupAttachment(Owner->GetRootComponent());
	}
	
	// Configure for VR HUD rendering
	WidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);  // Screen space for VR HUD (renders in front of camera)
	WidgetComponent->SetDrawSize(WidgetSize);
	WidgetComponent->SetRelativeLocation(FVector(FaceDistance, 0.0f, 0.0f));  // Distance in front of camera/face
	WidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WidgetComponent->SetVisibility(bIsVisible);
	WidgetComponent->SetTwoSided(true);  // Visible from both sides
	WidgetComponent->SetTickWhenOffscreen(true);  // Keep updating even when offscreen

	// Create HUD widget instance
	if (HUDWidgetClass)
	{
		HUDWidget = CreateWidget<UAIFacemaskLiveActorHUD>(GetWorld(), HUDWidgetClass);
		if (HUDWidget)
		{
			WidgetComponent->SetWidget(HUDWidget);
			
			// Create all widget elements procedurally
			CreateWidgetElements();
			
			UE_LOG(LogTemp, Log, TEXT("UAIFacemaskLiveActorHUDComponent: HUD widget created and attached to camera"));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("UAIFacemaskLiveActorHUDComponent: Failed to create HUD widget instance"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("UAIFacemaskLiveActorHUDComponent: HUDWidgetClass not set"));
	}
}

UCameraComponent* UAIFacemaskLiveActorHUDComponent::FindCameraComponent() const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	// Try to find camera component on owner
	UCameraComponent* CameraComponent = Owner->FindComponentByClass<UCameraComponent>();
	if (CameraComponent)
	{
		return CameraComponent;
	}

	// Try to find camera in pawn's components
	APawn* OwnerPawn = Cast<APawn>(Owner);
	if (OwnerPawn)
	{
		// Check if pawn has a camera component
		TArray<UCameraComponent*> CameraComponents;
		OwnerPawn->GetComponents<UCameraComponent>(CameraComponents);
		if (CameraComponents.Num() > 0)
		{
			return CameraComponents[0];
		}

		// Try to get camera from player controller
		if (APlayerController* PC = OwnerPawn->GetController<APlayerController>())
		{
			// Player controller's camera is typically the HMD camera in VR
			// We'll attach to the pawn's root component as fallback
		}
	}

	// Fallback: attach to root component
	UE_LOG(LogTemp, Warning, TEXT("UAIFacemaskLiveActorHUDComponent: No camera component found, attaching to root component"));
	return nullptr;
}

void UAIFacemaskLiveActorHUDComponent::GetCurrentStateFromScriptManager(
	FString& OutNarrativeTargetSentence,
	bool& OutbNarrativeTargetSpoken,
	FName& OutStateName,
	int32& OutStateIndex) const
{
	if (!ScriptManager)
	{
		return;
	}

	// Get current narrative state from experience base
	AActor* Owner = GetOwner();
	if (AAIFacemaskExperience* Experience = Cast<AAIFacemaskExperience>(Owner))
	{
		OutStateName = Experience->GetCurrentExperienceState();
		// TODO: Get state index from experience base
		OutStateIndex = 0;  // Placeholder
	}

	// Get current script and its target sentence
	if (ScriptManager->CurrentScript.ScriptLines.Num() > 0)
	{
		int32 CurrentLineIndex = ScriptManager->CurrentScriptLineIndex;
		if (CurrentLineIndex >= 0 && CurrentLineIndex < ScriptManager->CurrentScript.ScriptLines.Num())
		{
			const FAIFacemaskScriptLine& CurrentLine = ScriptManager->CurrentScript.ScriptLines[CurrentLineIndex];
			OutNarrativeTargetSentence = CurrentLine.TextPrompt;
			// Get spoken state from server (tracks completion state for HUD)
			OutbNarrativeTargetSpoken = CurrentLine.bHasBeenSpoken;
		}
	}
}

void UAIFacemaskLiveActorHUDComponent::GetCurrentStateFromImprovManager(
	FString& OutImprovResponse,
	bool& OutbImprovResponseSpoken,
	FString& OutBufferedTransition,
	bool& OutbTransitionSpoken) const
{
	if (!ImprovManager)
	{
		return;
	}

	// Get current improv response and its state (from server)
	if (ImprovManager->IsGeneratingResponse())
	{
		OutImprovResponse = ImprovManager->GetCurrentAIResponse();
		// Get usage state from server (queued → spoken when face starts)
		OutbImprovResponseSpoken = (ImprovManager->GetCurrentAIResponseState() == EImprovResponseState::Spoken);
	}

	// TODO: Get buffered transition and its state (Phase 11)
	OutBufferedTransition = TEXT("");  // Placeholder - will be implemented in Phase 11
	OutbTransitionSpoken = false;  // Placeholder - will be implemented in Phase 11
}

void UAIFacemaskLiveActorHUDComponent::GetButtonPressStates(
	bool& OutbForwardPressed,
	bool& OutbBackwardPressed) const
{
	// Get button states from embedded system or VR controllers
	AActor* Owner = GetOwner();
	if (AAIFacemaskExperience* Experience = Cast<AAIFacemaskExperience>(Owner))
	{
		// TODO: Query embedded system controller for button states
		// For now, return false (no buttons pressed)
		OutbForwardPressed = false;
		OutbBackwardPressed = false;
	}
}

void UAIFacemaskLiveActorHUDComponent::CreateWidgetElements()
{
	if (!HUDWidget)
	{
		UE_LOG(LogTemp, Error, TEXT("UAIFacemaskLiveActorHUDComponent: Cannot create widget elements - HUDWidget is null"));
		return;
	}

	// Ensure widget is constructed (creates root widget if needed)
	if (!HUDWidget->IsConstructed())
	{
		HUDWidget->Construct();
	}

	// Get or create root canvas panel
	UWidget* ExistingRoot = HUDWidget->GetRootWidget();
	
	// If root is already a canvas panel, use it
	if (UCanvasPanel* ExistingCanvas = Cast<UCanvasPanel>(ExistingRoot))
	{
		HUDWidget->RootPanel = ExistingCanvas;
	}
	else
	{
		// Create a new canvas panel
		HUDWidget->RootPanel = NewObject<UCanvasPanel>(HUDWidget, UCanvasPanel::StaticClass());
		if (HUDWidget->RootPanel && ExistingRoot)
		{
			// Try to add canvas panel to existing root (if root is a panel type)
			if (UPanelWidget* RootPanelWidget = Cast<UPanelWidget>(ExistingRoot))
			{
				RootPanelWidget->AddChild(HUDWidget->RootPanel);
			}
			else
			{
				// If root is not a panel, we'll work with the existing root
				// Use the existing root as our panel (cast might fail, but we'll handle it)
				HUDWidget->RootPanel = Cast<UCanvasPanel>(ExistingRoot);
			}
		}
	}

	if (!HUDWidget->RootPanel)
	{
		UE_LOG(LogTemp, Error, TEXT("UAIFacemaskLiveActorHUDComponent: Failed to create or find root canvas panel"));
		return;
	}

	// Create vertical box for text content
	HUDWidget->TextContentPanel = NewObject<UVerticalBox>(HUDWidget, UVerticalBox::StaticClass());
	if (HUDWidget->TextContentPanel)
	{
		UCanvasPanelSlot* ContentSlot = HUDWidget->RootPanel->AddChildToCanvas(HUDWidget->TextContentPanel);
		if (ContentSlot)
		{
			ContentSlot->SetAnchors(FAnchors(0.5f, 0.0f, 0.5f, 1.0f));  // Center horizontally, full height
			ContentSlot->SetAlignment(FVector2D(0.5f, 0.0f));  // Center horizontally, top aligned
			ContentSlot->SetOffsets(FMargin(0.0f, 20.0f, 0.0f, 20.0f));  // Padding
		}
	}

	// Create text blocks
	HUDWidget->NarrativeTargetTextBlock = NewObject<UTextBlock>(HUDWidget, UTextBlock::StaticClass());
	HUDWidget->ImprovResponseTextBlock = NewObject<UTextBlock>(HUDWidget, UTextBlock::StaticClass());
	HUDWidget->TransitionTextBlock = NewObject<UTextBlock>(HUDWidget, UTextBlock::StaticClass());
	HUDWidget->StateInfoTextBlock = NewObject<UTextBlock>(HUDWidget, UTextBlock::StaticClass());

	// Configure text blocks
	if (HUDWidget->NarrativeTargetTextBlock)
	{
		HUDWidget->NarrativeTargetTextBlock->SetText(FText::FromString(TEXT("Narrative Target")));
		HUDWidget->NarrativeTargetTextBlock->SetColorAndOpacity(HUDWidget->QueuedTextColor);
		HUDWidget->TextContentPanel->AddChild(HUDWidget->NarrativeTargetTextBlock);
	}

	if (HUDWidget->ImprovResponseTextBlock)
	{
		HUDWidget->ImprovResponseTextBlock->SetText(FText::FromString(TEXT("Improv Response")));
		HUDWidget->ImprovResponseTextBlock->SetColorAndOpacity(HUDWidget->QueuedTextColor);
		HUDWidget->TextContentPanel->AddChild(HUDWidget->ImprovResponseTextBlock);
	}

	if (HUDWidget->TransitionTextBlock)
	{
		HUDWidget->TransitionTextBlock->SetText(FText::FromString(TEXT("Transition")));
		HUDWidget->TransitionTextBlock->SetColorAndOpacity(HUDWidget->QueuedTextColor);
		HUDWidget->TextContentPanel->AddChild(HUDWidget->TransitionTextBlock);
	}

	if (HUDWidget->StateInfoTextBlock)
	{
		HUDWidget->StateInfoTextBlock->SetText(FText::FromString(TEXT("State: None")));
		HUDWidget->StateInfoTextBlock->SetColorAndOpacity(FLinearColor::White);
		HUDWidget->TextContentPanel->AddChild(HUDWidget->StateInfoTextBlock);
	}

	// Create arrow button images
	HUDWidget->ForwardArrowImage = NewObject<UImage>(HUDWidget, UImage::StaticClass());
	HUDWidget->BackwardArrowImage = NewObject<UImage>(HUDWidget, UImage::StaticClass());

	// Add arrows to canvas (positioned at bottom)
	if (HUDWidget->ForwardArrowImage)
	{
		UCanvasPanelSlot* ForwardSlot = HUDWidget->RootPanel->AddChildToCanvas(HUDWidget->ForwardArrowImage);
		if (ForwardSlot)
		{
			ForwardSlot->SetAnchors(FAnchors(0.5f, 1.0f, 0.5f, 1.0f));  // Center horizontally, bottom
			ForwardSlot->SetAlignment(FVector2D(0.5f, 1.0f));
			ForwardSlot->SetOffsets(FMargin(-50.0f, -50.0f, 0.0f, 10.0f));  // Left of center, 10px from bottom
			ForwardSlot->SetSize(FVector2D(40.0f, 40.0f));
		}
		// Set default arrow color (gray when not pressed)
		HUDWidget->ForwardArrowImage->SetColorAndOpacity(FLinearColor(0.5f, 0.5f, 0.5f, 1.0f));
	}

	if (HUDWidget->BackwardArrowImage)
	{
		UCanvasPanelSlot* BackwardSlot = HUDWidget->RootPanel->AddChildToCanvas(HUDWidget->BackwardArrowImage);
		if (BackwardSlot)
		{
			BackwardSlot->SetAnchors(FAnchors(0.5f, 1.0f, 0.5f, 1.0f));  // Center horizontally, bottom
			BackwardSlot->SetAlignment(FVector2D(0.5f, 1.0f));
			BackwardSlot->SetOffsets(FMargin(10.0f, -50.0f, 50.0f, 10.0f));  // Right of center, 10px from bottom
			BackwardSlot->SetSize(FVector2D(40.0f, 40.0f));
		}
		// Set default arrow color (gray when not pressed)
		HUDWidget->BackwardArrowImage->SetColorAndOpacity(FLinearColor(0.5f, 0.5f, 0.5f, 1.0f));
	}

	UE_LOG(LogTemp, Log, TEXT("UAIFacemaskLiveActorHUDComponent: Widget elements created successfully"));
}

void UAIFacemaskLiveActorHUDComponent::UpdateTextDisplay(
	const FString& CurrentImprovResponse,
	bool bImprovResponseSpoken,
	const FString& CurrentNarrativeTargetSentence,
	bool bNarrativeTargetSpoken,
	const FString& BufferedTransitionText,
	bool bTransitionSpoken)
{
	if (!HUDWidget)
	{
		return;
	}

	// Update narrative target text (top priority - always shown if present)
	if (HUDWidget->NarrativeTargetTextBlock)
	{
		if (!CurrentNarrativeTargetSentence.IsEmpty())
		{
			HUDWidget->NarrativeTargetTextBlock->SetText(FText::FromString(CurrentNarrativeTargetSentence));
			FLinearColor TextColor = bNarrativeTargetSpoken ? HUDWidget->SpokenTextColor : HUDWidget->QueuedTextColor;
			HUDWidget->NarrativeTargetTextBlock->SetColorAndOpacity(TextColor);
			HUDWidget->NarrativeTargetTextBlock->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			HUDWidget->NarrativeTargetTextBlock->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// Update improv response text (second priority)
	if (HUDWidget->ImprovResponseTextBlock)
	{
		if (!CurrentImprovResponse.IsEmpty())
		{
			HUDWidget->ImprovResponseTextBlock->SetText(FText::FromString(CurrentImprovResponse));
			FLinearColor TextColor = bImprovResponseSpoken ? HUDWidget->SpokenTextColor : HUDWidget->QueuedTextColor;
			HUDWidget->ImprovResponseTextBlock->SetColorAndOpacity(TextColor);
			HUDWidget->ImprovResponseTextBlock->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			HUDWidget->ImprovResponseTextBlock->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// Update transition text (third priority)
	if (HUDWidget->TransitionTextBlock)
	{
		if (!BufferedTransitionText.IsEmpty())
		{
			HUDWidget->TransitionTextBlock->SetText(FText::FromString(BufferedTransitionText));
			FLinearColor TextColor = bTransitionSpoken ? HUDWidget->SpokenTextColor : HUDWidget->QueuedTextColor;
			HUDWidget->TransitionTextBlock->SetColorAndOpacity(TextColor);
			HUDWidget->TransitionTextBlock->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			HUDWidget->TransitionTextBlock->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UAIFacemaskLiveActorHUDComponent::UpdateArrowButtons(bool bForwardPressed, bool bBackwardPressed)
{
	if (!HUDWidget)
	{
		return;
	}

	// Update forward arrow visual feedback
	if (HUDWidget->ForwardArrowImage)
	{
		FLinearColor ArrowColor = bForwardPressed ? FLinearColor::White : FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);
		HUDWidget->ForwardArrowImage->SetColorAndOpacity(ArrowColor);
	}

	// Update backward arrow visual feedback
	if (HUDWidget->BackwardArrowImage)
	{
		FLinearColor ArrowColor = bBackwardPressed ? FLinearColor::White : FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);
		HUDWidget->BackwardArrowImage->SetColorAndOpacity(ArrowColor);
	}
}

void UAIFacemaskLiveActorHUDComponent::UpdateStateInfo(FName CurrentStateName, int32 CurrentStateIndex)
{
	if (!HUDWidget || !HUDWidget->StateInfoTextBlock)
	{
		return;
	}

	// Format state info string
	FString StateInfoString = FString::Printf(TEXT("State: %s (%d)"), *CurrentStateName.ToString(), CurrentStateIndex);
	HUDWidget->StateInfoTextBlock->SetText(FText::FromString(StateInfoString));
}

