// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "HoloCadeExperienceBase.h"
#include "MovingPlatformExperience.generated.h"

// Forward declarations
class U4DOFPlatformController;

/**
 * 5DOF Moving Platform Experience Template
 * 
 * Pre-configured single-player standing VR experience on hydraulic platform.
 * Combines:
 * - 5DOF hydraulic platform (pitch, roll, Y/Z translation)
 * - Safety harness integration
 * - Standing player position
 * - Configurable motion profiles
 * 
 * Perfect for experiences requiring unstable ground simulation, earthquakes,
 * ship decks, moving vehicles, or any standing VR experience with motion.
 */
UCLASS(Blueprintable, ClassGroup=(HoloCade))
class HOLOCADEEXPERIENCES_API AMovingPlatformExperience : public AHoloCadeExperienceBase
{
	GENERATED_BODY()
	
public:
	AMovingPlatformExperience();

	/** 4DOF platform controller (specialized for Gunship, MovingPlatform, CarSim) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HoloCade|Moving Platform")
	TObjectPtr<U4DOFPlatformController> PlatformController;

	/** Maximum pitch angle in degrees */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Moving Platform", meta = (ClampMin = "1.0", ClampMax = "15.0"))
	float MaxPitch = 10.0f;

	/** Maximum roll angle in degrees */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Moving Platform", meta = (ClampMin = "1.0", ClampMax = "15.0"))
	float MaxRoll = 10.0f;

	/** Maximum vertical translation in cm */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Moving Platform", meta = (ClampMin = "10.0", ClampMax = "200.0"))
	float MaxVerticalTranslation = 100.0f;

	/**
	 * Send normalized platform tilt (RECOMMENDED FOR GAME CODE)
	 * Uses joystick-style input that automatically scales to hardware capabilities.
	 * This is hardware-agnostic - the same values work on platforms with different tilt ranges.
	 * 
	 * @param TiltX - Left/Right tilt (-1.0 = full left, +1.0 = full right, 0.0 = level)
	 * @param TiltY - Forward/Backward tilt (-1.0 = full backward, +1.0 = full forward, 0.0 = level)
	 * @param VerticalOffset - Vertical translation (-1.0 to +1.0, normalized to max capability)
	 * @param Duration - Time to reach target (seconds)
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Moving Platform")
	void SendPlatformTilt(float TiltX, float TiltY, float VerticalOffset = 0.0f, float Duration = 1.0f);

	/**
	 * Send motion command to platform (ADVANCED - uses absolute angles)
	 * Only use this if you need precise control and know the hardware capabilities.
	 * For most game code, use SendPlatformTilt() instead.
	 * 
	 * @param Pitch - Target pitch angle in degrees
	 * @param Roll - Target roll angle in degrees
	 * @param VerticalOffset - Vertical translation in cm
	 * @param Duration - Time to reach target (seconds)
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Moving Platform|Advanced")
	void SendPlatformMotion(float Pitch, float Roll, float VerticalOffset, float Duration = 1.0f);

	/**
	 * Return platform to neutral position
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Moving Platform")
	void ReturnToNeutral(float Duration = 2.0f);

	/**
	 * Emergency stop platform motion
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Moving Platform")
	void EmergencyStop();

protected:
	virtual bool InitializeExperienceImpl() override;
	virtual void ShutdownExperienceImpl() override;
};

