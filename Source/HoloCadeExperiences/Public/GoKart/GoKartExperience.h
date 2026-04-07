// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "HoloCadeExperienceBase.h"
#include "GoKart/Models/GoKartVehicleState.h"
#include "GoKartExperience.generated.h"

// Forward declarations
class UGoKartECUController;
class UGoKartTrackGenerator;
class UGoKartItemPickup;
class UGoKartBarrierSystem;
class AGoKartTrackSpline;

/**
 * GoKart Experience Template
 * 
 * Pre-configured go-kart VR experience with passthrough/AR support.
 * Combines:
 * - Real-world go-kart driving on physical track
 * - Virtual weapon/item pickup system
 * - Projectile combat with barrier collision
 * - Throttle control (boost/reduction based on game events)
 * - Shield system (hold item behind kart to block projectiles)
 * - Procedural spline-based track generation
 * - Multiple track support (switchable during debugging)
 * 
 * Perfect for electric go-karts, bumper cars, race boats, or bumper boats
 * augmented by passthrough VR or AR headsets with overlaid virtual weapons and pickups.
 */
UCLASS(Blueprintable, ClassGroup=(HoloCade))
class HOLOCADEEXPERIENCES_API AGoKartExperience : public AHoloCadeExperienceBase
{
	GENERATED_BODY()
	
public:
	AGoKartExperience();

	/** GoKart ECU controller for hardware communication */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HoloCade|GoKart")
	TObjectPtr<UGoKartECUController> ECUController;

	/** Track generator component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HoloCade|GoKart|Track")
	TObjectPtr<UGoKartTrackGenerator> TrackGenerator;

	/** Item pickup system component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HoloCade|GoKart|Items")
	TObjectPtr<UGoKartItemPickup> ItemPickupSystem;

	/** Barrier system component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HoloCade|GoKart|Barriers")
	TObjectPtr<UGoKartBarrierSystem> BarrierSystem;

	/** Array of track splines (can switch between them during debugging) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|GoKart|Track")
	TArray<TObjectPtr<AGoKartTrackSpline>> TrackSplines;

	/** Currently active track spline index */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|GoKart|Track")
	int32 ActiveTrackIndex = 0;

	/** Current vehicle state */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HoloCade|GoKart|Vehicle")
	FGoKartVehicleState VehicleState;

	/** ECU IP address */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|GoKart|ECU")
	FString ECUIPAddress = TEXT("192.168.1.100");

	/** ECU UDP port */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|GoKart|ECU")
	int32 ECUPort = 8888;

	/**
	 * Switch to a different track spline (for debugging)
	 * @param TrackIndex - Index of track to switch to
	 * @return True if switch was successful
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|GoKart|Track")
	bool SwitchTrack(int32 TrackIndex);

	/**
	 * Get current active track spline
	 * @return Active track spline, or nullptr if none
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|GoKart|Track")
	AGoKartTrackSpline* GetActiveTrack() const;

	/**
	 * Apply throttle boost/reduction based on game event
	 * @param Multiplier - Throttle multiplier (1.0 = normal, >1.0 = boost, <1.0 = reduction)
	 * @param Duration - How long the effect lasts (0 = permanent until reset)
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|GoKart|Throttle")
	void ApplyThrottleEffect(float Multiplier, float Duration = 0.0f);

	/**
	 * Reset throttle to normal (1.0 multiplier)
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|GoKart|Throttle")
	void ResetThrottle();

	/**
	 * Get current vehicle state
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|GoKart|Vehicle")
	FGoKartVehicleState GetVehicleState() const { return VehicleState; }

	virtual int32 GetMaxPlayers() const override { return 1; } // Single player for now

protected:
	virtual bool InitializeExperienceImpl() override;
	virtual void ShutdownExperienceImpl() override;
	virtual void Tick(float DeltaTime) override;

private:
	/** Current throttle multiplier effect */
	float CurrentThrottleMultiplier = 1.0f;

	/** Throttle effect duration timer */
	float ThrottleEffectTimer = 0.0f;

	/** Update vehicle state from ECU and tracking */
	void UpdateVehicleState(float DeltaTime);

	/** Handle button events from ECU */
	void HandleButtonEvents();
};

