// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "ExperienceLoop/ExperienceStateMachine.h"

UExperienceStateMachine::UExperienceStateMachine()
{
	CurrentStateIndex = 0;
	bIsRunning = false;
}

void UExperienceStateMachine::Initialize(const TArray<FExperienceState>& InStates)
{
	States = InStates;
	CurrentStateIndex = 0;
	bIsRunning = false;

	if (States.Num() > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("ExperienceStateMachine: Initialized with %d states"), States.Num());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ExperienceStateMachine: Initialized with no states"));
	}
}

void UExperienceStateMachine::StartExperience()
{
	if (States.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("ExperienceStateMachine: Cannot start - no states defined"));
		return;
	}

	CurrentStateIndex = 0;
	bIsRunning = true;

	FName InitialState = States[0].StateName;
	UE_LOG(LogTemp, Log, TEXT("ExperienceStateMachine: Started at state '%s'"), *InitialState.ToString());
	
	BroadcastStateChange(NAME_None, InitialState);
}

bool UExperienceStateMachine::AdvanceState()
{
	if (!bIsRunning)
	{
		UE_LOG(LogTemp, Warning, TEXT("ExperienceStateMachine: Cannot advance - experience not running"));
		return false;
	}

	if (!CanAdvance())
	{
		UE_LOG(LogTemp, Warning, TEXT("ExperienceStateMachine: Cannot advance from current state"));
		return false;
	}

	FName OldState = GetCurrentStateName();
	CurrentStateIndex++;
	FName NewState = GetCurrentStateName();

	UE_LOG(LogTemp, Log, TEXT("ExperienceStateMachine: Advanced from '%s' to '%s' (Index %d)"), 
		*OldState.ToString(), *NewState.ToString(), CurrentStateIndex);

	BroadcastStateChange(OldState, NewState);
	return true;
}

bool UExperienceStateMachine::RetreatState()
{
	if (!bIsRunning)
	{
		UE_LOG(LogTemp, Warning, TEXT("ExperienceStateMachine: Cannot retreat - experience not running"));
		return false;
	}

	if (!CanRetreat())
	{
		UE_LOG(LogTemp, Warning, TEXT("ExperienceStateMachine: Cannot retreat from current state"));
		return false;
	}

	FName OldState = GetCurrentStateName();
	CurrentStateIndex--;
	FName NewState = GetCurrentStateName();

	UE_LOG(LogTemp, Log, TEXT("ExperienceStateMachine: Retreated from '%s' to '%s' (Index %d)"), 
		*OldState.ToString(), *NewState.ToString(), CurrentStateIndex);

	BroadcastStateChange(OldState, NewState);
	return true;
}

bool UExperienceStateMachine::JumpToState(FName StateName)
{
	for (int32 i = 0; i < States.Num(); i++)
	{
		if (States[i].StateName == StateName)
		{
			return JumpToStateIndex(i);
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("ExperienceStateMachine: State '%s' not found"), *StateName.ToString());
	return false;
}

bool UExperienceStateMachine::JumpToStateIndex(int32 StateIndex)
{
	if (!States.IsValidIndex(StateIndex))
	{
		UE_LOG(LogTemp, Error, TEXT("ExperienceStateMachine: Invalid state index %d"), StateIndex);
		return false;
	}

	FName OldState = GetCurrentStateName();
	CurrentStateIndex = StateIndex;
	FName NewState = GetCurrentStateName();

	UE_LOG(LogTemp, Log, TEXT("ExperienceStateMachine: Jumped from '%s' to '%s' (Index %d)"), 
		*OldState.ToString(), *NewState.ToString(), CurrentStateIndex);

	BroadcastStateChange(OldState, NewState);
	return true;
}

FExperienceState UExperienceStateMachine::GetCurrentState() const
{
	if (States.IsValidIndex(CurrentStateIndex))
	{
		return States[CurrentStateIndex];
	}

	return FExperienceState();
}

FName UExperienceStateMachine::GetCurrentStateName() const
{
	if (States.IsValidIndex(CurrentStateIndex))
	{
		return States[CurrentStateIndex].StateName;
	}

	return NAME_None;
}

bool UExperienceStateMachine::CanAdvance() const
{
	if (!States.IsValidIndex(CurrentStateIndex))
	{
		return false;
	}

	// Check if we're at the last state
	if (CurrentStateIndex >= States.Num() - 1)
	{
		return false;
	}

	// Check if current state allows skipping forward
	return States[CurrentStateIndex].bCanSkipForward;
}

bool UExperienceStateMachine::CanRetreat() const
{
	if (!States.IsValidIndex(CurrentStateIndex))
	{
		return false;
	}

	// Check if we're at the first state
	if (CurrentStateIndex <= 0)
	{
		return false;
	}

	// Check if current state allows skipping backward
	return States[CurrentStateIndex].bCanSkipBackward;
}

void UExperienceStateMachine::ResetExperience()
{
	FName OldState = GetCurrentStateName();
	CurrentStateIndex = 0;
	FName NewState = GetCurrentStateName();

	UE_LOG(LogTemp, Log, TEXT("ExperienceStateMachine: Reset to initial state '%s'"), *NewState.ToString());

	if (bIsRunning)
	{
		BroadcastStateChange(OldState, NewState);
	}
}

void UExperienceStateMachine::StopExperience()
{
	bIsRunning = false;
	UE_LOG(LogTemp, Log, TEXT("ExperienceStateMachine: Experience stopped at state '%s'"), *GetCurrentStateName().ToString());
}

void UExperienceStateMachine::BroadcastStateChange(FName OldState, FName NewState)
{
	OnStateChanged.Broadcast(OldState, NewState, CurrentStateIndex);
}



