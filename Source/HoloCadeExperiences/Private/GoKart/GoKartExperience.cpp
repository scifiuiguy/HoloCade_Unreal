// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "GoKart/GoKartExperience.h"
#include "GoKart/GoKartECUController.h"
#include "GoKart/GoKartTrackGenerator.h"
#include "GoKart/GoKartItemPickup.h"
#include "GoKart/GoKartBarrierSystem.h"
#include "GoKart/GoKartTrackSpline.h"
#include "HoloCadeExperiences.h"

AGoKartExperience::AGoKartExperience()
{
	ECUController = CreateDefaultSubobject<UGoKartECUController>(TEXT("ECUController"));
	TrackGenerator = CreateDefaultSubobject<UGoKartTrackGenerator>(TEXT("TrackGenerator"));
	ItemPickupSystem = CreateDefaultSubobject<UGoKartItemPickup>(TEXT("ItemPickupSystem"));
	BarrierSystem = CreateDefaultSubobject<UGoKartBarrierSystem>(TEXT("BarrierSystem"));
	bMultiplayerEnabled = false; // Single player for now
	ActiveTrackIndex = 0;
	CurrentThrottleMultiplier = 1.0f;
	ThrottleEffectTimer = 0.0f;
}

bool AGoKartExperience::InitializeExperienceImpl()
{
	if (!Super::InitializeExperienceImpl())
	{
		return false;
	}

	// Initialize ECU connection
	if (ECUController)
	{
		if (!ECUController->InitializeECU(ECUIPAddress, ECUPort))
		{
			UE_LOG(LogGoKart, Error, TEXT("GoKartExperience: Failed to initialize ECU"));
			return false;
		}
	}

	// Initialize track generator
	if (TrackGenerator)
	{
		// NOOP: Track generation will be implemented
		// TrackGenerator->InitializeTrack(GetActiveTrack());
	}

	// Initialize item pickup system
	if (ItemPickupSystem)
	{
		// NOOP: Item system initialization will be implemented
		// ItemPickupSystem->InitializeItems(GetActiveTrack());
	}

	// Initialize barrier system
	if (BarrierSystem)
	{
		// NOOP: Barrier system initialization will be implemented
		// BarrierSystem->InitializeBarriers(GetActiveTrack());
	}

	UE_LOG(LogGoKart, Log, TEXT("GoKartExperience: Initialized"));
	return true;
}

void AGoKartExperience::ShutdownExperienceImpl()
{
	if (ECUController)
	{
		ECUController->EmergencyStop();
		ECUController->ShutdownECU();
	}

	Super::ShutdownExperienceImpl();
}

void AGoKartExperience::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Update throttle effect timer
	if (ThrottleEffectTimer > 0.0f)
	{
		ThrottleEffectTimer -= DeltaTime;
		if (ThrottleEffectTimer <= 0.0f)
		{
			ResetThrottle();
		}
	}

	// Update vehicle state
	UpdateVehicleState(DeltaTime);

	// Handle button events
	HandleButtonEvents();
}

bool AGoKartExperience::SwitchTrack(int32 TrackIndex)
{
	if (TrackIndex < 0 || TrackIndex >= TrackSplines.Num())
	{
		UE_LOG(LogGoKart, Warning, TEXT("GoKartExperience: Invalid track index %d"), TrackIndex);
		return false;
	}

	ActiveTrackIndex = TrackIndex;
	
	// NOOP: Will regenerate track, items, and barriers for new spline
	if (TrackGenerator)
	{
		// TrackGenerator->RegenerateTrack(GetActiveTrack());
	}
	if (ItemPickupSystem)
	{
		// ItemPickupSystem->RegenerateItems(GetActiveTrack());
	}
	if (BarrierSystem)
	{
		// BarrierSystem->RegenerateBarriers(GetActiveTrack());
	}

	UE_LOG(LogGoKart, Log, TEXT("GoKartExperience: Switched to track %d"), TrackIndex);
	return true;
}

AGoKartTrackSpline* AGoKartExperience::GetActiveTrack() const
{
	if (ActiveTrackIndex >= 0 && ActiveTrackIndex < TrackSplines.Num())
	{
		return TrackSplines[ActiveTrackIndex];
	}
	return nullptr;
}

void AGoKartExperience::ApplyThrottleEffect(float Multiplier, float Duration)
{
	CurrentThrottleMultiplier = FMath::Clamp(Multiplier, 0.0f, 2.0f);
	ThrottleEffectTimer = Duration;

	if (ECUController)
	{
		ECUController->SetThrottleMultiplier(CurrentThrottleMultiplier);
	}
}

void AGoKartExperience::ResetThrottle()
{
	CurrentThrottleMultiplier = 1.0f;
	ThrottleEffectTimer = 0.0f;

	if (ECUController)
	{
		ECUController->SetThrottleMultiplier(1.0f);
	}
}

void AGoKartExperience::UpdateVehicleState(float DeltaTime)
{
	// NOOP: Will update vehicle state from:
	// - ECU throttle feedback
	// - SteamVR tracker position/rotation
	// - Track spline progress calculation
	// - Current item state
	// - Shield state

	if (ECUController && ECUController->IsECUConnected())
	{
		FGoKartThrottleState ThrottleState;
		if (ECUController->GetThrottleStateFeedback(ThrottleState))
		{
			VehicleState.ThrottleState = ThrottleState;
			VehicleState.bECUConnected = true;
			VehicleState.LastECUUpdateTime = GetWorld()->GetTimeSeconds();
		}
	}
	else
	{
		VehicleState.bECUConnected = false;
	}
}

void AGoKartExperience::HandleButtonEvents()
{
	// NOOP: Will handle button events from ECU:
	// - Horn button (audio/visual feedback)
	// - Shield button (long-press detection, activate shield if item allows)

	if (ECUController && ECUController->IsECUConnected())
	{
		FGoKartButtonEvents ButtonEvents;
		if (ECUController->GetButtonEvents(ButtonEvents))
		{
			// Handle horn button
			if (ButtonEvents.HornButtonState)
			{
				// NOOP: Play horn sound, visual feedback
			}

			// Handle shield button (long-press)
			if (ButtonEvents.ShieldButtonState)
			{
				// NOOP: Activate shield if player has item that supports shield
				VehicleState.bShieldActive = true;
			}
			else
			{
				VehicleState.bShieldActive = false;
			}
		}
	}
}

