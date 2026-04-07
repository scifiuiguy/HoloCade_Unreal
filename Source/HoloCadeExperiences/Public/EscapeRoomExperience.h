// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "HoloCadeExperienceBase.h"
#include "EscapeRoomExperience.generated.h"

// Forward declarations
class UEmbeddedDeviceController;

/**
 * Escape Room Experience Template
 * 
 * Pre-configured experience for escape room installations with embedded systems integration.
 * Perfect for interactive escape rooms, puzzle experiences, and narrative-driven LBE.
 * 
 * Features:
 * - Narrative state machine for story progression (Intro -> Puzzle1 -> Puzzle2 -> Finale)
 * - Embedded systems integration for door locks, latches, and prop controls
 * - Wireless communication examples for all microcontroller platforms
 * - Example firmware sketches for unlocking mechanisms
 * - Multi-device support for complex room setups
 * 
 * Embedded Systems Integration:
 * This template demonstrates how to integrate microcontroller firmware for:
 * - Door/latch unlocking via wireless commands
 * - Sensor reading (pressure plates, motion sensors, etc.)
 * - LED feedback and status indicators
 * - Haptic feedback for props
 * 
 * See FirmwareExamples/ for example firmware sketches.
 */
UCLASS(Blueprintable, ClassGroup=(HoloCade))
class HOLOCADEEXPERIENCES_API AEscapeRoomExperience : public AHoloCadeExperienceBase
{
	GENERATED_BODY()
	
public:
	AEscapeRoomExperience();

	/** Embedded device controller for door locks and props */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HoloCade|Escape Room|Components")
	TObjectPtr<UEmbeddedDeviceController> DoorController;

	/** Optional second embedded device for additional props */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HoloCade|Escape Room|Components")
	TObjectPtr<UEmbeddedDeviceController> PropController;

	/** Number of doors/locks in this escape room */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Escape Room|Config", meta = (ClampMin = "1", ClampMax = "16"))
	int32 NumberOfDoors = 4;

	/** Number of props with embedded systems */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Escape Room|Config", meta = (ClampMin = "0", ClampMax = "16"))
	int32 NumberOfProps = 2;

	/**
	 * Mapping of narrative states to door indices for automatic unlocking
	 * When a state is reached, the corresponding door will be automatically unlocked.
	 * Leave empty to disable automatic unlocking, or override OnNarrativeStateChanged for custom logic.
	 * 
	 * Example: Map "Puzzle1" state to door 0, "Puzzle2" to door 1, etc.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Escape Room|Config|Narrative")
	TMap<FName, int32> StateToDoorMapping;

	/**
	 * Unlock a door by index
	 * Sends unlock command to embedded device via wireless communication
	 * 
	 * @param DoorIndex - Index of door to unlock (0-based)
	 * @return true if command was sent successfully
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Escape Room|Doors")
	bool UnlockDoor(int32 DoorIndex);

	/**
	 * Lock a door by index
	 * Sends lock command to embedded device via wireless communication
	 * 
	 * @param DoorIndex - Index of door to lock (0-based)
	 * @return true if command was sent successfully
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Escape Room|Doors")
	bool LockDoor(int32 DoorIndex);

	/**
	 * Check if a door is unlocked
	 * Reads state from embedded device
	 * 
	 * @param DoorIndex - Index of door to check (0-based)
	 * @return true if door is unlocked
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|Escape Room|Doors")
	bool IsDoorUnlocked(int32 DoorIndex) const;

	/**
	 * Trigger a prop action (e.g., open drawer, activate mechanism)
	 * Sends command to prop controller via wireless communication
	 * 
	 * @param PropIndex - Index of prop to activate (0-based)
	 * @param ActionValue - Action value (0.0-1.0 for intensity, or specific command)
	 * @return true if command was sent successfully
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Escape Room|Props")
	bool TriggerPropAction(int32 PropIndex, float ActionValue = 1.0f);

	/**
	 * Read sensor value from a prop
	 * Reads analog/digital sensor state from embedded device
	 * 
	 * @param PropIndex - Index of prop sensor to read (0-based)
	 * @return Sensor value (0.0-1.0 normalized, or raw value depending on sensor type)
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|Escape Room|Props")
	float ReadPropSensor(int32 PropIndex) const;

	/**
	 * Get the current narrative state (from state machine)
	 * Useful for triggering door unlocks based on puzzle completion
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|Escape Room|Narrative")
	FName GetCurrentPuzzleState() const;

	/**
	 * Delegate fired when a door unlock is confirmed by the embedded device
	 * This callback is triggered when the firmware sends back confirmation that the door actually unlocked.
	 * 
	 * @param DoorIndex - Index of the door that was unlocked
	 */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDoorUnlockConfirmed, int32, DoorIndex);

	/** Event fired when a door unlock is confirmed by the embedded device */
	UPROPERTY(BlueprintAssignable, Category = "HoloCade|Escape Room|Doors")
	FOnDoorUnlockConfirmed OnDoorUnlockConfirmed;

protected:
	virtual bool InitializeExperienceImpl() override;
	virtual void ShutdownExperienceImpl() override;

	/**
	 * Handle narrative state changes and unlock doors based on puzzle completion
	 * This implements the BlueprintImplementableEvent from the base class
	 * Note: No UFUNCTION macro needed - inherits from base class declaration
	 */
	void OnNarrativeStateChanged(FName OldState, FName NewState, int32 NewStateIndex);

	/**
	 * Initialize embedded device controllers for doors and props
	 */
	void InitializeEmbeddedDevices();

	/**
	 * Get the door index mapped to a narrative state
	 * @param StateName - Name of the narrative state
	 * @return Door index if mapped, or -1 if no mapping exists
	 */
	int32 GetDoorIndexForState(FName StateName) const;

	/**
	 * Handle door unlock events from embedded devices
	 */
	UFUNCTION()
	void OnDoorStateChanged(int32 Channel, bool bIsUnlocked);

	/**
	 * Handle prop sensor readings from embedded devices
	 */
	UFUNCTION()
	void OnPropSensorValue(int32 Channel, float Value);

	/** Track door unlock states (cached from embedded devices) */
	UPROPERTY()
	TArray<bool> DoorUnlockStates;

	/** Track prop sensor values (cached from embedded devices) */
	UPROPERTY()
	TArray<float> PropSensorValues;
};

