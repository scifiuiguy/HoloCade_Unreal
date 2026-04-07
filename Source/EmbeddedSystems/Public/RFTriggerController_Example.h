// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RFTriggerController_Example.generated.h"

// Forward declarations
class URF433MHzReceiver;

/**
 * RF Trigger Controller Example
 * 
 * Example Actor demonstrating how to use the RF433MHz low-level API
 * for 433MHz wireless remote/receiver integration.
 * 
 * This example shows:
 * - Height calibration clicker integration (SuperheroFlightExperience)
 * - Wireless trigger buttons (costume-embedded)
 * - Emergency stop remote
 * - Safety interlock enforcement
 * - Rolling code validation
 * - Code learning mode
 * - Multiple receivers (multiple remotes)
 * 
 * See RFTriggerController_Example.cpp for implementation details.
 */
UCLASS(Blueprintable, ClassGroup=(HoloCade))
class EMBEDDEDSYSTEMS_API ARFTriggerControllerExample : public AActor
{
	GENERATED_BODY()
	
public:
	ARFTriggerControllerExample();
	
	/** RF433MHz receiver component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HoloCade|RF433MHz")
	TObjectPtr<URF433MHzReceiver> RFReceiver;
	
	/** Additional receivers for multiple remotes */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HoloCade|RF433MHz")
	TObjectPtr<URF433MHzReceiver> HeightCalibrationReceiver;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HoloCade|RF433MHz")
	TObjectPtr<URF433MHzReceiver> TriggerReceiver;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HoloCade|RF433MHz")
	TObjectPtr<URF433MHzReceiver> EmergencyStopReceiver;
	
	/** Initialize height calibration system */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|RF433MHz|Examples")
	void InitializeHeightCalibration();
	
	/** Initialize wireless trigger buttons */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|RF433MHz|Examples")
	void InitializeWirelessTriggers();
	
	/** Initialize emergency stop remote */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|RF433MHz|Examples")
	void InitializeEmergencyStop();
	
	/** Initialize multiple receivers */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|RF433MHz|Examples")
	void InitializeMultipleReceivers();
	
	/** Initialize for SuperheroFlightExperience */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|RF433MHz|Examples")
	void InitializeSuperheroFlightCalibration();
	
	/** Enable code learning mode */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|RF433MHz|Examples")
	void EnableCodeLearningMode();
	
	/** Check rolling code validation status */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|RF433MHz|Examples")
	void CheckRollingCodeStatus();

protected:
	virtual void BeginPlay() override;

private:
	/** Button event handlers */
	UFUNCTION()
	void HandleButtonPressed(int32 ButtonCode);
	
	UFUNCTION()
	void HandleButtonReleased(int32 ButtonCode);
	
	UFUNCTION()
	void HandleTriggerButtonPressed(int32 ButtonCode);
	
	UFUNCTION()
	void HandleTriggerButtonFunction(int32 ButtonCode, const FString& FunctionName, bool bPressed);
	
	UFUNCTION()
	void HandleEmergencyStop(int32 ButtonCode);
	
	UFUNCTION()
	void HandleButtonFunctionTriggered(int32 ButtonCode, const FString& FunctionName, bool bPressed);
	
	// HandleCalibrationButton removed - use OnCalibrationButtonPressed instead
	
	UFUNCTION()
	void OnCalibrationButtonPressed(int32 ButtonCode);
	
	UFUNCTION()
	void OnCalibrationButtonReleased(int32 ButtonCode);
	
	UFUNCTION()
	void OnRemoteCodeLearned(int32 ButtonCode, int32 RollingCode);
	
	/** Winch control (for SuperheroFlightExperience example) */
	void AdjustWinchHeight(float DeltaInches);
	void StopWinchMovement();
	
	/** Safety interlock enforcement */
	void ProcessCalibrationButton(int32 ButtonCode, bool bPressed);
	
	/** State variables */
	bool bIsCalibrationMode = false;
	bool bPlaySessionActive = false;
	float LastCalibrationActivity = 0.0f;  // Time in seconds (not milliseconds)
};

