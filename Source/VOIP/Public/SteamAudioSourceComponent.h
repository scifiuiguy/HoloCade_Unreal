// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SteamAudioSourceComponent.generated.h"

/**
 * Steam Audio Source Component
 * 
 * Per-user audio source component that handles Steam Audio spatialization.
 * One component is created per remote player for spatial audio rendering.
 * 
 * Handles:
 * - Opus audio decoding (from Mumble)
 * - Steam Audio HRTF processing
 * - 3D spatialization based on player positions
 * - Audio playback via Unreal's audio system
 * 
 * This component will interface with the Steam Audio plugin (git submodule)
 * to provide high-quality 3D HRTF spatialization.
 * 
 * Audio Output:
 * - Routes to OS-selected audio output device (via Unreal's audio system)
 * - HMD headphones (Oculus, Vive, etc.) appear as standard audio devices
 * - Works with any audio output device recognized by Windows/OS
 */
UCLASS(ClassGroup=(HoloCade), meta=(BlueprintSpawnableComponent))
class VOIP_API USteamAudioSourceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USteamAudioSourceComponent(const FObjectInitializer& ObjectInitializer);
	virtual ~USteamAudioSourceComponent();

	/** Audio output volume (0.0 to 1.0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|VOIP|SteamAudio")
	float Volume = 1.0f;

	/**
	 * Process incoming Opus audio data
	 * @param OpusData - Compressed Opus audio data from Mumble
	 * @param RemotePosition - Position of the remote player
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|VOIP|SteamAudio")
	void ProcessAudioData(const TArray<uint8>& OpusData, const FVector& RemotePosition);

	/**
	 * Update audio source position for spatialization
	 * @param RemotePosition - Position of the remote player
	 * @param ListenerPosition - Position of the local listener (HMD)
	 * @param ListenerRotation - Rotation of the local listener (HMD)
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|VOIP|SteamAudio")
	void UpdatePosition(const FVector& RemotePosition, const FVector& ListenerPosition, const FRotator& ListenerRotation);

	/**
	 * Set output volume
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|VOIP|SteamAudio")
	void SetVolume(float NewVolume);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Steam Audio source handle (will be set when plugin is loaded) */
	// TODO: Add Steam Audio source handle
	// void* SteamAudioSource = nullptr;

	/** Current remote player position */
	FVector CurrentRemotePosition = FVector::ZeroVector;

	/** Decode Opus data to PCM */
	bool DecodeOpus(const TArray<uint8>& OpusData, TArray<float>& OutPCMData);

	/** Process PCM through Steam Audio HRTF */
	bool ProcessHRTF(const TArray<float>& PCMData, const FVector& SourcePosition, 
		const FVector& ListenerPosition, const FRotator& ListenerRotation, 
		TArray<float>& OutBinauralData);

	/** Play binaural audio via Unreal audio system */
	void PlayBinauralAudio(const TArray<float>& BinauralData, int32 SampleRate);

	/** Initialize Steam Audio plugin */
	bool InitializeSteamAudio();

	/** Cleanup Steam Audio plugin */
	void CleanupSteamAudio();
};

