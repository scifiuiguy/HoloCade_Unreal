// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "HoloCadeExperienceInterface.h"
#include "HoloCadePlayerController.generated.h"

// Forward declarations
class UInputAction;
class UInputMappingContext;
class IHoloCadeExperienceInterface;
class UHoloCadeInputAdapter;

/**
 * AHoloCadePlayerController
 *
 * Optional helper class that bridges Unreal's Enhanced Input System to the HoloCadeInputAdapter.
 * This allows developers to use standard gamepads, keyboards, and mice for testing LBE experiences
 * without physical hardware (ESP32, VR controllers, etc.).
 *
 * Usage:
 * 1. Create Input Actions in the editor (e.g., IA_Button0, IA_Button1, IA_Axis0)
 * 2. Create an Input Mapping Context and assign gamepad/keyboard bindings
 * 3. Set these as properties on this controller (or a Blueprint child)
 * 4. Input will automatically route to the experience's InputAdapter
 *
 * Typical Use Cases:
 * - Development testing with gamepad before hardware is available
 * - Listen server hosts using keyboard/gamepad instead of VR controllers
 * - Rapid prototyping without ESP32/Arduino setup
 *
 * Production Deployment:
 * - In production LBE venues, the dedicated server reads directly from ESP32 via InputAdapter
 * - This controller is optional and only used for development/testing
 */
UCLASS()
class HOLOCADECORE_API AHoloCadePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AHoloCadePlayerController();

	// ========================================
	// EXPERIENCE REFERENCE
	// ========================================

	/** Reference to the current experience (auto-assigned in BeginPlay if not set) */
	UPROPERTY(BlueprintReadWrite, Category = "HoloCade|Input")
	TScriptInterface<IHoloCadeExperienceInterface> CurrentExperience;

	// ========================================
	// INPUT MAPPING CONTEXT
	// ========================================

	/** 
	 * Input Mapping Context for this controller.
	 * Assign a context that maps gamepad/keyboard inputs to the Input Actions below.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HoloCade|Input|Mapping")
	TObjectPtr<UInputMappingContext> InputMappingContext;

	/**
	 * Priority for the Input Mapping Context (higher = takes precedence).
	 * Default is 0. Increase if you have conflicting contexts.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HoloCade|Input|Mapping")
	int32 MappingPriority = 0;

	// ========================================
	// DIGITAL INPUT ACTIONS (Buttons)
	// ========================================

	/** Input Action for digital button 0 (e.g., gamepad A button, keyboard 1) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HoloCade|Input|Digital")
	TObjectPtr<UInputAction> IA_Button0;

	/** Input Action for digital button 1 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HoloCade|Input|Digital")
	TObjectPtr<UInputAction> IA_Button1;

	/** Input Action for digital button 2 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HoloCade|Input|Digital")
	TObjectPtr<UInputAction> IA_Button2;

	/** Input Action for digital button 3 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HoloCade|Input|Digital")
	TObjectPtr<UInputAction> IA_Button3;

	/** Input Action for digital button 4 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HoloCade|Input|Digital")
	TObjectPtr<UInputAction> IA_Button4;

	/** Input Action for digital button 5 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HoloCade|Input|Digital")
	TObjectPtr<UInputAction> IA_Button5;

	/** Input Action for digital button 6 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HoloCade|Input|Digital")
	TObjectPtr<UInputAction> IA_Button6;

	/** Input Action for digital button 7 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HoloCade|Input|Digital")
	TObjectPtr<UInputAction> IA_Button7;

	// ========================================
	// ANALOG INPUT ACTIONS (Axes)
	// ========================================

	/** Input Action for analog axis 0 (e.g., gamepad left stick X, throttle) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HoloCade|Input|Analog")
	TObjectPtr<UInputAction> IA_Axis0;

	/** Input Action for analog axis 1 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HoloCade|Input|Analog")
	TObjectPtr<UInputAction> IA_Axis1;

	/** Input Action for analog axis 2 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HoloCade|Input|Analog")
	TObjectPtr<UInputAction> IA_Axis2;

	/** Input Action for analog axis 3 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HoloCade|Input|Analog")
	TObjectPtr<UInputAction> IA_Axis3;

	// ========================================
	// CONFIGURATION
	// ========================================

	/**
	 * If true, automatically finds and assigns CurrentExperience in BeginPlay.
	 * If false, you must manually assign CurrentExperience.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Input|Config")
	bool bAutoFindExperience = true;

	/**
	 * If true, logs input events for debugging.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Input|Config")
	bool bDebugLogInput = false;

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

private:
	// ========================================
	// INPUT CALLBACKS (Digital)
	// ========================================

	void OnButton0Pressed(const FInputActionValue& Value);
	void OnButton0Released(const FInputActionValue& Value);
	void OnButton1Pressed(const FInputActionValue& Value);
	void OnButton1Released(const FInputActionValue& Value);
	void OnButton2Pressed(const FInputActionValue& Value);
	void OnButton2Released(const FInputActionValue& Value);
	void OnButton3Pressed(const FInputActionValue& Value);
	void OnButton3Released(const FInputActionValue& Value);
	void OnButton4Pressed(const FInputActionValue& Value);
	void OnButton4Released(const FInputActionValue& Value);
	void OnButton5Pressed(const FInputActionValue& Value);
	void OnButton5Released(const FInputActionValue& Value);
	void OnButton6Pressed(const FInputActionValue& Value);
	void OnButton6Released(const FInputActionValue& Value);
	void OnButton7Pressed(const FInputActionValue& Value);
	void OnButton7Released(const FInputActionValue& Value);

	// ========================================
	// INPUT CALLBACKS (Analog)
	// ========================================

	void OnAxis0Changed(const FInputActionValue& Value);
	void OnAxis1Changed(const FInputActionValue& Value);
	void OnAxis2Changed(const FInputActionValue& Value);
	void OnAxis3Changed(const FInputActionValue& Value);

	// ========================================
	// HELPERS
	// ========================================

	/** Helper to inject button press/release to InputAdapter */
	void InjectButton(int32 ButtonIndex, bool bPressed);

	/** Helper to inject axis value to InputAdapter */
	void InjectAxis(int32 AxisIndex, float Value);
};

