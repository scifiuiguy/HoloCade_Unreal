// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VOIPTypes.h"
#include "IVOIPAudioVisitor.h"
#include "VOIPManager.generated.h"

class UMumbleClient;
class USteamAudioSourceComponent;

/**
 * Delegate for VOIP connection events
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVOIPConnectionStateChanged, EVOIPConnectionState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRemotePlayerAudioReceived, int32, UserId, const FVector&, Position);

/**
 * HoloCade VOIP Manager Component
 * 
 * Main component for VOIP functionality. Attach to HMD actor or player pawn.
 * 
 * Handles:
 * - Mumble connection management
 * - Per-user audio source creation and management
 * - Steam Audio spatialization setup
 * - Automatic audio routing based on player positions
 * 
 * Usage:
 * 1. Add component to HMD/Player actor
 * 2. Set Server IP and Port
 * 3. Call Connect() to start VOIP
 * 4. Audio is automatically spatialized based on player positions
 * 
 * Replication:
 * - Uses Unreal's native replication system
 * - Player positions are replicated automatically
 * - Audio data is streamed via Mumble (not replicated)
 */
UCLASS(ClassGroup=(HoloCade), meta=(BlueprintSpawnableComponent))
class VOIP_API UVOIPManager : public UActorComponent
{
	GENERATED_BODY()

public:
	UVOIPManager(const FObjectInitializer& ObjectInitializer);
	virtual ~UVOIPManager();

	/** Server IP address for Mumble connection */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|VOIP|Connection")
	FString ServerIP = TEXT("192.168.1.100");

	/** Server port for Mumble connection (default: 64738) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|VOIP|Connection")
	int32 ServerPort = 64738;

	/** Player name/identifier for Mumble (auto-generated if empty) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|VOIP|Connection")
	FString PlayerName;

	/** Enable automatic connection on BeginPlay */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|VOIP|Connection")
	bool bAutoConnect = true;

	/** Current connection state */
	UPROPERTY(BlueprintReadOnly, Category = "HoloCade|VOIP|Connection")
	EVOIPConnectionState ConnectionState = EVOIPConnectionState::Disconnected;

	/** Fired when connection state changes */
	UPROPERTY(BlueprintAssignable, Category = "HoloCade|VOIP|Events")
	FOnVOIPConnectionStateChanged OnConnectionStateChanged;

	/** Fired when remote player audio is received */
	UPROPERTY(BlueprintAssignable, Category = "HoloCade|VOIP|Events")
	FOnRemotePlayerAudioReceived OnRemotePlayerAudioReceived;

	/**
	 * Connect to Mumble server
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|VOIP")
	bool Connect();

	/**
	 * Disconnect from Mumble server
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|VOIP")
	void Disconnect();

	/**
	 * Check if currently connected
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|VOIP")
	bool IsConnected() const { return ConnectionState == EVOIPConnectionState::Connected; }

	/**
	 * Get current player count
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|VOIP")
	int32 GetPlayerCount() const;

	/**
	 * Set microphone mute state
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|VOIP")
	void SetMicrophoneMuted(bool bMuted);

	/**
	 * Check if microphone is muted
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|VOIP")
	bool IsMicrophoneMuted() const { return bMicrophoneMuted; }

	/**
	 * Set audio output volume (0.0 to 1.0)
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|VOIP")
	void SetOutputVolume(float Volume);

	/**
	 * Get audio output volume
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|VOIP")
	float GetOutputVolume() const { return OutputVolume; }

	/**
	 * Register an audio visitor to receive audio events
	 * Visitors are notified when player audio is received
	 * @param Visitor - Visitor implementing IVOIPAudioVisitor interface
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|VOIP")
	void RegisterAudioVisitor(TScriptInterface<IVOIPAudioVisitor> Visitor);

	/**
	 * Unregister an audio visitor
	 * @param Visitor - Visitor to remove
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|VOIP")
	void UnregisterAudioVisitor(TScriptInterface<IVOIPAudioVisitor> Visitor);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Mumble client instance */
	UPROPERTY()
	TObjectPtr<UMumbleClient> MumbleClient;

	/** Map of user IDs to audio source components */
	UPROPERTY()
	TMap<int32, TObjectPtr<USteamAudioSourceComponent>> AudioSourceMap;

	/** Registered audio visitors (for decoupled module integration) */
	UPROPERTY()
	TArray<TScriptInterface<IVOIPAudioVisitor>> AudioVisitors;

	/** Microphone mute state */
	bool bMicrophoneMuted = false;

	/** Audio output volume (0.0 to 1.0) */
	float OutputVolume = 1.0f;

	/** Handle remote audio received from Mumble */
	UFUNCTION()
	void OnMumbleAudioReceived(int32 UserId, const TArray<uint8>& OpusData, const FVector& Position);

	/** Handle connection state change from Mumble */
	UFUNCTION()
	void OnMumbleConnectionStateChanged(EVOIPConnectionState NewState);

	/** Create or get audio source for a user */
	USteamAudioSourceComponent* GetOrCreateAudioSource(int32 UserId);

	/** Remove audio source for a user */
	void RemoveAudioSource(int32 UserId);

	/** Update audio source positions based on player locations */
	void UpdateAudioSourcePositions();
};

