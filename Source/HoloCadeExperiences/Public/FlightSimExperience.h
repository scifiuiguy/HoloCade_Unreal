// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "HoloCadeExperienceBase.h"
#include "2DOFGyroPlatformController.h"
#include "FlightSimExperience.generated.h"

/**
 * 2DOF Full 360 Flight Sim Experience Template
 * 
 * Pre-configured single-player flight simulator with gyroscope and HOTAS.
 * Combines:
 * - 2DOF gyroscope system (continuous pitch and roll beyond 360°)
 * - HOTAS controller integration (Logitech X56 or Thrustmaster T.Flight)
 * - Secured cockpit with safety harness
 * - No hydraulics - pure gyroscopic rotation
 * 
 * Perfect for realistic flight arcade games, space combat simulators,
 * and any experience requiring full 360° continuous rotation.
 * 
 * ⚠️ IMPORTANT: Space Reset Feature Tracking Requirements
 * 
 * If using the Space Reset feature (bSpaceReset), you MUST use outside-in tracking
 * with trackers mounted to the cockpit frame. HoloCade does NOT provide HMD correction
 * for Space Reset - this is a complex problem that requires cockpit-relative tracking.
 * 
 * Recommended: Bigscreen Beyond 2 + SteamVR Lighthouse with base stations mounted
 * to the cockpit frame (not room walls).
 * 
 * See: FirmwareExamples/FlightSimExperience/README.md for detailed tracking setup
 * instructions and warnings about HMD correction complexity.
 */
UCLASS(Blueprintable, ClassGroup=(HoloCade))
class HOLOCADEEXPERIENCES_API AFlightSimExperience : public AHoloCadeExperienceBase
{
	GENERATED_BODY()
	
public:
	AFlightSimExperience();

	/** Gyroscope platform controller */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HoloCade|Flight Sim")
	TObjectPtr<U2DOFGyroPlatformController> GyroscopeController;

	/** HOTAS controller type */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Flight Sim|HOTAS")
	EHoloCadeHOTASType HOTASType = EHoloCadeHOTASType::LogitechX56;

	/** Enable HOTAS joystick */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Flight Sim|HOTAS")
	bool bEnableJoystick = true;

	/** Enable HOTAS throttle */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Flight Sim|HOTAS")
	bool bEnableThrottle = true;

	/** Enable HOTAS pedals */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Flight Sim|HOTAS")
	bool bEnablePedals = false;

	/** Joystick sensitivity */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Flight Sim|HOTAS", meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float JoystickSensitivity = 1.5f;

	/** Throttle sensitivity */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Flight Sim|HOTAS", meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float ThrottleSensitivity = 1.0f;

	/** Maximum rotation speed in degrees per second */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Flight Sim", meta = (ClampMin = "10.0", ClampMax = "180.0"))
	float MaxRotationSpeed = 90.0f;

	/**
	 * If true, the ECU will smoothstep the gyros toward world-up (0° pitch, 0° roll).
	 * Intended for gravity-based reset when input is idle. Sent to ECU on connect.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Flight Sim|Reset")
	bool bGravityReset = true;

	/**
	 * Speed used by ECU for gravity reset smoothing (degrees/second equivalency).
	 * Exposed as a slider for tech artists to calibrate. Sent to ECU on connect.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Flight Sim|Reset", meta = (ClampMin = "1.0", ClampMax = "180.0", UIMin = "1.0", UIMax = "180.0"))
	float ResetSpeed = 45.0f;

	/**
	 * Idle timeout (seconds). If HOTAS input is idle for this duration, ECU smoothsteps back to zero.
	 * Sent to ECU on connect.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Flight Sim|Reset", meta = (ClampMin = "0.0", ClampMax = "10.0", UIMin = "0.0", UIMax = "10.0"))
	float ResetIdleTimeout = 1.5f;

	/**
	 * If true, virtual cockpit transform decouples from physical cockpit during gravity reset.
	 * This simulates zero-gravity space by smoothly interpolating physical back to zero without
	 * rotating the player's virtual cockpit. The cockpit visual remains fixed (assuming no stick input)
	 * until the ECU reports the platform is back at zero and gravityReset is turned off.
	 * NOTE: This does not yet apply HMD correction; that will be handled separately.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Flight Sim|Reset")
	bool bSpaceReset = false;

	/** Cockpit actor to decouple/recouple during space reset (assigned by tech art) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Flight Sim|Reset")
	AActor* CockpitActor = nullptr;

	/** Degrees threshold near zero to consider the physical platform reset */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Flight Sim|Reset", meta = (ClampMin = "0.1", ClampMax = "10.0"))
	float ZeroThresholdDegrees = 2.0f;

	/**
	 * Send continuous rotation command
	 * @param Pitch - Target pitch (can exceed 360°)
	 * @param Roll - Target roll (can exceed 360°)
	 * @param Duration - Time to complete rotation
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Flight Sim")
	void SendContinuousRotation(float Pitch, float Roll, float Duration);

	/**
	 * Get current HOTAS joystick input
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|Flight Sim|HOTAS")
	FVector2D GetJoystickInput() const;

	/**
	 * Get current HOTAS throttle input
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|Flight Sim|HOTAS")
	float GetThrottleInput() const;

	/**
	 * Get current HOTAS pedal input
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|Flight Sim|HOTAS")
	float GetPedalInput() const;

	/**
	 * Check if HOTAS is connected
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|Flight Sim|HOTAS")
	bool IsHOTASConnected() const;

	/**
	 * Return gyroscope to neutral position
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Flight Sim")
	void ReturnToNeutral(float Duration = 3.0f);

	/**
	 * Emergency stop
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Flight Sim")
	void EmergencyStop();

protected:
	virtual bool InitializeExperienceImpl() override;
	virtual void ShutdownExperienceImpl() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	/** Maintain cockpit transform sync/decouple behavior for space reset */
	void UpdateCockpitTransform(float DeltaSeconds);

	/** Cached cockpit rotation used while decoupled */
	FRotator DecoupledCockpitRotation = FRotator::ZeroRotator;

	/** Whether cockpit is currently decoupled from physical platform */
	bool bCockpitDecoupled = false;
};

