// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RF433MHzTypes.h"
#include "I433MHzReceiver.h"
#include "RF433MHzReceiver.generated.h"

// Forward declarations
class I433MHzReceiver;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRF433MHzButtonPressed, int32, ButtonCode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRF433MHzButtonReleased, int32, ButtonCode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRF433MHzButtonEvent, int32, ButtonCode, bool, bPressed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRF433MHzCodeLearned, int32, ButtonCode, int32, RollingCode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnRF433MHzButtonFunctionTriggered, int32, ButtonCode, const FString&, FunctionName, bool, bPressed);

/**
 * HoloCade RF433MHz Receiver Component
 * 
 * Hardware-agnostic 433MHz wireless remote/receiver integration.
 * Provides abstraction layer for different USB receiver modules (RTL-SDR, CC1101, RFM69, Generic)
 * with rolling code validation and replay attack prevention.
 * 
 * Usage:
 * 1. Add component to Actor
 * 2. Configure receiver type and USB device path
 * 3. Subscribe to button event delegates
 * 4. Handle button events in your experience logic
 * 
 * See RFTriggerController_Example.cpp for usage examples.
 */
UCLASS(ClassGroup=(HoloCade), meta=(BlueprintSpawnableComponent))
class RF433MHZ_API URF433MHzReceiver : public UActorComponent
{
	GENERATED_BODY()

public:
	URF433MHzReceiver(const FObjectInitializer& ObjectInitializer);
	virtual ~URF433MHzReceiver();

	/** Configuration */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|RF433MHz")
	FRF433MHzReceiverConfig Config;

	/**
	 * Initialize receiver with configuration
	 * @param InConfig - Receiver configuration
	 * @return True if initialization successful
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|RF433MHz")
	bool InitializeReceiver(const FRF433MHzReceiverConfig& InConfig);

	/**
	 * Shutdown receiver and close USB connection
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|RF433MHz")
	void ShutdownReceiver();

	/**
	 * Check if receiver is connected
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|RF433MHz")
	bool IsConnected() const;

	/**
	 * Get button events (polling method - delegates are preferred)
	 * @param OutEvents - Array of button events
	 * @return True if valid events were received
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|RF433MHz")
	bool GetButtonEvents(TArray<FRF433MHzButtonEvent>& OutEvents);

	/**
	 * Check if rolling code validation is enabled and valid
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|RF433MHz|Security")
	bool IsRollingCodeValid() const;

	/**
	 * Get rolling code drift
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|RF433MHz|Security")
	int32 GetRollingCodeDrift() const;

	/**
	 * Enable code learning mode (for pairing new remotes)
	 * @param TimeoutSeconds - How long learning mode stays active (0 = until disabled)
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|RF433MHz|Learning")
	void EnableLearningMode(float TimeoutSeconds = 30.0f);

	/**
	 * Disable code learning mode
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|RF433MHz|Learning")
	void DisableLearningMode();

	/**
	 * Check if learning mode is active
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|RF433MHz|Learning")
	bool IsLearningModeActive() const;

	// ========================================
	// BUTTON MAPPING & LEARNING API
	// ========================================

	/**
	 * Get all learned buttons
	 * @param OutLearnedButtons - Array of learned button information
	 * @return Number of learned buttons
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|RF433MHz|Mapping")
	int32 GetLearnedButtons(TArray<FRF433MHzLearnedButton>& OutLearnedButtons) const;

	/**
	 * Get number of learned buttons
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|RF433MHz|Mapping")
	int32 GetLearnedButtonCount() const;

	/**
	 * Check if a button code has been learned
	 * @param ButtonCode - Button code to check
	 * @return True if button has been learned
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|RF433MHz|Mapping")
	bool IsButtonLearned(int32 ButtonCode) const;

	/**
	 * Assign a function name to a button code
	 * @param ButtonCode - Button code to map
	 * @param FunctionName - Function name (e.g., "HeightUp", "HeightDown")
	 * @return True if mapping successful (button must be learned first)
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|RF433MHz|Mapping")
	bool AssignButtonFunction(int32 ButtonCode, const FString& FunctionName);

	/**
	 * Unassign a function from a button code
	 * @param ButtonCode - Button code to unmap
	 * @return True if unmapping successful
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|RF433MHz|Mapping")
	bool UnassignButtonFunction(int32 ButtonCode);

	/**
	 * Get function name assigned to a button code
	 * @param ButtonCode - Button code to query
	 * @param OutFunctionName - Function name (empty if not assigned)
	 * @return True if button has an assigned function
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|RF433MHz|Mapping")
	bool GetButtonFunction(int32 ButtonCode, FString& OutFunctionName) const;

	/**
	 * Get all button mappings
	 * @param OutMappings - Array of button mappings
	 * @return Number of active mappings
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|RF433MHz|Mapping")
	int32 GetButtonMappings(TArray<FRF433MHzButtonMapping>& OutMappings) const;

	/**
	 * Clear all learned buttons and mappings
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|RF433MHz|Mapping")
	void ClearAllButtons();

	/**
	 * Remove a specific learned button
	 * @param ButtonCode - Button code to remove
	 * @return True if button was removed
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|RF433MHz|Mapping")
	bool RemoveLearnedButton(int32 ButtonCode);

	// ========================================
	// PERSISTENCE (Save/Load to JSON)
	// ========================================

	/**
	 * Save learned buttons and mappings to JSON file
	 * @param CustomFilePath - Optional custom file path (if empty, uses default: Saved/Config/HoloCade/RF433MHz_Buttons.json)
	 * @return True if save successful
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|RF433MHz|Persistence")
	bool SaveButtonMappings(const FString& CustomFilePath = TEXT(""));

	/**
	 * Load learned buttons and mappings from JSON file
	 * @param CustomFilePath - Optional custom file path (if empty, uses default: Saved/Config/HoloCade/RF433MHz_Buttons.json)
	 * @return True if load successful
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|RF433MHz|Persistence")
	bool LoadButtonMappings(const FString& CustomFilePath = TEXT(""));

	/**
	 * Get default file path for button mappings
	 * @return Path to default JSON file
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|RF433MHz|Persistence")
	FString GetDefaultButtonMappingsFilePath() const;

	/**
	 * Enable/disable auto-save (saves automatically when buttons are learned or mappings change)
	 * @param bEnable - True to enable auto-save
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|RF433MHz|Persistence")
	void SetAutoSave(bool bEnable);

	/**
	 * Check if auto-save is enabled
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|RF433MHz|Persistence")
	bool IsAutoSaveEnabled() const;

	// ========================================
	// DELEGATES (Event-Driven API)
	// ========================================

	/** Called when a button is pressed */
	UPROPERTY(BlueprintAssignable, Category = "HoloCade|RF433MHz|Events")
	FOnRF433MHzButtonPressed OnButtonPressed;

	/** Called when a button is released */
	UPROPERTY(BlueprintAssignable, Category = "HoloCade|RF433MHz|Events")
	FOnRF433MHzButtonReleased OnButtonReleased;

	/** Called when any button event occurs (pressed or released) */
	UPROPERTY(BlueprintAssignable, Category = "HoloCade|RF433MHz|Events")
	FOnRF433MHzButtonEvent OnButtonEvent;

	/** Called when a new remote code is learned during learning mode */
	UPROPERTY(BlueprintAssignable, Category = "HoloCade|RF433MHz|Events")
	FOnRF433MHzCodeLearned OnCodeLearned;

	/** Called when a mapped button function is triggered (only fires if button has assigned function) */
	UPROPERTY(BlueprintAssignable, Category = "HoloCade|RF433MHz|Events")
	FOnRF433MHzButtonFunctionTriggered OnButtonFunctionTriggered;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	/** Receiver implementation (polymorphic) */
	TUniquePtr<I433MHzReceiver> ReceiverImpl;

	/** Last received button states (for detecting press/release) */
	TMap<int32, bool> LastButtonStates;

	/** Registry of learned buttons (ButtonCode -> LearnedButton info) */
	TMap<int32, FRF433MHzLearnedButton> LearnedButtons;

	/** Button function mappings (ButtonCode -> FunctionName) */
	TMap<int32, FString> ButtonFunctionMappings;

	/** Auto-save enabled flag */
	bool bAutoSaveEnabled = true;

	/** Custom save file path (empty = use default) */
	FString CustomSaveFilePath;

	/** Process button events and trigger delegates */
	void ProcessButtonEvents(const TArray<FRF433MHzButtonEvent>& Events);

	/** Register a newly learned button */
	void RegisterLearnedButton(int32 ButtonCode, int32 RollingCode);

	/** Auto-save if enabled */
	void AutoSaveIfEnabled();
};

