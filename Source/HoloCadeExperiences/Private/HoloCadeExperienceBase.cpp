// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "HoloCadeExperienceBase.h"
#include "Input/HoloCadeInputAdapter.h"
#include "Networking/HoloCadeServerCommandProtocol.h"
#include "ExperienceLoop/ExperienceStateMachine.h"
#include "HoloCadeWorldPositionCalibrator.h"
#include "GameFramework/GameStateBase.h"

AHoloCadeExperienceBase::AHoloCadeExperienceBase()
{
	// Enable ticking for command protocol processing
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	// Create input adapter component
	InputAdapter = CreateDefaultSubobject<UHoloCadeInputAdapter>(TEXT("InputAdapter"));

	// Create command protocol component (will be initialized on dedicated server)
	CommandProtocol = CreateDefaultSubobject<UHoloCadeServerCommandProtocol>(TEXT("CommandProtocol"));

	// Narrative state machine will be created in InitializeExperienceImpl if bUseNarrativeStateMachine is true
	NarrativeStateMachine = nullptr;

	// Create world position calibrator (available to all experiences)
	WorldPositionCalibrator = CreateDefaultSubobject<UHoloCadeWorldPositionCalibrator>(TEXT("WorldPositionCalibrator"));

	// Default HMD configuration
	HMDConfig.HMDType = EHoloCadeHMDType::OpenXR;
	HMDConfig.bEnablePassthrough = false;
	HMDConfig.PassthroughAlpha = 0.0f;

	// Default tracking configuration
	TrackingConfig.TrackingSystem = EHoloCadeTrackingSystem::SteamVRTrackers;
	TrackingConfig.ExpectedDeviceCount = 0;
}

void AHoloCadeExperienceBase::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoInitialize)
	{
		InitializeExperience();
	}
}

void AHoloCadeExperienceBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ShutdownExperience();
	Super::EndPlay(EndPlayReason);
}

bool AHoloCadeExperienceBase::InitializeExperience()
{
	if (bIsInitialized)
	{
		UE_LOG(LogTemp, Warning, TEXT("HoloCadeExperience: Already initialized"));
		return true;
	}

	UE_LOG(LogTemp, Log, TEXT("HoloCadeExperience: Initializing experience..."));

	// Call derived class implementation
	if (!InitializeExperienceImpl())
	{
		UE_LOG(LogTemp, Error, TEXT("HoloCadeExperience: Failed to initialize experience"));
		return false;
	}

	bIsInitialized = true;
	UE_LOG(LogTemp, Log, TEXT("HoloCadeExperience: Initialization complete"));
	return true;
}

void AHoloCadeExperienceBase::ShutdownExperience()
{
	if (!bIsInitialized)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("HoloCadeExperience: Shutting down experience..."));

	ShutdownExperienceImpl();

	bIsInitialized = false;
	UE_LOG(LogTemp, Log, TEXT("HoloCadeExperience: Shutdown complete"));
}

bool AHoloCadeExperienceBase::InitializeExperienceImpl()
{
	// Initialize command protocol if running as dedicated server
	InitializeCommandProtocol();

	// Initialize narrative state machine if enabled
	if (bUseNarrativeStateMachine && !NarrativeStateMachine)
	{
		NarrativeStateMachine = NewObject<UExperienceStateMachine>(this, UExperienceStateMachine::StaticClass());
		if (NarrativeStateMachine)
		{
			// Bind to state change events (dynamic delegate uses AddDynamic)
			NarrativeStateMachine->OnStateChanged.AddDynamic(this, &AHoloCadeExperienceBase::HandleNarrativeStateChanged);
			UE_LOG(LogTemp, Log, TEXT("HoloCadeExperienceBase: Narrative state machine created"));
		}
	}

	// Base implementation - override in derived classes
	return true;
}

void AHoloCadeExperienceBase::ShutdownExperienceImpl()
{
	// Stop command protocol if running
	if (CommandProtocol && CommandProtocol->IsListening())
	{
		CommandProtocol->StopListening();
	}

	// Base implementation - override in derived classes
}

void AHoloCadeExperienceBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Tick command protocol if listening (dedicated server mode)
	if (CommandProtocol && CommandProtocol->IsListening())
	{
		CommandProtocol->Tick(DeltaTime);
	}
}

UHoloCadeInputAdapter* AHoloCadeExperienceBase::GetInputAdapter() const
{
	return InputAdapter;
}

void AHoloCadeExperienceBase::InitializeCommandProtocol()
{
	UWorld* World = GetWorld();
	if (!World || World->GetNetMode() != NM_DedicatedServer)
	{
		// Not running as dedicated server, skip command protocol
		return;
	}

	if (!CommandProtocol)
	{
		UE_LOG(LogTemp, Warning, TEXT("HoloCadeExperienceBase: CommandProtocol not created"));
		return;
	}

	// Start listening for commands
	if (CommandProtocol->StartListening())
	{
		// Bind to command received event
		CommandProtocol->OnCommandReceived.AddDynamic(this, &AHoloCadeExperienceBase::OnCommandReceived);
		UE_LOG(LogTemp, Log, TEXT("HoloCadeExperienceBase: Command protocol listening on port 7779"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("HoloCadeExperienceBase: Failed to start command protocol"));
	}
}

void AHoloCadeExperienceBase::OnCommandReceived(const FHoloCadeServerCommandMessage& Command, UHoloCadeServerCommandProtocol* Protocol)
{
	UE_LOG(LogTemp, Log, TEXT("HoloCadeExperienceBase: Received command %d (seq: %d)"), 
		(uint8)Command.Command, Command.SequenceNumber);

	// Handle base commands
	switch (Command.Command)
	{
	case EHoloCadeServerCommand::RequestStatus:
	{
		// Get current player count (NOOP: TODO - Implement proper player tracking)
		int32 CurrentPlayerCount = 0;
		UWorld* World = GetWorld();
		if (World && World->GetGameState())
		{
			// Try to get player count from game state
			CurrentPlayerCount = World->GetGameState()->PlayerArray.Num();
		}

		// Build status JSON response
		FString StatusData = FString::Printf(
			TEXT("{\"IsRunning\":%s,\"IsInitialized\":%s,\"CurrentPlayers\":%d,\"MaxPlayers\":%d,\"ExperienceState\":\"%s\"}"),
			bIsInitialized ? TEXT("true") : TEXT("false"),
			bIsInitialized ? TEXT("true") : TEXT("false"),
			CurrentPlayerCount,
			GetMaxPlayers(),
			bIsInitialized ? TEXT("Active") : TEXT("Idle")
		);

		// Send response back to client
		if (Protocol)
		{
			TSharedPtr<FInternetAddr> SenderAddr = Protocol->GetLastSenderAddress();
			if (SenderAddr.IsValid())
			{
				FHoloCadeServerResponseMessage Response(true, TEXT("Status"), StatusData);
				Protocol->SendResponse(Response, SenderAddr.ToSharedRef());
				UE_LOG(LogTemp, Log, TEXT("HoloCadeExperienceBase: Sent status response (Players: %d/%d)"), 
					CurrentPlayerCount, GetMaxPlayers());
			}
		}
		break;
	}
	case EHoloCadeServerCommand::Shutdown:
	{
		UE_LOG(LogTemp, Log, TEXT("HoloCadeExperienceBase: Shutdown command received"));
		ShutdownExperience();
		
		// Send confirmation response
		if (Protocol)
		{
			TSharedPtr<FInternetAddr> SenderAddr = Protocol->GetLastSenderAddress();
			if (SenderAddr.IsValid())
			{
				FHoloCadeServerResponseMessage Response(true, TEXT("Shutdown initiated"));
				Protocol->SendResponse(Response, SenderAddr.ToSharedRef());
			}
		}
		break;
	}
	default:
		// Other commands handled by derived classes
		break;
	}
}

// ========================================
// NARRATIVE STATE MACHINE API
// ========================================

UExperienceStateMachine* AHoloCadeExperienceBase::GetNarrativeStateMachine() const
{
	return NarrativeStateMachine;
}

FName AHoloCadeExperienceBase::GetCurrentNarrativeState() const
{
	if (NarrativeStateMachine && NarrativeStateMachine->bIsRunning)
	{
		return NarrativeStateMachine->GetCurrentStateName();
	}
	return NAME_None;
}

bool AHoloCadeExperienceBase::AdvanceNarrativeState()
{
	if (!NarrativeStateMachine || !NarrativeStateMachine->bIsRunning)
	{
		return false;
	}
	return NarrativeStateMachine->AdvanceState();
}

bool AHoloCadeExperienceBase::RetreatNarrativeState()
{
	if (!NarrativeStateMachine || !NarrativeStateMachine->bIsRunning)
	{
		return false;
	}
	return NarrativeStateMachine->RetreatState();
}

bool AHoloCadeExperienceBase::JumpToNarrativeState(FName StateName)
{
	if (!NarrativeStateMachine || !NarrativeStateMachine->bIsRunning)
	{
		return false;
	}
	return NarrativeStateMachine->JumpToState(StateName);
}

void AHoloCadeExperienceBase::HandleNarrativeStateChanged(FName OldState, FName NewState, int32 NewStateIndex)
{
	// Call Blueprint implementable event
	OnNarrativeStateChanged(OldState, NewState, NewStateIndex);
}

