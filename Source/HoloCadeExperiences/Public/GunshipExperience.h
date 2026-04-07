// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "HoloCadeExperienceBase.h"
#include "Models/TiltState.h"
#include "Models/ScissorLiftState.h"
#include "GunshipExperience.generated.h"

// Forward declarations
class U4DOFPlatformController;

/**
 * 4DOF Gunship Experience Template
 * 
 * Pre-configured four-player seated VR experience on hydraulic platform.
 * Combines:
 * - 4DOF motion platform:
 *   - Hydraulic platform: pitch, roll (yaw restricted)
 *   - Scissor lift: forward/reverse, up/down
 * - Four player seated positions
 * - LAN multiplayer support
 * - Synchronized motion for all players
 * 
 * Perfect for gunship, helicopter, spaceship, or any multi-crew vehicle
 * experiences requiring shared motion simulation.
 */
UCLASS(Blueprintable, ClassGroup=(HoloCade))
class HOLOCADEEXPERIENCES_API AGunshipExperience : public AHoloCadeExperienceBase
{
	GENERATED_BODY()
	
public:
	AGunshipExperience();

	/** 4DOF platform controller (specialized for Gunship, MovingPlatform, CarSim) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HoloCade|Gunship")
	TObjectPtr<U4DOFPlatformController> PlatformController;

	/** Player seat locations (4 seats) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Gunship")
	TArray<FVector> SeatLocations;

	/** Maximum pitch angle in degrees */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Gunship", meta = (ClampMin = "1.0", ClampMax = "15.0"))
	float MaxPitch = 10.0f;

	/** Maximum roll angle in degrees */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Gunship", meta = (ClampMin = "1.0", ClampMax = "15.0"))
	float MaxRoll = 10.0f;

	/**
	 * Send normalized gunship motion (RECOMMENDED FOR GAME CODE)
	 * Uses joystick-style input that automatically scales to hardware capabilities.
	 * 
	 * @param TiltX - Left/Right roll (-1.0 = full left, +1.0 = full right, 0.0 = level)
	 * @param TiltY - Forward/Backward pitch (-1.0 = full backward, +1.0 = full forward, 0.0 = level)
	 * @param ForwardOffset - Scissor lift forward/reverse (-1.0 = full reverse, +1.0 = full forward, 0.0 = neutral)
	 * @param VerticalOffset - Scissor lift up/down (-1.0 = full down, +1.0 = full up, 0.0 = neutral)
	 * @param Duration - Time to reach target (seconds)
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Gunship")
	void SendGunshipTilt(float TiltX, float TiltY, float ForwardOffset = 0.0f, float VerticalOffset = 0.0f, float Duration = 1.0f);

	/**
	 * Send motion command to platform (ADVANCED - uses absolute angles)
	 * For most game code, use SendGunshipTilt() instead.
	 * 
	 * @param Pitch - Platform pitch angle in degrees
	 * @param Roll - Platform roll angle in degrees
	 * @param ForwardOffset - Scissor lift forward/reverse translation in cm (positive = forward)
	 * @param VerticalOffset - Scissor lift up/down translation in cm (positive = up)
	 * @param Duration - Time to reach target (seconds)
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Gunship|Advanced")
	void SendGunshipMotion(float Pitch, float Roll, float ForwardOffset, float VerticalOffset, float Duration = 1.0f);

	/**
	 * Return platform to neutral
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Gunship")
	void ReturnToNeutral(float Duration = 2.0f);

	/**
	 * Emergency stop
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Gunship")
	void EmergencyStop();

	virtual int32 GetMaxPlayers() const override { return 4; }

protected:
	virtual bool InitializeExperienceImpl() override;
	virtual void ShutdownExperienceImpl() override;
};

