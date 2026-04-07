// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "Input/HoloCadePlayerController.h"
#include "Input/HoloCadeInputAdapter.h"
#include "HoloCadeExperienceInterface.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "EngineUtils.h"

AHoloCadePlayerController::AHoloCadePlayerController()
{
	// Enable input by default
	bShowMouseCursor = false;
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;
}

void AHoloCadePlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Auto-find experience if not already assigned
	if (bAutoFindExperience && !CurrentExperience.GetInterface())
	{
		// Search for any actor implementing IHoloCadeExperienceInterface
		for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		{
			AActor* Actor = *It;
			if (Actor && Actor->GetClass()->ImplementsInterface(UHoloCadeExperienceInterface::StaticClass()))
			{
				CurrentExperience = TScriptInterface<IHoloCadeExperienceInterface>(Actor);
				UE_LOG(LogTemp, Log, TEXT("[HoloCadePlayerController] Auto-assigned CurrentExperience: %s"), *Actor->GetName());
				break; // Use first experience found
			}
		}

		if (!CurrentExperience.GetInterface())
		{
			UE_LOG(LogTemp, Warning, TEXT("[HoloCadePlayerController] No experience found in world. Enhanced Input will not work."));
		}
	}

	// Add Input Mapping Context
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (InputMappingContext)
		{
			Subsystem->AddMappingContext(InputMappingContext, MappingPriority);
			UE_LOG(LogTemp, Log, TEXT("[HoloCadePlayerController] Added Input Mapping Context: %s"), *InputMappingContext->GetName());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[HoloCadePlayerController] No InputMappingContext assigned. Create one in the editor and assign it."));
		}
	}
}

void AHoloCadePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EIC)
	{
		UE_LOG(LogTemp, Error, TEXT("[HoloCadePlayerController] Enhanced Input Component not found. Enable Enhanced Input plugin."));
		return;
	}

	// Bind digital buttons (pressed and released)
	if (IA_Button0)
	{
		EIC->BindAction(IA_Button0, ETriggerEvent::Started, this, &AHoloCadePlayerController::OnButton0Pressed);
		EIC->BindAction(IA_Button0, ETriggerEvent::Completed, this, &AHoloCadePlayerController::OnButton0Released);
	}
	if (IA_Button1)
	{
		EIC->BindAction(IA_Button1, ETriggerEvent::Started, this, &AHoloCadePlayerController::OnButton1Pressed);
		EIC->BindAction(IA_Button1, ETriggerEvent::Completed, this, &AHoloCadePlayerController::OnButton1Released);
	}
	if (IA_Button2)
	{
		EIC->BindAction(IA_Button2, ETriggerEvent::Started, this, &AHoloCadePlayerController::OnButton2Pressed);
		EIC->BindAction(IA_Button2, ETriggerEvent::Completed, this, &AHoloCadePlayerController::OnButton2Released);
	}
	if (IA_Button3)
	{
		EIC->BindAction(IA_Button3, ETriggerEvent::Started, this, &AHoloCadePlayerController::OnButton3Pressed);
		EIC->BindAction(IA_Button3, ETriggerEvent::Completed, this, &AHoloCadePlayerController::OnButton3Released);
	}
	if (IA_Button4)
	{
		EIC->BindAction(IA_Button4, ETriggerEvent::Started, this, &AHoloCadePlayerController::OnButton4Pressed);
		EIC->BindAction(IA_Button4, ETriggerEvent::Completed, this, &AHoloCadePlayerController::OnButton4Released);
	}
	if (IA_Button5)
	{
		EIC->BindAction(IA_Button5, ETriggerEvent::Started, this, &AHoloCadePlayerController::OnButton5Pressed);
		EIC->BindAction(IA_Button5, ETriggerEvent::Completed, this, &AHoloCadePlayerController::OnButton5Released);
	}
	if (IA_Button6)
	{
		EIC->BindAction(IA_Button6, ETriggerEvent::Started, this, &AHoloCadePlayerController::OnButton6Pressed);
		EIC->BindAction(IA_Button6, ETriggerEvent::Completed, this, &AHoloCadePlayerController::OnButton6Released);
	}
	if (IA_Button7)
	{
		EIC->BindAction(IA_Button7, ETriggerEvent::Started, this, &AHoloCadePlayerController::OnButton7Pressed);
		EIC->BindAction(IA_Button7, ETriggerEvent::Completed, this, &AHoloCadePlayerController::OnButton7Released);
	}

	// Bind analog axes (continuous triggering)
	if (IA_Axis0)
		EIC->BindAction(IA_Axis0, ETriggerEvent::Triggered, this, &AHoloCadePlayerController::OnAxis0Changed);
	if (IA_Axis1)
		EIC->BindAction(IA_Axis1, ETriggerEvent::Triggered, this, &AHoloCadePlayerController::OnAxis1Changed);
	if (IA_Axis2)
		EIC->BindAction(IA_Axis2, ETriggerEvent::Triggered, this, &AHoloCadePlayerController::OnAxis2Changed);
	if (IA_Axis3)
		EIC->BindAction(IA_Axis3, ETriggerEvent::Triggered, this, &AHoloCadePlayerController::OnAxis3Changed);

	UE_LOG(LogTemp, Log, TEXT("[HoloCadePlayerController] Enhanced Input bindings created successfully."));
}

// ========================================
// DIGITAL INPUT CALLBACKS
// ========================================

void AHoloCadePlayerController::OnButton0Pressed(const FInputActionValue& Value)  { InjectButton(0, true); }
void AHoloCadePlayerController::OnButton0Released(const FInputActionValue& Value) { InjectButton(0, false); }
void AHoloCadePlayerController::OnButton1Pressed(const FInputActionValue& Value)  { InjectButton(1, true); }
void AHoloCadePlayerController::OnButton1Released(const FInputActionValue& Value) { InjectButton(1, false); }
void AHoloCadePlayerController::OnButton2Pressed(const FInputActionValue& Value)  { InjectButton(2, true); }
void AHoloCadePlayerController::OnButton2Released(const FInputActionValue& Value) { InjectButton(2, false); }
void AHoloCadePlayerController::OnButton3Pressed(const FInputActionValue& Value)  { InjectButton(3, true); }
void AHoloCadePlayerController::OnButton3Released(const FInputActionValue& Value) { InjectButton(3, false); }
void AHoloCadePlayerController::OnButton4Pressed(const FInputActionValue& Value)  { InjectButton(4, true); }
void AHoloCadePlayerController::OnButton4Released(const FInputActionValue& Value) { InjectButton(4, false); }
void AHoloCadePlayerController::OnButton5Pressed(const FInputActionValue& Value)  { InjectButton(5, true); }
void AHoloCadePlayerController::OnButton5Released(const FInputActionValue& Value) { InjectButton(5, false); }
void AHoloCadePlayerController::OnButton6Pressed(const FInputActionValue& Value)  { InjectButton(6, true); }
void AHoloCadePlayerController::OnButton6Released(const FInputActionValue& Value) { InjectButton(6, false); }
void AHoloCadePlayerController::OnButton7Pressed(const FInputActionValue& Value)  { InjectButton(7, true); }
void AHoloCadePlayerController::OnButton7Released(const FInputActionValue& Value) { InjectButton(7, false); }

// ========================================
// ANALOG INPUT CALLBACKS
// ========================================

void AHoloCadePlayerController::OnAxis0Changed(const FInputActionValue& Value) { InjectAxis(0, Value.Get<float>()); }
void AHoloCadePlayerController::OnAxis1Changed(const FInputActionValue& Value) { InjectAxis(1, Value.Get<float>()); }
void AHoloCadePlayerController::OnAxis2Changed(const FInputActionValue& Value) { InjectAxis(2, Value.Get<float>()); }
void AHoloCadePlayerController::OnAxis3Changed(const FInputActionValue& Value) { InjectAxis(3, Value.Get<float>()); }

// ========================================
// HELPERS
// ========================================

void AHoloCadePlayerController::InjectButton(int32 ButtonIndex, bool bPressed)
{
	if (!CurrentExperience.GetInterface())
	{
		UE_LOG(LogTemp, Warning, TEXT("[HoloCadePlayerController] CurrentExperience is null. Cannot inject button input."));
		return;
	}

	UHoloCadeInputAdapter* InputAdapter = CurrentExperience->GetInputAdapter();
	if (!InputAdapter)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HoloCadePlayerController] Experience has no InputAdapter. Cannot inject button input."));
		return;
	}

	if (bDebugLogInput)
	{
		UE_LOG(LogTemp, Log, TEXT("[HoloCadePlayerController] Button %d %s"), ButtonIndex, bPressed ? TEXT("Pressed") : TEXT("Released"));
	}

	if (bPressed)
		InputAdapter->InjectButtonPress(ButtonIndex);
	else
		InputAdapter->InjectButtonRelease(ButtonIndex);
}

void AHoloCadePlayerController::InjectAxis(int32 AxisIndex, float Value)
{
	if (!CurrentExperience.GetInterface())
	{
		UE_LOG(LogTemp, Warning, TEXT("[HoloCadePlayerController] CurrentExperience is null. Cannot inject axis input."));
		return;
	}

	UHoloCadeInputAdapter* InputAdapter = CurrentExperience->GetInputAdapter();
	if (!InputAdapter)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HoloCadePlayerController] Experience has no InputAdapter. Cannot inject axis input."));
		return;
	}

	if (bDebugLogInput)
	{
		UE_LOG(LogTemp, Log, TEXT("[HoloCadePlayerController] Axis %d = %.2f"), AxisIndex, Value);
	}

	InputAdapter->InjectAxisValue(AxisIndex, Value);
}

