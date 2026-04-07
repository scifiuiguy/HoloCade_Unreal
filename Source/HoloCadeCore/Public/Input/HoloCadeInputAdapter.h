// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HoloCadeEmbeddedDeviceInterface.h"
#include "HoloCadeInputAdapter.generated.h"

// Forward declarations
class IHoloCadeEmbeddedDeviceInterface;

/**
 * Input Event Delegates
 * These fire on ALL machines (server and clients) when input is detected
 * Use these to respond to input in your experience templates
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInputButtonPressed, int32, ButtonIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInputButtonReleased, int32, ButtonIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInputAxisChanged, int32, AxisIndex, float, Value);

/**
 * HoloCade Input Adapter
 * 
 * Universal input adapter that works with ANY input source and ANY server type.
 * Handles all the complexity of authority checks, RPC routing, and input sources.
 * 
 * USAGE:
 * 1. Add this component to your Experience actor
 * 2. Bind to input events (OnButtonPressed, OnAxisChanged, etc.)
 * 3. Optionally connect embedded systems or override VR controller input
 * 4. Everything else is handled automatically
 * 
 * WORKS WITH:
 * - Dedicated Servers (embedded systems, keyboard, AI)
 * - Listen Servers (VR controllers, embedded systems, keyboard, AI)
 * - All input sources can coexist simultaneously
 * 
 * EXAMPLE:
 * ```cpp
 * // In your experience BeginPlay():
 * InputAdapter->OnButtonPressed.AddDynamic(this, &AMyExperience::OnInputButton);
 * 
 * // In your callback:
 * void AMyExperience::OnInputButton(int32 ButtonIndex)
 * {
 *     // This fires on server AND clients (input is replicated automatically)
 *     if (ButtonIndex == 0) AdvanceToNextState();
 * }
 * ```
 */
UCLASS(Blueprintable, ClassGroup=(HoloCade), meta=(BlueprintSpawnableComponent))
class HOLOCADECORE_API UHoloCadeInputAdapter : public UActorComponent
{
	GENERATED_BODY()

public:
	UHoloCadeInputAdapter();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ========================================
	// INPUT EVENTS (Subscribe to these)
	// ========================================

	/** Fired when any button is pressed (replicated to all clients) */
	UPROPERTY(BlueprintAssignable, Category = "HoloCade|Input")
	FOnInputButtonPressed OnButtonPressed;

	/** Fired when any button is released (replicated to all clients) */
	UPROPERTY(BlueprintAssignable, Category = "HoloCade|Input")
	FOnInputButtonReleased OnButtonReleased;

	/** Fired when any axis value changes (replicated to all clients) */
	UPROPERTY(BlueprintAssignable, Category = "HoloCade|Input")
	FOnInputAxisChanged OnAxisChanged;

	// ========================================
	// CONFIGURATION
	// ========================================

	/** Reference to embedded device controller (ESP32, Arduino, etc.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Input|Embedded Systems")
	TScriptInterface<IHoloCadeEmbeddedDeviceInterface> EmbeddedDeviceController;

	/** Enable embedded system input (ESP32 wrist buttons, etc.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Input|Embedded Systems")
	bool bEnableEmbeddedSystemInput = true;

	/** Enable VR controller input (for listen server hosts) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Input|VR")
	bool bEnableVRControllerInput = false;

	/** Number of button inputs to track */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Input|Config", meta = (ClampMin = "1", ClampMax = "32"))
	int32 ButtonCount = 4;

	/** Number of axis inputs to track */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Input|Config", meta = (ClampMin = "0", ClampMax = "32"))
	int32 AxisCount = 0;

	// ========================================
	// INPUT INJECTION API
	// ========================================

	/**
	 * Inject a button press from any source (VR controller, keyboard, AI, etc.)
	 * Automatically handles authority checks and replication
	 * 
	 * @param ButtonIndex - Index of the button (0-31)
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Input")
	void InjectButtonPress(int32 ButtonIndex);

	/**
	 * Inject a button release from any source
	 * 
	 * @param ButtonIndex - Index of the button (0-31)
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Input")
	void InjectButtonRelease(int32 ButtonIndex);

	/**
	 * Inject an axis value change from any source
	 * 
	 * @param AxisIndex - Index of the axis (0-31)
	 * @param Value - New axis value (typically -1.0 to 1.0)
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Input")
	void InjectAxisValue(int32 AxisIndex, float Value);

	/**
	 * Get current button state (for polling)
	 * 
	 * @param ButtonIndex - Index of the button (0-31)
	 * @return True if button is currently pressed
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|Input")
	bool IsButtonPressed(int32 ButtonIndex) const;

	/**
	 * Get current axis value (for polling)
	 * 
	 * @param AxisIndex - Index of the axis (0-31)
	 * @return Current axis value
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|Input")
	float GetAxisValue(int32 AxisIndex) const;

	// ========================================
	// VR CONTROLLER OVERRIDE (Blueprint)
	// ========================================

	/**
	 * Process VR controller input
	 * Override this in Blueprint to add VR controller support for listen servers
	 * Call InjectButtonPress/InjectButtonRelease/InjectAxisValue as needed
	 * 
	 * Only runs on authority (server or listen server host)
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "HoloCade|Input|VR")
	void ProcessVRControllerInput();
	virtual void ProcessVRControllerInput_Implementation() {}

protected:
	virtual void BeginPlay() override;

private:
	// ========================================
	// INTERNAL STATE (Replicated)
	// ========================================

	/** Replicated button states (bitfield for efficiency) */
	UPROPERTY(ReplicatedUsing=OnRep_ButtonStates)
	int32 ReplicatedButtonStates = 0;

	/** Replicated axis values */
	UPROPERTY(ReplicatedUsing=OnRep_AxisValues)
	TArray<float> ReplicatedAxisValues;

	/** Previous button states for edge detection (authority only) */
	int32 PreviousButtonStates = 0;

	/** Previous axis values for change detection (authority only) */
	TArray<float> PreviousAxisValues;

	// ========================================
	// REPLICATION
	// ========================================

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** RepNotify for button state changes */
	UFUNCTION()
	void OnRep_ButtonStates();

	/** RepNotify for axis value changes */
	UFUNCTION()
	void OnRep_AxisValues();

	// ========================================
	// INPUT PROCESSING (Authority Only)
	// ========================================

	/** Process embedded system input (ESP32, Arduino, etc.) */
	void ProcessEmbeddedSystemInput();

	// ========================================
	// RPC FUNCTIONS (Client → Server)
	// ========================================

	/** Client requests button press */
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerInjectButtonPress(int32 ButtonIndex);

	/** Client requests button release */
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerInjectButtonRelease(int32 ButtonIndex);

	/** Client requests axis value change */
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerInjectAxisValue(int32 AxisIndex, float Value);

	// ========================================
	// INTERNAL HELPERS
	// ========================================

	/** Update button state on authority */
	void UpdateButtonState(int32 ButtonIndex, bool bPressed);

	/** Update axis value on authority */
	void UpdateAxisValue(int32 AxisIndex, float Value);

	/** Fire delegates for button/axis changes */
	void BroadcastButtonChange(int32 ButtonIndex, bool bPressed);
	void BroadcastAxisChange(int32 AxisIndex, float Value);
};

