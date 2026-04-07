// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "HoloCadeExperienceBase.h"
#include "CarSimExperience.generated.h"

// Forward declarations
class U4DOFPlatformController;

/**
 * 5DOF Car Sim Experience Template
 * 
 * Pre-configured single-player racing/driving simulator on hydraulic platform.
 * Combines:
 * - 5DOF hydraulic platform (pitch, roll, Y/Z translation)
 * - Cockpit seating position
 * - Racing wheel and pedal integration support
 * - Motion profiles optimized for driving
 * 
 * Perfect for arcade racing games, driving simulators, and car-themed experiences.
 */
UCLASS(Blueprintable, ClassGroup=(HoloCade))
class HOLOCADEEXPERIENCES_API ACarSimExperience : public AHoloCadeExperienceBase
{
	GENERATED_BODY()
	
public:
	ACarSimExperience();

	/** 4DOF platform controller (specialized for Gunship, MovingPlatform, CarSim) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HoloCade|Car Sim")
	TObjectPtr<U4DOFPlatformController> PlatformController;

	/** Maximum pitch angle in degrees (for acceleration/braking) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Car Sim", meta = (ClampMin = "1.0", ClampMax = "15.0"))
	float MaxPitch = 10.0f;

	/** Maximum roll angle in degrees (for cornering) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Car Sim", meta = (ClampMin = "1.0", ClampMax = "15.0"))
	float MaxRoll = 10.0f;

	/**
	 * Simulate cornering motion (RECOMMENDED - normalized input)
	 * @param TurnIntensity - Normalized turn intensity (-1.0 = full left, +1.0 = full right, 0.0 = straight)
	 * @param Duration - Transition time
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Car Sim")
	void SimulateCornerNormalized(float TurnIntensity, float Duration = 0.5f);

	/**
	 * Simulate acceleration/braking motion (RECOMMENDED - normalized input)
	 * @param AccelIntensity - Normalized acceleration (-1.0 = full brake, +1.0 = full acceleration, 0.0 = neutral)
	 * @param Duration - Transition time
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Car Sim")
	void SimulateAccelerationNormalized(float AccelIntensity, float Duration = 0.5f);

	/**
	 * Simulate cornering motion (ADVANCED - uses absolute angles)
	 * @param LeanAngle - Negative for left turn, positive for right turn (degrees)
	 * @param Duration - Transition time
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Car Sim|Advanced")
	void SimulateCorner(float LeanAngle, float Duration = 0.5f);

	/**
	 * Simulate acceleration/braking motion (ADVANCED - uses absolute angles)
	 * @param PitchAngle - Positive for acceleration, negative for braking (degrees)
	 * @param Duration - Transition time
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Car Sim|Advanced")
	void SimulateAcceleration(float PitchAngle, float Duration = 0.5f);

	/**
	 * Simulate road bumps
	 * @param Intensity - Bump intensity (0-1)
	 * @param Duration - Duration of bump effect
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Car Sim")
	void SimulateBump(float Intensity, float Duration = 0.2f);

	/**
	 * Return to neutral position
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Car Sim")
	void ReturnToNeutral(float Duration = 1.0f);

	/**
	 * Emergency stop
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Car Sim")
	void EmergencyStop();

protected:
	virtual bool InitializeExperienceImpl() override;
	virtual void ShutdownExperienceImpl() override;
};

