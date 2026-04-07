// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/WidgetComponent.h"
#include "AIFacemaskLiveActorHUDComponent.generated.h"

// Forward declarations
class UAIFacemaskLiveActorHUD;
class UAIFacemaskScriptManager;
class UAIFacemaskImprovManager;
class UCameraComponent;

/**
 * Live Actor HUD Component
 * 
 * Actor component that creates and manages the stereo VR HUD overlay for live actors.
 * Attaches a UWidgetComponent to the live actor's camera for stereo rendering.
 * 
 * ARCHITECTURE:
 * - Client-only component (visible only to live actor HMD clients)
 * - Creates UWidgetComponent attached to live actor's camera
 * - Widget component renders UAIFacemaskLiveActorHUD widget
 * - Updates HUD display based on ScriptManager and ImprovManager state
 * 
 * USAGE:
 * - Automatically created by AAIFacemaskExperience for live actor pawns
 * - Finds ScriptManager and ImprovManager on the same actor
 * - Subscribes to state change events for real-time updates
 */
UCLASS(ClassGroup=(HoloCade), meta=(BlueprintSpawnableComponent))
class HOLOCADEEXPERIENCES_API UAIFacemaskLiveActorHUDComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAIFacemaskLiveActorHUDComponent();

	/**
	 * Initialize the HUD component
	 * @param InScriptManager - Script manager reference
	 * @param InImprovManager - Improv manager reference
	 * @return true if initialization was successful
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|AIFacemask HUD")
	bool InitializeHUD(UAIFacemaskScriptManager* InScriptManager, UAIFacemaskImprovManager* InImprovManager);

	/**
	 * Update HUD display (called by managers when state changes)
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|AIFacemask HUD")
	void UpdateHUDDisplay();

	/**
	 * Show/Hide the HUD
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|AIFacemask HUD")
	void SetHUDVisible(bool bVisible);

	/**
	 * Check if HUD is visible
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|AIFacemask HUD")
	bool IsHUDVisible() const { return bIsVisible; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	/** Create widget component and attach to camera */
	void CreateWidgetComponent();

	/** Create all widget elements procedurally (called after widget is created) */
	void CreateWidgetElements();

	/** Update text display with proper ordering and state-based colors */
	void UpdateTextDisplay(
		const FString& CurrentImprovResponse,
		bool bImprovResponseSpoken,
		const FString& CurrentNarrativeTargetSentence,
		bool bNarrativeTargetSpoken,
		const FString& BufferedTransitionText,
		bool bTransitionSpoken);

	/** Update arrow button visual feedback */
	void UpdateArrowButtons(bool bForwardPressed, bool bBackwardPressed);

	/** Update state info display */
	void UpdateStateInfo(FName CurrentStateName, int32 CurrentStateIndex);

	/** Find camera component on owner actor */
	UCameraComponent* FindCameraComponent() const;

	/** Get current state from ScriptManager */
	void GetCurrentStateFromScriptManager(FString& OutNarrativeTargetSentence, bool& OutbNarrativeTargetSpoken, FName& OutStateName, int32& OutStateIndex) const;

	/** Get current state from ImprovManager */
	void GetCurrentStateFromImprovManager(FString& OutImprovResponse, bool& OutbImprovResponseSpoken, FString& OutBufferedTransition, bool& OutbTransitionSpoken) const;

	/** Get button press states (from embedded system or VR controllers) */
	void GetButtonPressStates(bool& OutbForwardPressed, bool& OutbBackwardPressed) const;

	/** Widget component attached to camera for stereo rendering */
	UPROPERTY()
	TObjectPtr<UWidgetComponent> WidgetComponent;

	/** HUD widget instance */
	UPROPERTY()
	TObjectPtr<UAIFacemaskLiveActorHUD> HUDWidget;

	/** Reference to Script Manager */
	UPROPERTY()
	TObjectPtr<UAIFacemaskScriptManager> ScriptManager;

	/** Reference to Improv Manager */
	UPROPERTY()
	TObjectPtr<UAIFacemaskImprovManager> ImprovManager;

	/** Whether HUD is visible */
	bool bIsVisible = true;

	/** Whether component is initialized */
	bool bIsInitialized = false;

	/** Widget class to instantiate */
	UPROPERTY(EditAnywhere, Category = "HoloCade|AIFacemask HUD")
	TSubclassOf<UAIFacemaskLiveActorHUD> HUDWidgetClass;

	/** Widget size (width and height in world units) */
	UPROPERTY(EditAnywhere, Category = "HoloCade|AIFacemask HUD", meta = (ClampMin = "0.1", ClampMax = "5.0"))
	FVector2D WidgetSize = FVector2D(1.0f, 0.75f);

public:
	/** Distance from camera/face for widget rendering (in world units) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|AIFacemask HUD", meta = (ClampMin = "0.1", ClampMax = "10.0"))
	float FaceDistance = 2.0f;
};

