// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Networking/HoloCadeUDPTransport.h"
#include "GoKart/Models/GoKartButtonEvents.h"
#include "GoKart/Models/GoKartThrottleState.h"
#include "GoKartECUController.generated.h"

/**
 * GoKart ECU Controller
 * 
 * Handles UDP communication with GoKartExperience_ECU firmware.
 * Manages throttle control (man-in-the-middle), button events, and vehicle state.
 * 
 * Similar to U4DOFPlatformController but specialized for GoKart hardware:
 * - Throttle control (boost/reduction)
 * - Horn button with LED
 * - Shield button (long-press)
 * - Vehicle telemetry
 * 
 * Communication Protocol:
 * - Server → ECU: Throttle commands, game state
 * - ECU → Server: Button events, throttle feedback, vehicle telemetry
 */
UCLASS(ClassGroup=(HoloCade), meta=(BlueprintSpawnableComponent))
class HOLOCADEEXPERIENCES_API UGoKartECUController : public UActorComponent
{
	GENERATED_BODY()

public:
	UGoKartECUController();
	virtual ~UGoKartECUController();

	/**
	 * Initialize connection to GoKart ECU
	 * @param ECUIPAddress - IP address of the GoKart ECU
	 * @param ECUPort - UDP port for ECU communication
	 * @return True if initialization successful
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|GoKart|ECU")
	bool InitializeECU(const FString& ECUIPAddress, int32 ECUPort = 8888);

	/**
	 * Shutdown ECU connection
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|GoKart|ECU")
	void ShutdownECU();

	/**
	 * Check if ECU is connected
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|GoKart|ECU")
	bool IsECUConnected() const;

	// =====================================
	// Throttle Control (Server → ECU)
	// =====================================

	/**
	 * Send throttle multiplier to ECU (man-in-the-middle control)
	 * @param Multiplier - Throttle multiplier (1.0 = normal, >1.0 = boost, <1.0 = reduction)
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|GoKart|ECU|Throttle")
	void SetThrottleMultiplier(float Multiplier);

	/**
	 * Send complete throttle state to ECU
	 * @param ThrottleState - Complete throttle state (input, multiplier, output)
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|GoKart|ECU|Throttle")
	void SendThrottleState(const FGoKartThrottleState& ThrottleState);

	// =====================================
	// Game State (Server → ECU)
	// =====================================

	/**
	 * Set play session active state (controls whether kart can operate)
	 * @param bActive - True if play session is active
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|GoKart|ECU|GameState")
	void SetPlaySessionActive(bool bActive);

	/**
	 * Send emergency stop command
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|GoKart|ECU|GameState")
	void EmergencyStop();

	// =====================================
	// Button Events (ECU → Server)
	// =====================================

	/**
	 * Get button events from ECU (Channel 310)
	 * @param OutButtonEvents - Output button events
	 * @return True if valid button events were received
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|GoKart|ECU|Buttons")
	bool GetButtonEvents(FGoKartButtonEvents& OutButtonEvents) const;

	/**
	 * Get throttle state feedback from ECU (Channel 311)
	 * @param OutThrottleState - Output throttle state
	 * @return True if valid throttle state was received
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|GoKart|ECU|Throttle")
	bool GetThrottleStateFeedback(FGoKartThrottleState& OutThrottleState) const;

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

	/** Last time button events were received */
	float LastButtonEventTime = 0.0f;

	/** Last time throttle feedback was received */
	float LastThrottleFeedbackTime = 0.0f;

	/** Connection timeout in seconds */
	float ConnectionTimeout = 2.0f;

	/** Process received UDP data */
	void ProcessReceivedData(const TArray<uint8>& Data);
};

