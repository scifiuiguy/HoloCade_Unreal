// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Networking/HoloCadeUDPTransport.h"
#include "SuperheroFlight/Models/SuperheroFlightDualWinchState.h"
#include "SuperheroFlight/Models/SuperheroFlightTelemetry.h"
#include "SuperheroFlight/Models/SuperheroFlightGameState.h"
#include "SuperheroFlightECUController.generated.h"

/**
 * Superhero Flight ECU Controller
 * 
 * Handles UDP communication with SuperheroFlightExperience_ECU firmware.
 * Manages dual-winch control (front and rear winches), game state, and safety interlocks.
 * 
 * Communication Protocol (Binary HoloCade over UDP):
 * - Server → ECU: Winch positions/speeds, game state, parameters (Channels 0-13)
 * - ECU → Server: Dual-winch telemetry (Channel 310), system telemetry (Channel 311)
 * 
 * Channel Mapping (Server → ECU):
 * - Channel 0: Front winch position (inches, relative to standingGroundHeight)
 * - Channel 1: Front winch speed (inches/second)
 * - Channel 2: Rear winch position (inches, relative to standingGroundHeight)
 * - Channel 3: Rear winch speed (inches/second)
 * - Channel 6: Game state (0=standing, 1=hovering, 2=flight-up, 3=flight-forward, 4=flight-down)
 * - Channel 7: Emergency stop (true = stop all systems, return to standing)
 * - Channel 9: Play session active (true = winches can operate)
 * - Channel 10: Standing ground height acknowledgment (current winch position becomes new baseline)
 * - Channel 11: Air height parameter (inches)
 * - Channel 12: Prone height parameter (inches)
 * - Channel 13: Player height compensation (multiplier)
 */
UCLASS(ClassGroup=(HoloCade), meta=(BlueprintSpawnableComponent))
class HOLOCADEEXPERIENCES_API USuperheroFlightECUController : public UActorComponent
{
	GENERATED_BODY()

public:
	USuperheroFlightECUController();
	virtual ~USuperheroFlightECUController();

	/**
	 * Initialize connection to Superhero Flight ECU
	 * @param ECUIPAddress - IP address of the Superhero Flight ECU
	 * @param ECUPort - UDP port for ECU communication
	 * @return True if initialization successful
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|SuperheroFlight|ECU")
	bool InitializeECU(const FString& ECUIPAddress, int32 ECUPort = 8888);

	/**
	 * Shutdown ECU connection
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|SuperheroFlight|ECU")
	void ShutdownECU();

	/**
	 * Check if ECU is connected
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|SuperheroFlight|ECU")
	bool IsECUConnected() const;

	// =====================================
	// Winch Control (Server → ECU)
	// =====================================

	/**
	 * Set front winch position (Channel 0)
	 * @param Position - Position in inches relative to standingGroundHeight
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|SuperheroFlight|ECU|Winch")
	void SetFrontWinchPosition(float Position);

	/**
	 * Set front winch speed (Channel 1)
	 * @param Speed - Speed in inches/second
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|SuperheroFlight|ECU|Winch")
	void SetFrontWinchSpeed(float Speed);

	/**
	 * Set rear winch position (Channel 2)
	 * @param Position - Position in inches relative to standingGroundHeight
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|SuperheroFlight|ECU|Winch")
	void SetRearWinchPosition(float Position);

	/**
	 * Set rear winch speed (Channel 3)
	 * @param Speed - Speed in inches/second
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|SuperheroFlight|ECU|Winch")
	void SetRearWinchSpeed(float Speed);

	/**
	 * Set both winch positions simultaneously
	 * @param FrontPosition - Front winch position (inches)
	 * @param RearPosition - Rear winch position (inches)
	 * @param Speed - Speed for both winches (inches/second)
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|SuperheroFlight|ECU|Winch")
	void SetDualWinchPositions(float FrontPosition, float RearPosition, float Speed);

	// =====================================
	// Game State (Server → ECU)
	// =====================================

	/**
	 * Set game state (Channel 6)
	 * @param GameState - Game state enum (0=standing, 1=hovering, 2=flight-up, 3=flight-forward, 4=flight-down)
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|SuperheroFlight|ECU|GameState")
	void SetGameState(ESuperheroFlightGameState GameState);

	/**
	 * Set play session active state (Channel 9)
	 * @param bActive - True if play session is active
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|SuperheroFlight|ECU|GameState")
	void SetPlaySessionActive(bool bActive);

	/**
	 * Send emergency stop command (Channel 7)
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|SuperheroFlight|ECU|GameState")
	void EmergencyStop();

	// =====================================
	// Parameters (Server → ECU)
	// =====================================

	/**
	 * Acknowledge standing ground height (Channel 10)
	 * Sets current winch position as new baseline for relative positioning
	 * @param Height - Current winch position becomes new standingGroundHeight
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|SuperheroFlight|ECU|Parameters")
	void AcknowledgeStandingGroundHeight(float Height);

	/**
	 * Set air height parameter (Channel 11)
	 * @param Height - Height in inches for hovering/flight-up/flight-down
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|SuperheroFlight|ECU|Parameters")
	void SetAirHeight(float Height);

	/**
	 * Set prone height parameter (Channel 12)
	 * @param Height - Height in inches for flight-forward (prone position)
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|SuperheroFlight|ECU|Parameters")
	void SetProneHeight(float Height);

	/**
	 * Set player height compensation multiplier (Channel 13)
	 * @param Multiplier - Adjusts proneHeight for player size
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|SuperheroFlight|ECU|Parameters")
	void SetPlayerHeightCompensation(float Multiplier);

	// =====================================
	// Telemetry (ECU → Server)
	// =====================================

	/**
	 * Get dual-winch state from ECU (Channel 310)
	 * @param OutWinchState - Output winch state
	 * @return True if valid winch state was received
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|SuperheroFlight|ECU|Telemetry")
	bool GetDualWinchState(FSuperheroFlightDualWinchState& OutWinchState) const;

	/**
	 * Get system telemetry from ECU (Channel 311)
	 * @param OutTelemetry - Output telemetry
	 * @return True if valid telemetry was received
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|SuperheroFlight|ECU|Telemetry")
	bool GetSystemTelemetry(FSuperheroFlightTelemetry& OutTelemetry) const;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	/** UDP transport for ECU communication */
	UPROPERTY()
	TObjectPtr<UHoloCadeUDPTransport> UDPTransport;

	/** ECU IP address */
	FString ECUIPAddress;

	/** ECU UDP port */
	int32 ECUPort = 8888;

	/** Whether ECU is connected */
	bool bECUConnected = false;

	/** Last received dual-winch state */
	FSuperheroFlightDualWinchState LastWinchState;

	/** Last received system telemetry */
	FSuperheroFlightTelemetry LastTelemetry;

	/** Last time winch state was received */
	float LastWinchStateTime = 0.0f;

	/** Last time telemetry was received */
	float LastTelemetryTime = 0.0f;

	/** Connection timeout in seconds */
	float ConnectionTimeout = 2.0f;

	/** Handle bytes received from UDP transport (delegate callback) */
	UFUNCTION()
	void OnBytesReceived(int32 Channel, TArray<uint8> Data);
};

