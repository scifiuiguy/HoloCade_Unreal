// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "VOIPTypes.h"
#include "MumbleClient.generated.h"

/**
 * Delegate for Mumble audio received
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnMumbleAudioReceived, int32, UserId, const TArray<uint8>&, OpusData, const FVector&, Position);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMumbleConnectionStateChanged, EVOIPConnectionState, NewState);

/**
 * Mumble Client Wrapper
 * 
 * Wraps Mumble protocol implementation for Unreal Engine.
 * Handles connection, audio encoding/decoding, and user management.
 * 
 * This class will interface with the MumbleLink plugin (git submodule)
 * to provide low-latency VOIP functionality.
 * 
 * Protocol:
 * - Uses Mumble's native protocol (TCP for control, UDP for audio)
 * - Opus codec for audio compression
 * - Positional audio support
 * 
 * Note: Actual Mumble implementation will be provided by MumbleLink plugin.
 * This class provides a clean Unreal-friendly interface.
 */
UCLASS(BlueprintType)
class VOIP_API UMumbleClient : public UObject
{
	GENERATED_BODY()

public:
	UMumbleClient(const FObjectInitializer& ObjectInitializer);
	virtual ~UMumbleClient();

	/** Fired when audio data is received from a remote user */
	UPROPERTY(BlueprintAssignable, Category = "HoloCade|VOIP|Mumble")
	FOnMumbleAudioReceived OnAudioReceived;

	/** Fired when connection state changes */
	UPROPERTY(BlueprintAssignable, Category = "HoloCade|VOIP|Mumble")
	FOnMumbleConnectionStateChanged OnConnectionStateChanged;

	/**
	 * Connect to Mumble server
	 * @param ServerIP - Server IP address
	 * @param Port - Server port (default: 64738)
	 * @param UserName - Username for connection
	 * @return True if connection initiated successfully
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|VOIP|Mumble")
	bool Connect(const FString& ServerIP, int32 Port, const FString& UserName);

	/**
	 * Disconnect from Mumble server
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|VOIP|Mumble")
	void Disconnect();

	/**
	 * Check if connected
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|VOIP|Mumble")
	bool IsConnected() const { return bIsConnected; }

	/**
	 * Set microphone mute state
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|VOIP|Mumble")
	void SetMicrophoneMuted(bool bMuted);

	/**
	 * Check if microphone is muted
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|VOIP|Mumble")
	bool IsMicrophoneMuted() const { return bMicrophoneMuted; }

	/**
	 * Send audio data to server
	 * Called automatically from microphone input
	 * 
	 * Note: Microphone capture uses Unreal's audio system, which accesses
	 * any microphone device recognized by the OS (WASAPI on Windows).
	 * HMD microphones (Oculus, Vive, etc.) appear as standard audio input
	 * devices and are automatically accessible.
	 */
	void SendAudioData(const TArray<float>& PCMData, const FVector& Position);

	/**
	 * Process incoming audio data
	 * Called by MumbleLink plugin when audio is received
	 */
	void ProcessIncomingAudio(int32 UserId, const TArray<uint8>& OpusData, const FVector& Position);

	/**
	 * Update connection state
	 */
	void SetConnectionState(EVOIPConnectionState NewState);

	/**
	 * Get current user ID (assigned by server)
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|VOIP|Mumble")
	int32 GetUserId() const { return UserId; }

protected:
	/** Server IP address */
	FString ServerIP;

	/** Server port */
	int32 ServerPort = 64738;

	/** Username */
	FString UserName;

	/** Is currently connected */
	bool bIsConnected = false;

	/** Microphone mute state */
	bool bMicrophoneMuted = false;

	/** User ID assigned by server */
	int32 UserId = -1;

	/** MumbleLink plugin interface (will be set when plugin is loaded) */
	// TODO: Add interface to MumbleLink plugin
	// IMumbleLinkInterface* MumbleLinkInterface = nullptr;

	/** Initialize MumbleLink plugin connection */
	bool InitializeMumbleLink();

	/** Cleanup MumbleLink plugin connection */
	void CleanupMumbleLink();
};

