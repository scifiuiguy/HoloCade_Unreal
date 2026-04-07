// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HoloCadeHMDTypes.h"
#include "HoloCadeTrackingInterface.h"
#include "Networking/HoloCadeServerCommandProtocol.h"
#include "HoloCadeWorldPositionCalibrator.h"
#include "HoloCadeExperienceInterface.h"
#include "HoloCadeExperienceBase.generated.h"

// Forward declarations
class UHoloCadeInputAdapter;
class UExperienceStateMachine;
class UHoloCadeWorldPositionCalibrator;

/**
 * Server mode for multiplayer experiences
 */
UENUM(BlueprintType)
enum class EHoloCadeServerMode : uint8
{
	/** Dedicated server (no local player, headless capable) */
	DedicatedServer UMETA(DisplayName = "Dedicated Server"),
	
	/** Listen server (host player + server) */
	ListenServer UMETA(DisplayName = "Listen Server"),
	
	/** Client only (connect to existing server) */
	Client UMETA(DisplayName = "Client"),
	
	/** Standalone (no networking) */
	Standalone UMETA(DisplayName = "Standalone")
};

/**
 * Base class for all HoloCade Experience Templates
 * 
 * Experience Templates are pre-configured, drag-and-drop solutions that combine
 * multiple HoloCade APIs to create complete LBE experiences. They provide:
 * - Pre-configured hardware setups
 * - Default component arrangements
 * - Blueprint-friendly interfaces
 * - Quick deployment capabilities
 * 
 * Developers can use these as-is or extend them for custom experiences.
 */
UCLASS(Abstract, Blueprintable, BlueprintType, ClassGroup=(HoloCade))
class HOLOCADEEXPERIENCES_API AHoloCadeExperienceBase : public AActor, public IHoloCadeExperienceInterface
{
	GENERATED_BODY()
	
public:	
	AHoloCadeExperienceBase();

	/** HMD configuration for this experience */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Experience")
	FHoloCadeHMDConfig HMDConfig;

	/** Tracking configuration for this experience */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Experience")
	FHoloCadeTrackingConfig TrackingConfig;

	/** Whether to auto-initialize on BeginPlay */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Experience")
	bool bAutoInitialize = true;

	/** Whether this experience supports multiplayer */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Experience")
	bool bMultiplayerEnabled = false;

	/** Server mode (dedicated, listen, client, standalone) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Experience|Networking")
	EHoloCadeServerMode ServerMode = EHoloCadeServerMode::Standalone;

	/** Whether to enforce the required server mode */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Experience|Networking")
	bool bEnforceServerMode = false;

	/** Required server mode (used when bEnforceServerMode is true) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HoloCade|Experience|Networking")
	EHoloCadeServerMode RequiredServerMode = EHoloCadeServerMode::Standalone;

	// ========================================
	// COMPONENTS
	// ========================================

	/** 
	 * Input adapter component for handling all input sources (embedded systems, VR, keyboard, etc.)
	 * Auto-created in constructor. Configure in InitializeExperienceImpl().
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HoloCade|Components")
	TObjectPtr<UHoloCadeInputAdapter> InputAdapter;

	/**
	 * Command protocol for receiving remote commands from Command Console
	 * Auto-created on dedicated server. Processes commands in Tick().
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HoloCade|Components")
	TObjectPtr<UHoloCadeServerCommandProtocol> CommandProtocol;

	/**
	 * Optional narrative state machine for experience flow control
	 * Auto-created if bUseNarrativeStateMachine is true.
	 * Provides discrete state progression (Intro -> Act1 -> Act2 -> Finale, etc.)
	 * Perfect for escape rooms, narrative experiences, and story-driven LBE.
	 * 
	 * Usage:
	 * - Set bUseNarrativeStateMachine = true to enable
	 * - Define states in InitializeExperienceImpl() or Blueprint
	 * - Subscribe to OnNarrativeStateChanged to trigger game events
	 * - Use AdvanceNarrativeState() / RetreatNarrativeState() to control flow
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HoloCade|Components|Narrative")
	TObjectPtr<UExperienceStateMachine> NarrativeStateMachine;

	/**
	 * World position calibrator for drift correction
	 * Auto-created for all experiences. Provides VR 6DOF drag/drop calibration.
	 * Allows Ops Tech to quickly recalibrate if tracking drift occurs throughout the day.
	 * 
	 * Usage:
	 * - Player trigger-holds any part of virtual world
	 * - System automatically detects horizontal or vertical drag axis
	 * - Constrains to that axis as virtual world recalibrates origin offset
	 * - Releases when trigger is released
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HoloCade|Components|Calibration")
	TObjectPtr<class UHoloCadeWorldPositionCalibrator> WorldPositionCalibrator;

	/** Whether to enable narrative state machine for this experience */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Experience|Narrative")
	bool bUseNarrativeStateMachine = false;

	/**
	 * Initialize the experience
	 * Called automatically if bAutoInitialize is true, or manually by developer
	 * @return true if initialization was successful
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Experience")
	virtual bool InitializeExperience();

	/**
	 * Shutdown the experience and cleanup resources
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Experience")
	virtual void ShutdownExperience();

	/**
	 * Check if the experience is currently active and initialized
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|Experience")
	bool IsExperienceActive() const { return bIsInitialized; }

	/**
	 * Get the number of players this experience supports
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|Experience")
	virtual int32 GetMaxPlayers() const { return 1; }

	// ========================================
	// NARRATIVE STATE MACHINE API
	// ========================================

	/**
	 * Get the narrative state machine (if enabled)
	 * @return State machine component, or nullptr if not enabled
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|Experience|Narrative")
	UExperienceStateMachine* GetNarrativeStateMachine() const;

	/**
	 * Get the current narrative state name
	 * @return Current state name, or NAME_None if state machine not enabled
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|Experience|Narrative")
	FName GetCurrentNarrativeState() const;

	/**
	 * Advance to the next narrative state
	 * @return true if successfully advanced
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Experience|Narrative")
	bool AdvanceNarrativeState();

	/**
	 * Retreat to the previous narrative state
	 * @return true if successfully retreated
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Experience|Narrative")
	bool RetreatNarrativeState();

	/**
	 * Jump to a specific narrative state by name
	 * @param StateName - Name of the state to jump to
	 * @return true if successfully jumped
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Experience|Narrative")
	bool JumpToNarrativeState(FName StateName);

	/**
	 * Event fired when narrative state changes
	 * Override in Blueprint or C++ to handle state transitions
	 * 
	 * @param OldState - Previous state name
	 * @param NewState - New state name
	 * @param NewStateIndex - Index of the new state
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "HoloCade|Experience|Narrative")
	void OnNarrativeStateChanged(FName OldState, FName NewState, int32 NewStateIndex);

	// ========================================
	// IHoloCadeExperienceInterface Implementation
	// ========================================

	/** Get the InputAdapter component (implements IHoloCadeExperienceInterface) */
	virtual UHoloCadeInputAdapter* GetInputAdapter() const override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Whether the experience has been initialized */
	bool bIsInitialized = false;

	/**
	 * Override this to perform custom initialization logic
	 */
	virtual bool InitializeExperienceImpl();

	/**
	 * Override this to perform custom shutdown logic
	 */
	virtual void ShutdownExperienceImpl();

	virtual void Tick(float DeltaTime) override;

	/**
	 * Handle incoming command from Command Console
	 * Override this to handle custom commands in derived classes
	 */
	UFUNCTION()
	virtual void OnCommandReceived(const FHoloCadeServerCommandMessage& Command, UHoloCadeServerCommandProtocol* Protocol);

	/**
	 * Initialize command protocol for dedicated server mode
	 */
	void InitializeCommandProtocol();

	/**
	 * Internal handler for narrative state changes (binds to state machine delegate)
	 */
	UFUNCTION()
	void HandleNarrativeStateChanged(FName OldState, FName NewState, int32 NewStateIndex);
};

