// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "VOIPManager.h"
#include "VOIP.h"
#include "MumbleClient.h"
#include "SteamAudioSourceComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Engine/World.h"

UVOIPManager::UVOIPManager(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f; // Update 10 times per second
	bWantsInitializeComponent = true;
	
	ConnectionState = EVOIPConnectionState::Disconnected;
	bMicrophoneMuted = false;
	OutputVolume = 1.0f;
}

UVOIPManager::~UVOIPManager()
{
	Disconnect();
}

void UVOIPManager::BeginPlay()
{
	Super::BeginPlay();

	// Auto-generate player name if not set
	if (PlayerName.IsEmpty())
	{
		if (APawn* Pawn = GetOwner<APawn>())
		{
			if (APlayerController* PC = Pawn->GetController<APlayerController>())
			{
				if (APlayerState* PS = PC->GetPlayerState<APlayerState>())
				{
					PlayerName = FString::Printf(TEXT("Player_%d"), PS->GetPlayerId());
				}
				else
				{
					PlayerName = FString::Printf(TEXT("Player_%d"), GetOwner()->GetUniqueID());
				}
			}
			else
			{
				PlayerName = FString::Printf(TEXT("Player_%d"), GetOwner()->GetUniqueID());
			}
		}
		else
		{
			PlayerName = FString::Printf(TEXT("Player_%d"), GetOwner()->GetUniqueID());
		}
	}

	// Create Mumble client
	MumbleClient = NewObject<UMumbleClient>(this);
	if (MumbleClient)
	{
		MumbleClient->OnAudioReceived.AddDynamic(this, &UVOIPManager::OnMumbleAudioReceived);
		MumbleClient->OnConnectionStateChanged.AddDynamic(this, &UVOIPManager::OnMumbleConnectionStateChanged);
	}

	// Auto-connect if enabled
	if (bAutoConnect)
	{
		Connect();
	}
}

void UVOIPManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Disconnect();
	Super::EndPlay(EndPlayReason);
}

void UVOIPManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Update audio source positions
	if (ConnectionState == EVOIPConnectionState::Connected)
	{
		UpdateAudioSourcePositions();
	}
}

bool UVOIPManager::Connect()
{
	if (ConnectionState == EVOIPConnectionState::Connected || ConnectionState == EVOIPConnectionState::Connecting)
	{
		UE_LOG(LogVOIP, Warning, TEXT("VOIPManager: Already connected or connecting"));
		return false;
	}

	if (!MumbleClient)
	{
		UE_LOG(LogVOIP, Error, TEXT("VOIPManager: MumbleClient not initialized"));
		return false;
	}

	UE_LOG(LogVOIP, Log, TEXT("VOIPManager: Connecting to Mumble server %s:%d as %s"), 
		*ServerIP, ServerPort, *PlayerName);

	ConnectionState = EVOIPConnectionState::Connecting;
	OnConnectionStateChanged.Broadcast(ConnectionState);

	bool bSuccess = MumbleClient->Connect(ServerIP, ServerPort, PlayerName);
	
	if (!bSuccess)
	{
		ConnectionState = EVOIPConnectionState::Error;
		OnConnectionStateChanged.Broadcast(ConnectionState);
		UE_LOG(LogVOIP, Error, TEXT("VOIPManager: Failed to connect to Mumble server"));
	}

	return bSuccess;
}

void UVOIPManager::Disconnect()
{
	if (ConnectionState == EVOIPConnectionState::Disconnected)
	{
		return;
	}

	UE_LOG(LogVOIP, Log, TEXT("VOIPManager: Disconnecting from Mumble server"));

	if (MumbleClient)
	{
		MumbleClient->Disconnect();
	}

	// Clean up all audio sources
	for (auto& Pair : AudioSourceMap)
	{
		if (Pair.Value)
		{
			Pair.Value->DestroyComponent();
		}
	}
	AudioSourceMap.Empty();

	ConnectionState = EVOIPConnectionState::Disconnected;
	OnConnectionStateChanged.Broadcast(ConnectionState);
}

int32 UVOIPManager::GetPlayerCount() const
{
	return AudioSourceMap.Num();
}

void UVOIPManager::SetMicrophoneMuted(bool bMuted)
{
	bMicrophoneMuted = bMuted;
	if (MumbleClient)
	{
		MumbleClient->SetMicrophoneMuted(bMuted);
	}
	UE_LOG(LogVOIP, Log, TEXT("VOIPManager: Microphone %s"), bMuted ? TEXT("muted") : TEXT("unmuted"));
}

void UVOIPManager::SetOutputVolume(float Volume)
{
	OutputVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
	
	// Update all audio sources
	for (auto& Pair : AudioSourceMap)
	{
		if (Pair.Value)
		{
			Pair.Value->SetVolume(OutputVolume);
		}
	}
}

void UVOIPManager::OnMumbleAudioReceived(int32 UserId, const TArray<uint8>& OpusData, const FVector& Position)
{
	// Get or create audio source for this user
	USteamAudioSourceComponent* AudioSource = GetOrCreateAudioSource(UserId);
	if (!AudioSource)
	{
		UE_LOG(LogVOIP, Warning, TEXT("VOIPManager: Failed to create audio source for user %d"), UserId);
		return;
	}

	// Process audio through Steam Audio spatialization
	AudioSource->ProcessAudioData(OpusData, Position);

	// Broadcast event
	OnRemotePlayerAudioReceived.Broadcast(UserId, Position);

	// NOOP: TODO - Decode Opus to PCM for visitors
	// For now, visitors will need to decode themselves or we need to decode here
	// Decode Opus to PCM (Mumble uses 48kHz, 16-bit PCM)
	TArray<float> PCMData;
	// TODO: Decode OpusData to PCMData
	// For now, create empty array - visitors will need to handle Opus decoding or we decode here
	
	// Notify all registered visitors
	for (TScriptInterface<IVOIPAudioVisitor> Visitor : AudioVisitors)
	{
		if (Visitor.GetInterface())
		{
			// NOOP: TODO - Decode Opus to PCM before passing to visitor
			// For now, pass empty PCM data - visitor will need to decode Opus themselves
			// Or we decode here and pass PCM
			TArray<float> DecodedPCM;
			// TODO: Decode OpusData to DecodedPCM
			Visitor->OnPlayerAudioReceived(UserId, DecodedPCM, 48000, Position);  // Mumble uses 48kHz
		}
	}
}

void UVOIPManager::OnMumbleConnectionStateChanged(EVOIPConnectionState NewState)
{
	ConnectionState = NewState;
	OnConnectionStateChanged.Broadcast(NewState);

	if (NewState == EVOIPConnectionState::Connected)
	{
		UE_LOG(LogVOIP, Log, TEXT("VOIPManager: Connected to Mumble server"));
	}
	else if (NewState == EVOIPConnectionState::Disconnected)
	{
		UE_LOG(LogVOIP, Log, TEXT("VOIPManager: Disconnected from Mumble server"));
	}
	else if (NewState == EVOIPConnectionState::Error)
	{
		UE_LOG(LogVOIP, Error, TEXT("VOIPManager: Connection error"));
	}
}

USteamAudioSourceComponent* UVOIPManager::GetOrCreateAudioSource(int32 UserId)
{
	// Check if audio source already exists
	if (TObjectPtr<USteamAudioSourceComponent>* ExistingSource = AudioSourceMap.Find(UserId))
	{
		if (*ExistingSource && IsValid(*ExistingSource))
		{
			return *ExistingSource;
		}
	}

	// Create new audio source component
	USteamAudioSourceComponent* AudioSource = NewObject<USteamAudioSourceComponent>(this);
	if (!AudioSource)
	{
		return nullptr;
	}

	// Register component
	AudioSource->RegisterComponent();
	AudioSource->SetVolume(OutputVolume);

	// Add to map
	AudioSourceMap.Add(UserId, AudioSource);

	UE_LOG(LogVOIP, Log, TEXT("VOIPManager: Created audio source for user %d"), UserId);

	return AudioSource;
}

void UVOIPManager::RemoveAudioSource(int32 UserId)
{
	if (TObjectPtr<USteamAudioSourceComponent>* AudioSource = AudioSourceMap.Find(UserId))
	{
		if (*AudioSource && IsValid(*AudioSource))
		{
			(*AudioSource)->DestroyComponent();
		}
		AudioSourceMap.Remove(UserId);
		UE_LOG(LogVOIP, Log, TEXT("VOIPManager: Removed audio source for user %d"), UserId);
	}
}

void UVOIPManager::UpdateAudioSourcePositions()
{
	// Get local player position (for HRTF calculation)
	FVector LocalPlayerPosition = FVector::ZeroVector;
	FRotator LocalPlayerRotation = FRotator::ZeroRotator;

	if (APawn* Pawn = GetOwner<APawn>())
	{
		LocalPlayerPosition = Pawn->GetActorLocation();
		LocalPlayerRotation = Pawn->GetActorRotation();
	}
	else if (AActor* Owner = GetOwner())
	{
		LocalPlayerPosition = Owner->GetActorLocation();
		LocalPlayerRotation = Owner->GetActorRotation();
	}

	// Update all audio source positions
	// Note: Remote player positions should come from replicated player states
	// This is a placeholder - actual implementation will query player positions from game state
	for (auto& Pair : AudioSourceMap)
	{
		if (Pair.Value && IsValid(Pair.Value))
		{
			// Get remote player position (this should come from replicated player state)
			// For now, we'll use the position from Mumble if available
			// TODO: Integrate with player replication system
			FVector RemotePlayerPosition = FVector::ZeroVector; // Get from player state
			Pair.Value->UpdatePosition(RemotePlayerPosition, LocalPlayerPosition, LocalPlayerRotation);
		}
	}
}

void UVOIPManager::RegisterAudioVisitor(TScriptInterface<IVOIPAudioVisitor> Visitor)
{
	if (!Visitor.GetInterface())
	{
		UE_LOG(LogVOIP, Warning, TEXT("VOIPManager: Attempted to register invalid audio visitor"));
		return;
	}

	// Check if already registered
	if (AudioVisitors.Contains(Visitor))
	{
		UE_LOG(LogVOIP, Warning, TEXT("VOIPManager: Audio visitor already registered"));
		return;
	}

	AudioVisitors.Add(Visitor);
	UE_LOG(LogVOIP, Log, TEXT("VOIPManager: Registered audio visitor"));
}

void UVOIPManager::UnregisterAudioVisitor(TScriptInterface<IVOIPAudioVisitor> Visitor)
{
	AudioVisitors.Remove(Visitor);
	UE_LOG(LogVOIP, Log, TEXT("VOIPManager: Unregistered audio visitor"));
}

