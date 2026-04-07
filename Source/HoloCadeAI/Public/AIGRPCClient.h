// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "AIAPI.h"  // For HOLOCADEAI_API macro - must be before .generated.h

// TurboLink includes (conditional compilation) - must be before .generated.h
#if WITH_TURBOLINK
#include "TurboLinkGrpcManager.h"
#include "TurboLinkGrpcService.h"
#include "TurboLinkGrpcClient.h"
#endif // WITH_TURBOLINK

#include "AIGRPCClient.generated.h"  // Must be last include

/**
 * gRPC Request Result
 */
USTRUCT(BlueprintType)
struct HOLOCADEAI_API FAIGRPCResult
{
	GENERATED_BODY()

	/** Whether the request was successful */
	UPROPERTY(BlueprintReadOnly, Category = "gRPC")
	bool bSuccess = false;

	/** Response data (format depends on service) */
	UPROPERTY(BlueprintReadOnly, Category = "gRPC")
	FString ResponseData;

	/** Error message if request failed */
	UPROPERTY(BlueprintReadOnly, Category = "gRPC")
	FString ErrorMessage;

	/** gRPC status code (0 = OK) */
	UPROPERTY(BlueprintReadOnly, Category = "gRPC")
	int32 StatusCode = 0;

	FAIGRPCResult()
		: bSuccess(false)
		, StatusCode(0)
	{
	}

	FAIGRPCResult(bool bInSuccess, const FString& InResponseData, const FString& InErrorMessage = TEXT(""), int32 InStatusCode = 0)
		: bSuccess(bInSuccess)
		, ResponseData(InResponseData)
		, ErrorMessage(InErrorMessage)
		, StatusCode(InStatusCode)
	{
	}
};

/**
 * ASR (Automatic Speech Recognition) Request Parameters
 */
USTRUCT(BlueprintType)
struct HOLOCADEAI_API FAIASRRequest
{
	GENERATED_BODY()

	/** Audio data (PCM format) */
	UPROPERTY(BlueprintReadWrite, Category = "ASR")
	TArray<uint8> AudioData;

	/** Sample rate (typically 48000 for Mumble) */
	UPROPERTY(BlueprintReadWrite, Category = "ASR")
	int32 SampleRate = 48000;

	/** Language code (e.g., "en-US") */
	UPROPERTY(BlueprintReadWrite, Category = "ASR")
	FString LanguageCode = TEXT("en-US");

	/** Audio format (e.g., "pcm", "wav") */
	UPROPERTY(BlueprintReadWrite, Category = "ASR")
	FString AudioFormat = TEXT("pcm");

	FAIASRRequest()
		: SampleRate(48000)
		, LanguageCode(TEXT("en-US"))
		, AudioFormat(TEXT("pcm"))
	{
	}
};

/**
 * ASR Response (transcribed text)
 */
USTRUCT(BlueprintType)
struct HOLOCADEAI_API FAIASRResponse
{
	GENERATED_BODY()

	/** Transcribed text */
	UPROPERTY(BlueprintReadOnly, Category = "ASR")
	FString TranscribedText;

	/** Confidence score (0.0 to 1.0) */
	UPROPERTY(BlueprintReadOnly, Category = "ASR")
	float Confidence = 0.0f;

	/** Whether transcription is final (vs partial/interim) */
	UPROPERTY(BlueprintReadOnly, Category = "ASR")
	bool bIsFinal = true;

	FAIASRResponse()
		: Confidence(0.0f)
		, bIsFinal(true)
	{
	}
};

/**
 * TTS (Text-to-Speech) Request Parameters
 */
USTRUCT(BlueprintType)
struct HOLOCADEAI_API FAITTSRequest
{
	GENERATED_BODY()

	/** Text to synthesize */
	UPROPERTY(BlueprintReadWrite, Category = "TTS")
	FString Text;

	/** Voice name/ID (e.g., "English-US-Female") */
	UPROPERTY(BlueprintReadWrite, Category = "TTS")
	FString VoiceName;

	/** Sample rate (typically 48000) */
	UPROPERTY(BlueprintReadWrite, Category = "TTS")
	int32 SampleRate = 48000;

	/** Language code (e.g., "en-US") */
	UPROPERTY(BlueprintReadWrite, Category = "TTS")
	FString LanguageCode = TEXT("en-US");

	FAITTSRequest()
		: SampleRate(48000)
		, LanguageCode(TEXT("en-US"))
	{
	}
};

/**
 * TTS Response (synthesized audio)
 */
USTRUCT(BlueprintType)
struct HOLOCADEAI_API FAITTSResponse
{
	GENERATED_BODY()

	/** Synthesized audio data (PCM/WAV format) */
	UPROPERTY(BlueprintReadOnly, Category = "TTS")
	TArray<uint8> AudioData;

	/** Sample rate of audio */
	UPROPERTY(BlueprintReadOnly, Category = "TTS")
	int32 SampleRate = 48000;

	/** Audio format (e.g., "pcm", "wav") */
	UPROPERTY(BlueprintReadOnly, Category = "TTS")
	FString AudioFormat = TEXT("pcm");

	FAITTSResponse()
		: SampleRate(48000)
		, AudioFormat(TEXT("pcm"))
	{
	}
};

/**
 * Generic gRPC Client for AI Services
 * 
 * Provides async gRPC request/response handling for:
 * - ASR (Automatic Speech Recognition) - converts audio to text
 * - TTS (Text-to-Speech) - converts text to audio
 * - Other gRPC-based AI services
 * 
 * **Implementation:**
 * Uses TurboLink gRPC plugin for native gRPC support in Unreal Engine.
 * TurboLink must be installed as a git submodule in Plugins/TurboLink/
 * 
 * **Setup:**
 * 1. Run: .\Source\AI\Common\SetupTurboLink.ps1
 * 2. Regenerate Visual Studio project files
 * 3. Build project (may require compatibility fixes for UE 5.5.4)
 * 4. Enable TurboLink plugin in Unreal Editor
 * 
 * **Supported Services:**
 * - NVIDIA Riva ASR/TTS (gRPC)
 * - NVIDIA NIM ASR models (Parakeet, Canary, Whisper)
 * - Other gRPC-based AI services
 * 
 * **Fallback:**
 * If TurboLink is not installed, this will use NOOP implementation with warnings.
 */
UCLASS()
class HOLOCADEAI_API UAIGRPCClient : public UObject
{
	GENERATED_BODY()

public:
	UAIGRPCClient();
	virtual ~UAIGRPCClient();

	/**
	 * Initialize gRPC client
	 * @param ServerAddress - gRPC server address (e.g., "localhost:50051")
	 * @return True if initialization successful
	 */
	UFUNCTION(BlueprintCallable, Category = "gRPC")
	bool Initialize(const FString& ServerAddress);

	/**
	 * Request ASR transcription (async)
	 * @param Request - ASR request parameters (audio data, sample rate, language)
	 * @param Callback - Callback function called when request completes
	 */
	void RequestASRTranscription(const FAIASRRequest& Request, TFunction<void(const FAIASRResponse&)> Callback);

	/**
	 * Request TTS synthesis (async)
	 * @param Request - TTS request parameters (text, voice, sample rate)
	 * @param Callback - Callback function called when request completes
	 */
	void RequestTTSSynthesis(const FAITTSRequest& Request, TFunction<void(const FAITTSResponse&)> Callback);

	/**
	 * Check if gRPC client is initialized
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "gRPC")
	bool IsInitialized() const { return bIsInitialized; }

	/**
	 * Get current server address
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "gRPC")
	FString GetServerAddress() const { return ServerAddress; }

private:
	/** Whether client is initialized */
	bool bIsInitialized = false;

	/** gRPC server address */
	FString ServerAddress;

#if WITH_TURBOLINK
	/** TurboLink gRPC manager (for ASR/TTS services) */
	class UTurboLinkGrpcManager* GrpcManager = nullptr;

	/** TurboLink ASR service client */
	class UTurboLinkGrpcClient* ASRClient = nullptr;

	/** TurboLink TTS service client */
	class UTurboLinkGrpcClient* TTSClient = nullptr;
#endif // WITH_TURBOLINK

	/**
	 * Internal: Execute ASR gRPC call
	 * Uses TurboLink if available, otherwise NOOP
	 */
	void ExecuteASRCall(const FAIASRRequest& Request, TFunction<void(const FAIASRResponse&)> Callback);

	/**
	 * Internal: Execute TTS gRPC call
	 * Uses TurboLink if available, otherwise NOOP
	 */
	void ExecuteTTSCall(const FAITTSRequest& Request, TFunction<void(const FAITTSResponse&)> Callback);
};

