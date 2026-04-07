// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "HoloCadeExperienceBase.h"
#include "SuperheroFlight/Models/SuperheroFlightDualWinchState.h"
#include "SuperheroFlight/Models/SuperheroFlightTelemetry.h"
#include "SuperheroFlight/Models/SuperheroFlightGameState.h"
#include "SuperheroFlight/Models/SuperheroFlightGestureState.h"
#include "SuperheroFlightExperience.generated.h"

// Forward declarations
class USuperheroFlightECUController;
class UFlightHandsController;
class UGestureDebugger;
class URF433MHzReceiver;

/**
 * Superhero Flight Experience Template
 * 
 * Pre-configured dual-winch suspended harness system for free-body flight (flying like Superman).
 * Uses gesture-based control (10-finger/arm gestures) - no HOTAS, no button events, no 6DOF body tracking.
 * 
 * Features:
 * - Dual-winch system (front shoulder-hook, rear pelvis-hook)
 * - Five flight modes: Standing, Hovering, Flight-Up, Flight-Forward, Flight-Down
 * - Gesture-based control (fist detection, HMD-to-hands vector analysis)
 * - Virtual altitude system (raycast for landable surfaces)
 * - 433MHz wireless height calibration clicker
 * - Server-side parameter exposure (airHeight, proneHeight, speeds, angles)
 * - Safety interlocks (calibration mode only, movement limits, timeout)
 * 
 * Note: Distinct from FlightSimExperience (2DOF gyroscope HOTAS cockpit for jet/spaceship simulation).
 */
UCLASS(Blueprintable, ClassGroup=(HoloCade))
class HOLOCADEEXPERIENCES_API ASuperheroFlightExperience : public AHoloCadeExperienceBase
{
	GENERATED_BODY()
	
public:
	ASuperheroFlightExperience();

	/** Superhero Flight ECU controller for winch hardware communication */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HoloCade|SuperheroFlight")
	TObjectPtr<USuperheroFlightECUController> ECUController;

	/** Flight hands controller (client-side, runs on HMD) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HoloCade|SuperheroFlight|Gesture")
	TObjectPtr<UFlightHandsController> FlightHandsController;

	/** Gesture debugger (HMD HUD visualization for Ops Tech) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HoloCade|SuperheroFlight|Debug")
	TObjectPtr<UGestureDebugger> GestureDebugger;

	/** 433MHz RF receiver for height calibration clicker */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HoloCade|SuperheroFlight|Calibration")
	TObjectPtr<URF433MHzReceiver> RF433MHzReceiver;

	// =====================================
	// ECU Configuration
	// =====================================

	/** ECU IP address */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|SuperheroFlight|ECU")
	FString ECUIPAddress = TEXT("192.168.1.100");

	/** ECU UDP port */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|SuperheroFlight|ECU")
	int32 ECUPort = 8888;

	// =====================================
	// Server-Side Parameters (Exposed in Command Console)
	// =====================================

	/** Air height (inches) - Height for hovering/flight-up/flight-down */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|SuperheroFlight|Parameters")
	float AirHeight = 24.0f;

	/** Prone height (inches) - Height for flight-forward (prone position) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|SuperheroFlight|Parameters")
	float ProneHeight = 36.0f;

	/** Standing ground height (inches) - Calibrated per-player baseline (read-only) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HoloCade|SuperheroFlight|Parameters")
	float StandingGroundHeight = 0.0f;

	/** Player height compensation (multiplier) - Adjusts proneHeight for player size */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|SuperheroFlight|Parameters")
	float PlayerHeightCompensation = 1.0f;

	/** Flying forward speed (maximum forward flight speed) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|SuperheroFlight|Parameters")
	float FlyingForwardSpeed = 10.0f;

	/** Flying up speed (maximum upward flight speed) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|SuperheroFlight|Parameters")
	float FlyingUpSpeed = 5.0f;

	/** Flying down speed (maximum downward flight speed) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|SuperheroFlight|Parameters")
	float FlyingDownSpeed = 8.0f;

	/** Arm length (inches) - Auto-calibrated from player height, manually adjustable */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|SuperheroFlight|Parameters")
	float ArmLength = 28.0f;

	/** Up to forward angle threshold (degrees) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|SuperheroFlight|Parameters")
	float UpToForwardAngle = 45.0f;

	/** Forward to down angle threshold (degrees) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|SuperheroFlight|Parameters")
	float ForwardToDownAngle = 45.0f;

	// =====================================
	// State
	// =====================================

	/** Current game state */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HoloCade|SuperheroFlight|State")
	ESuperheroFlightGameState CurrentGameState = ESuperheroFlightGameState::Standing;

	/** Whether play session is active */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HoloCade|SuperheroFlight|State")
	bool bPlaySessionActive = false;

	/** Whether emergency stop is active */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HoloCade|SuperheroFlight|State")
	bool bEmergencyStop = false;

	/** Current dual-winch state (from ECU) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HoloCade|SuperheroFlight|State")
	FSuperheroFlightDualWinchState CurrentWinchState;

	/** Current system telemetry (from ECU) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HoloCade|SuperheroFlight|State")
	FSuperheroFlightTelemetry CurrentTelemetry;

	// =====================================
	// API Methods
	// =====================================

	/**
	 * Acknowledge standing ground height
	 * Sets current winch position as new baseline for relative positioning
	 * Called by Ops Tech after height calibration is complete
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|SuperheroFlight|Calibration")
	void AcknowledgeStandingGroundHeight();

	/**
	 * Get current game state
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|SuperheroFlight|State")
	ESuperheroFlightGameState GetCurrentGameState() const { return CurrentGameState; }

	/**
	 * Get current winch state
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|SuperheroFlight|State")
	FSuperheroFlightDualWinchState GetCurrentWinchState() const { return CurrentWinchState; }

	/**
	 * Get current telemetry
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|SuperheroFlight|State")
	FSuperheroFlightTelemetry GetCurrentTelemetry() const { return CurrentTelemetry; }

	virtual int32 GetMaxPlayers() const override { return 1; } // Single player for now

protected:
	virtual bool InitializeExperienceImpl() override;
	virtual void ShutdownExperienceImpl() override;
	virtual void Tick(float DeltaTime) override;

private:
	/** Last gesture state (for detecting changes) */
	FSuperheroFlightGestureState LastGestureState;

	/** Calibration inactive time (for timeout protection) */
	float CalibrationInactiveTime = 0.0f;

	/** Calibration timeout (5 minutes) */
	float CalibrationTimeout = 300.0f;

	/** Update winch positions based on current game state and gesture input */
	void UpdateWinchPositions(float DeltaTime);

	/** Handle gesture state changes */
	void HandleGestureStateChanged(const FSuperheroFlightGestureState& GestureState);

	/** Handle 433MHz calibration button events */
	void HandleCalibrationButton(int32 ButtonCode, const FString& FunctionName, bool bPressed);

	/** Transition to a new game state */
	void TransitionToGameState(ESuperheroFlightGameState NewState);

	/** Calculate target winch positions for current game state */
	void CalculateTargetWinchPositions(float& OutFrontPosition, float& OutRearPosition) const;

	/** Apply safety interlocks for calibration mode */
	bool CheckCalibrationSafetyInterlocks(const FString& FunctionName) const;
};

