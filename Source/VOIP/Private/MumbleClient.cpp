// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "MumbleClient.h"
#include "VOIP.h"
#include "Engine/Engine.h"
#include "TimerManager.h"

UMumbleClient::UMumbleClient(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsConnected = false;
	bMicrophoneMuted = false;
	UserId = -1;
	ServerPort = 64738;
}

UMumbleClient::~UMumbleClient()
{
	Disconnect();
}

bool UMumbleClient::Connect(const FString& InServerIP, int32 InPort, const FString& InUserName)
{
	if (bIsConnected)
	{
		UE_LOG(LogVOIP, Warning, TEXT("MumbleClient: Already connected"));
		return false;
	}

	ServerIP = InServerIP;
	ServerPort = InPort;
	UserName = InUserName;

	UE_LOG(LogVOIP, Log, TEXT("MumbleClient: Connecting to %s:%d as %s"), 
		*ServerIP, ServerPort, *UserName);

	// Initialize MumbleLink plugin
	if (!InitializeMumbleLink())
	{
		UE_LOG(LogVOIP, Error, TEXT("MumbleClient: Failed to initialize MumbleLink plugin"));
		SetConnectionState(EVOIPConnectionState::Error);
		return false;
	}

	// TODO: Call MumbleLink plugin's Connect function
	// if (MumbleLinkInterface)
	// {
	//     bool bSuccess = MumbleLinkInterface->Connect(ServerIP, ServerPort, UserName);
	//     if (bSuccess)
	//     {
	//         SetConnectionState(EVOIPConnectionState::Connecting);
	//     }
	//     return bSuccess;
	// }

	// Placeholder: For now, simulate connection
	// Remove this when MumbleLink plugin is integrated
	SetConnectionState(EVOIPConnectionState::Connecting);
	
	// Simulate successful connection after a delay
	// In real implementation, this will be called by MumbleLink plugin callback
	FTimerHandle TimerHandle;
	if (UWorld* World = GEngine->GetWorldFromContextObject(this, EGetWorldErrorMode::LogAndReturnNull))
	{
		World->GetTimerManager().SetTimer(TimerHandle, [this]()
		{
			SetConnectionState(EVOIPConnectionState::Connected);
			UserId = 1; // Placeholder - will be assigned by server
		}, 1.0f, false);
	}

	return true;
}

void UMumbleClient::Disconnect()
{
	if (!bIsConnected)
	{
		return;
	}

	UE_LOG(LogVOIP, Log, TEXT("MumbleClient: Disconnecting from server"));

	// TODO: Call MumbleLink plugin's Disconnect function
	// if (MumbleLinkInterface)
	// {
	//     MumbleLinkInterface->Disconnect();
	// }

	CleanupMumbleLink();

	bIsConnected = false;
	UserId = -1;
	SetConnectionState(EVOIPConnectionState::Disconnected);
}

void UMumbleClient::SetMicrophoneMuted(bool bMuted)
{
	bMicrophoneMuted = bMuted;
	
	// TODO: Call MumbleLink plugin's SetMute function
	// if (MumbleLinkInterface)
	// {
	//     MumbleLinkInterface->SetMicrophoneMuted(bMuted);
	// }

	UE_LOG(LogVOIP, Log, TEXT("MumbleClient: Microphone %s"), bMuted ? TEXT("muted") : TEXT("unmuted"));
}

void UMumbleClient::SendAudioData(const TArray<float>& PCMData, const FVector& Position)
{
	if (!bIsConnected || bMicrophoneMuted)
	{
		return;
	}

	// TODO: Encode PCM to Opus and send via MumbleLink plugin
	// if (MumbleLinkInterface)
	// {
	//     TArray<uint8> OpusData;
	//     if (EncodeOpus(PCMData, OpusData))
	//     {
	//         MumbleLinkInterface->SendAudio(OpusData, Position);
	//     }
	// }
}

void UMumbleClient::ProcessIncomingAudio(int32 InUserId, const TArray<uint8>& OpusData, const FVector& Position)
{
	// Broadcast to listeners
	OnAudioReceived.Broadcast(InUserId, OpusData, Position);
}

void UMumbleClient::SetConnectionState(EVOIPConnectionState NewState)
{
	if (NewState == EVOIPConnectionState::Connected)
	{
		bIsConnected = true;
	}
	else if (NewState == EVOIPConnectionState::Disconnected || NewState == EVOIPConnectionState::Error)
	{
		bIsConnected = false;
	}

	OnConnectionStateChanged.Broadcast(NewState);
}

bool UMumbleClient::InitializeMumbleLink()
{
	// TODO: Load MumbleLink plugin and get interface
	// This will be implemented when MumbleLink submodule is added
	// 
	// Example:
	// IPluginManager& PluginManager = IPluginManager::Get();
	// TSharedPtr<IPlugin> MumbleLinkPlugin = PluginManager.FindPlugin("MumbleLink");
	// if (MumbleLinkPlugin && MumbleLinkPlugin->IsEnabled())
	// {
	//     // Get interface from plugin
	//     MumbleLinkInterface = ...;
	//     return MumbleLinkInterface != nullptr;
	// }

	UE_LOG(LogVOIP, Warning, TEXT("MumbleClient: MumbleLink plugin not yet integrated. Using placeholder."));
	return true; // Placeholder - return true for now
}

void UMumbleClient::CleanupMumbleLink()
{
	// TODO: Cleanup MumbleLink plugin interface
	// MumbleLinkInterface = nullptr;
}

