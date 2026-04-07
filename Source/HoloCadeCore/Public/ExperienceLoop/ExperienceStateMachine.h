// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ExperienceStateMachine.generated.h"

/**
 * Experience state definition
 */
USTRUCT(BlueprintType)
struct FExperienceState
{
	GENERATED_BODY()

	/** Unique identifier for this state */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Experience Loop")
	FName StateName;

	/** Human-readable description of this state */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Experience Loop")
	FString Description;

	/** Can this state be skipped forward? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Experience Loop")
	bool bCanSkipForward = true;

	/** Can this state be rewound backward? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Experience Loop")
	bool bCanSkipBackward = true;

	/** Duration of this state in seconds (0 = infinite) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Experience Loop")
	float Duration = 0.0f;

	/** Optional audio cue for this state */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Experience Loop")
	FString AudioCue;

	FExperienceState()
		: StateName(NAME_None)
		, Description(TEXT(""))
	{}

	FExperienceState(FName InName, const FString& InDescription)
		: StateName(InName)
		, Description(InDescription)
	{}
};

/**
 * Delegate for state change events
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnExperienceStateChanged, FName, OldState, FName, NewState, int32, NewStateIndex);

/**
 * Experience Loop State Machine
 * 
 * Manages the progression of an LBE experience through discrete states.
 * Live actors use wrist-mounted buttons to advance/retreat through the experience.
 * 
 * Example states: Intro -> Tutorial -> Act1 -> Act2 -> Finale -> Credits
 * 
 * Usage:
 * - Define states in your experience template
 * - Map embedded system buttons to AdvanceState/RetreatState
 * - Subscribe to OnStateChanged to trigger game events
 */
UCLASS(BlueprintType, Blueprintable)
class HOLOCADECORE_API UExperienceStateMachine : public UObject
{
	GENERATED_BODY()

public:
	UExperienceStateMachine();

	/** Current state index */
	UPROPERTY(BlueprintReadOnly, Category = "HoloCade|Experience Loop")
	int32 CurrentStateIndex = 0;

	/** All states in this experience */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Experience Loop")
	TArray<FExperienceState> States;

	/** Is the experience currently running? */
	UPROPERTY(BlueprintReadOnly, Category = "HoloCade|Experience Loop")
	bool bIsRunning = false;

	/** Fired when state changes */
	UPROPERTY(BlueprintAssignable, Category = "HoloCade|Experience Loop")
	FOnExperienceStateChanged OnStateChanged;

	/**
	 * Initialize the state machine with states
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Experience Loop")
	void Initialize(const TArray<FExperienceState>& InStates);

	/**
	 * Start the experience from the first state
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Experience Loop")
	void StartExperience();

	/**
	 * Advance to the next state
	 * @return true if successfully advanced
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Experience Loop")
	bool AdvanceState();

	/**
	 * Retreat to the previous state
	 * @return true if successfully retreated
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Experience Loop")
	bool RetreatState();

	/**
	 * Jump to a specific state by name
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Experience Loop")
	bool JumpToState(FName StateName);

	/**
	 * Jump to a specific state by index
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Experience Loop")
	bool JumpToStateIndex(int32 StateIndex);

	/**
	 * Get the current state
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|Experience Loop")
	FExperienceState GetCurrentState() const;

	/**
	 * Get the current state name
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|Experience Loop")
	FName GetCurrentStateName() const;

	/**
	 * Check if we can advance from current state
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|Experience Loop")
	bool CanAdvance() const;

	/**
	 * Check if we can retreat from current state
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|Experience Loop")
	bool CanRetreat() const;

	/**
	 * Reset to the first state
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Experience Loop")
	void ResetExperience();

	/**
	 * Stop the experience
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Experience Loop")
	void StopExperience();

private:
	void BroadcastStateChange(FName OldState, FName NewState);
};



