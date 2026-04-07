// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "IVOIPAudioVisitor.h"
#include "AIGRPCClient.h"
#include "ASRProviderManager.h"
#include "IContainerManager.h"
#include "AIASRManager.generated.h"

// Forward declarations (must be at global scope for UHT)
class UAIGRPCClient;
class UASRProviderManager;

/**
 * Generic configuration for ASR (Automatic Speech Recognition)
 * NOTE: USTRUCT must be at global scope (Unreal Engine limitation)
 */
USTRUCT(BlueprintType)
struct HOLOCADEAI_API FAIASRConfig
{
	GENERATED_BODY()

	/** Whether ASR is enabled */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|ASR")
	bool bEnableASR = true;

	/** 
	 * Local ASR endpoint URL
	 * Supports multiple backends (all via gRPC for low latency):
	 * - NVIDIA Riva ASR (gRPC): "localhost:50051"
	 * - Parakeet via NIM (gRPC): "localhost:50052"
	 * - Canary via NIM (gRPC): "localhost:50053"
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|ASR")
	FString LocalASREndpointURL;

	/** Whether to use local ASR or cloud ASR */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|ASR")
	bool bUseLocalASR = true;

	/** Language code for ASR (e.g., "en-US", "en-GB") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|ASR")
	FString LanguageCode = TEXT("en-US");

	/** Minimum audio duration to trigger ASR (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|ASR", meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float MinAudioDuration = 0.5f;

	/** Maximum audio duration to process (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|ASR", meta = (ClampMin = "1.0", ClampMax = "30.0"))
	float MaxAudioDuration = 10.0f;

	/**
	 * Whether to auto-start container if not running (for NIM containers)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|ASR|Container")
	bool bAutoStartContainer = false;

	/**
	 * Container configuration (only used if bAutoStartContainer is true)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|ASR|Container")
	FContainerConfig ContainerConfig;

	FAIASRConfig()
		: bEnableASR(true)
		, LocalASREndpointURL(TEXT("localhost:50051"))
		, bUseLocalASR(true)
		, LanguageCode(TEXT("en-US"))
		, MinAudioDuration(0.5f)
		, MaxAudioDuration(10.0f)
		, bAutoStartContainer(false)
	{}
};

/**
 * Delegate for ASR transcription events
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnASRTranscriptionComplete, int32, SourceId, const FString&, TranscribedText);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnASRTranscriptionStarted, int32, SourceId);

/**
 * Generic ASR Manager Component
 * 
 * Base class for managing Automatic Speech Recognition (ASR).
 * Provides generic ASR functionality without experience-specific logic.
 * 
 * Subclasses should extend this for experience-specific needs:
 * - Auto-triggering improv responses after transcription
 * - Experience-specific transcription handling
 * - Experience-specific source identification
 * 
 * ARCHITECTURE:
 * - Runs on dedicated server (receives audio from VOIP)
 * - Receives audio streams via IVOIPAudioVisitor pattern
 * - Converts speech to text using local ASR (NVIDIA Riva, Parakeet, Canary via gRPC)
 * - Broadcasts transcription events for experience-specific handling
 * 
 * NOTE: UCLASS must be at global scope (Unreal Engine limitation)
 */
UCLASS(ClassGroup=(HoloCade), meta=(BlueprintSpawnableComponent))
class HOLOCADEAI_API UAIASRManager : public UActorComponent, public IVOIPAudioVisitor
{
	GENERATED_BODY()

public:
	UAIASRManager();

	/** Configuration for ASR */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|ASR")
	FAIASRConfig ASRConfig;

	/** Event fired when transcription completes */
	UPROPERTY(BlueprintAssignable, Category = "AI|ASR")
	FOnASRTranscriptionComplete OnTranscriptionComplete;

	/** Event fired when transcription starts */
	UPROPERTY(BlueprintAssignable, Category = "AI|ASR")
	FOnASRTranscriptionStarted OnTranscriptionStarted;

	/**
	 * Initialize the ASR manager
	 * @return true if initialization was successful
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|ASR")
	virtual bool InitializeASRManager();

	/**
	 * Process audio data from a source (called by VOIPManager when audio is received)
	 * @param SourceId - Source ID who spoke
	 * @param AudioData - PCM audio data (from Mumble, decoded from Opus)
	 * @param SampleRate - Audio sample rate (typically 48000 for Mumble)
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|ASR")
	virtual void ProcessAudio(int32 SourceId, const TArray<float>& AudioData, int32 SampleRate);

	// IVOIPAudioVisitor implementation
	virtual void OnPlayerAudioReceived(int32 PlayerId, const TArray<float>& AudioData, int32 SampleRate, const FVector& Position) override;

	/**
	 * Manually trigger transcription for a source (if audio buffering is enabled)
	 * @param SourceId - Source ID to transcribe
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|ASR")
	virtual void TriggerTranscriptionForSource(int32 SourceId);

	/**
	 * Check if a source is currently being transcribed
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI|ASR")
	bool IsSourceBeingTranscribed(int32 SourceId) const;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Whether the ASR manager is initialized */
	bool bIsInitialized = false;

	/** Audio buffers per source (for voice activity detection and buffering) */
	TMap<int32, TArray<float>> SourceAudioBuffers;

	/** Timestamps for when audio started per source */
	TMap<int32, float> SourceAudioStartTimes;

	/** Whether each source is currently speaking (voice activity detection) */
	TMap<int32, bool> SourceSpeakingStates;

	/** Whether transcription is in progress per source */
	TMap<int32, bool> SourceTranscribingStates;

	/** Timer for voice activity detection (silence detection) */
	float VoiceActivityTimer = 0.0f;

	/** Silence duration threshold (seconds) - if exceeded, trigger transcription */
	UPROPERTY(EditAnywhere, Category = "AI|ASR")
	float SilenceThreshold = 1.0f;

	/**
	 * Request ASR transcription from local ASR service
	 * Subclasses can override for custom transcription handling
	 */
	virtual void RequestASRTranscription(int32 SourceId, const TArray<float>& AudioData, int32 SampleRate);

	/**
	 * Handle transcription result
	 * Subclasses can override for experience-specific handling (e.g., trigger improv)
	 */
	virtual void HandleTranscriptionResult(int32 SourceId, const FString& TranscribedText);

	/**
	 * Detect voice activity in audio buffer (simple energy-based VAD)
	 */
	virtual bool DetectVoiceActivity(const TArray<float>& AudioData) const;

	/**
	 * Clear audio buffer for a source
	 */
	virtual void ClearSourceAudioBuffer(int32 SourceId);

	/**
	 * Convert PCM float audio data to uint8 bytes for gRPC
	 */
	TArray<uint8> ConvertPCMFloatToBytes(const TArray<float>& FloatAudio, int32 SampleRate) const;

	/** gRPC client for ASR transcription */
	UPROPERTY()
	TObjectPtr<UAIGRPCClient> GRPCClient;

	/** ASR Provider Manager (enables hot-swapping, extensibility) */
	UPROPERTY()
	TObjectPtr<UASRProviderManager> ASRProviderManager;
};
