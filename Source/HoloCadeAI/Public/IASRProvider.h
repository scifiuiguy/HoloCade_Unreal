// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "IASRProvider.generated.h"

// Forward declarations
class UAIGRPCClient;

/**
 * @brief Structure to hold ASR request parameters.
 */
USTRUCT(BlueprintType)
struct FASRRequest
{
	GENERATED_BODY()

	/** PCM audio data (16-bit, little-endian) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ASR")
	TArray<uint8> AudioData;

	/** Audio sample rate (typically 48000 for Mumble) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ASR")
	int32 SampleRate = 48000;

	/** Language code (e.g., "en-US", "en-GB") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ASR")
	FString LanguageCode = TEXT("en-US");

	/** Whether to use streaming recognition (real-time) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ASR")
	bool bUseStreaming = true;

	FString EndpointURL;
};

/**
 * @brief Structure to hold ASR response data.
 */
USTRUCT(BlueprintType)
struct FASRResponse
{
	GENERATED_BODY()

	/** Transcribed text */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ASR")
	FString TranscribedText;

	/** Whether transcription was successful */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ASR")
	bool bSuccess = false;

	/** Error message if transcription failed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ASR")
	FString ErrorMessage;

	/** Confidence score (0.0 to 1.0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ASR")
	float Confidence = 0.0f;

	FASRResponse()
		: bSuccess(false)
		, Confidence(0.0f)
	{}
};

// Delegate for asynchronous ASR responses
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnASRResponseReceived, const FASRResponse&, Response);

/**
 * @brief Interface for ASR providers.
 * This interface allows for hot-swapping different ASR backends (Riva, Parakeet, Canary, etc.)
 * without modifying the core AIFacemaskASRManager.
 * 
 * **NVIDIA NIM Containerized Approach:**
 * NIM runs as Docker containers, making it perfect for hot-swapping:
 * - Each ASR model runs in its own container
 * - Containers can be started/stopped independently
 * - Multiple models can run simultaneously on different ports
 * - Easy to swap models by changing endpoint URL
 * 
 * **Supported Providers:**
 * - NVIDIA Riva ASR (containerized, gRPC streaming)
 * - Parakeet via NIM (containerized, gRPC streaming)
 * - Canary via NIM (containerized, gRPC streaming, includes translation)
 * - Whisper via NIM (containerized, gRPC offline only - not recommended for real-time)
 * - Any custom provider implementing this interface
 * 
 * **Hot-Swapping Workflow:**
 * 1. Start new ASR container/service
 * 2. Update endpoint URL in config
 * 3. System automatically uses new provider (no code changes)
 * 
 * **Example with NIM:**
 * ```bash
 * # Start Riva ASR container
 * docker run -d -p 50051:50051 --gpus all nvcr.io/nim/riva-asr:latest
 * 
 * # Later, swap to Parakeet
 * docker stop <riva-container>
 * docker run -d -p 50052:50051 --gpus all nvcr.io/nim/parakeet-rnnt-1.1b:latest
 * # Update config: LocalASREndpointURL = "localhost:50052"
 * ```
 */
UINTERFACE(MinimalAPI, Blueprintable)
class UASRProvider : public UInterface
{
	GENERATED_BODY()
};

class HOLOCADEAI_API IASRProvider
{
	GENERATED_BODY()

public:
	/**
	 * @brief Initializes the ASR provider.
	 * @param InGRPCClient The gRPC client to use for requests.
	 * @param InEndpointURL The base URL for the ASR API (gRPC endpoint).
	 * @return True if initialization is successful, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "ASR Provider")
	bool Initialize(UAIGRPCClient* InGRPCClient, const FString& InEndpointURL);

	/**
	 * @brief Requests ASR transcription asynchronously.
	 * @param Request The ASR request parameters.
	 * @param Callback The delegate to call when the response is received.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "ASR Provider")
	void RequestASRTranscription(const FASRRequest& Request, const FOnASRResponseReceived& Callback);

	/**
	 * @brief Checks if the ASR provider is available and ready to process requests.
	 * @return True if the provider is available, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "ASR Provider")
	bool IsAvailable() const;

	/**
	 * @brief Gets the name of the ASR provider.
	 * @return The name of the provider (e.g., "Riva ASR", "Parakeet", "Canary").
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "ASR Provider")
	FString GetProviderName() const;

	/**
	 * @brief Gets a list of models supported by this provider.
	 * @return An array of supported model names.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "ASR Provider")
	TArray<FString> GetSupportedModels() const;

	/**
	 * @brief Gets whether this provider supports streaming recognition.
	 * @return True if streaming is supported, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "ASR Provider")
	bool SupportsStreaming() const;
};

