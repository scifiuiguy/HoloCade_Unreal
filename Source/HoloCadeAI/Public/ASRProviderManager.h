// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "IASRProvider.h"
#include "IContainerManager.h"
#include "ASRProviderManager.generated.h"

// Forward declarations
class UASRProviderRiva;
class UASRProviderNIM;
class UAIGRPCClient;
class UASRTranscriptionCallbackProxy;
class UASRProviderManager;

/**
 * ASR Provider Type
 */
UENUM(BlueprintType)
enum class EASRProviderType : uint8
{
	/** Auto-detect from endpoint URL */
	AutoDetect UMETA(DisplayName = "Auto-Detect"),
	
	/** NVIDIA Riva ASR */
	Riva UMETA(DisplayName = "NVIDIA Riva ASR"),
	
	/** NVIDIA NIM ASR models (Parakeet, Canary, Whisper) */
	NIM UMETA(DisplayName = "NVIDIA NIM ASR"),
	
	/** Custom provider (implement IASRProvider) */
	Custom UMETA(DisplayName = "Custom")
};

/**
 * @brief Manages ASR providers, enabling hot-swapping and extensibility.
 * This class acts as a factory and registry for different ASR backends.
 * 
 * **NVIDIA NIM Containerized Hot-Swapping:**
 * 
 * NIM runs as Docker containers, making hot-swapping seamless:
 * 
 * ```bash
 * # Start Riva ASR container
 * docker run -d -p 50051:50051 --gpus all nvcr.io/nim/riva-asr:latest
 * 
 * # Later, swap to Parakeet (different port)
 * docker run -d -p 50052:50051 --gpus all nvcr.io/nim/parakeet-rnnt-1.1b:latest
 * 
 * # In Unreal: Update config endpoint URL from localhost:50051 to localhost:50052
 * # System automatically uses new model - no code changes!
 * ```
 * 
 * **Benefits:**
 * - Hot-swap models at runtime (change endpoint URL)
 * - Run multiple models simultaneously (different ports)
 * - Easy A/B testing (swap between models)
 * - Container isolation (each model in separate container)
 * - No code changes required (just config update)
 * 
 * **Usage:**
 * ```cpp
 * // Get provider manager
 * UASRProviderManager* ProviderManager = GetASRProviderManager();
 * 
 * // Hot-swap to different model
 * ProviderManager->SetProviderEndpoint("localhost:50052", EASRProviderType::NIM);
 * 
 * // Request transcription (uses current provider)
 * FASRRequest Request;
 * Request.AudioData = AudioBytes;
 * Request.SampleRate = 48000;
 * ProviderManager->RequestTranscription(Request, [](const FASRResponse& Response) {
 *     UE_LOG(LogTemp, Log, TEXT("Transcribed: %s"), *Response.TranscribedText);
 * });
 * ```
 */
UCLASS()
class HOLOCADEAI_API UASRTranscriptionCallbackProxy : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(UASRProviderManager* InOwner, TFunction<void(const FASRResponse&)> InCallback);

	UFUNCTION()
	void HandleResponse(const FASRResponse& Response);

private:
	TFunction<void(const FASRResponse&)> NativeCallback;
	TWeakObjectPtr<UASRProviderManager> Owner;
};

UCLASS(BlueprintType)
class HOLOCADEAI_API UASRProviderManager : public UObject
{
	GENERATED_BODY()

public:
	UASRProviderManager();

	/**
	 * @brief Initializes the ASR provider manager and its default providers.
	 * @param InGRPCClient The gRPC client to pass to providers.
	 * @param InDefaultEndpointURL The default endpoint URL for auto-detection.
	 * @param InDefaultProviderType The default provider type if not auto-detecting.
	 * @param ContainerConfig Optional container config for auto-start (if bAutoStartContainer is true).
	 * @param bAutoStartContainer Whether to auto-start container if not running.
	 * @return True if initialization is successful.
	 */
	UFUNCTION(BlueprintCallable, Category = "ASR Provider")
	bool Initialize(UAIGRPCClient* InGRPCClient, const FString& InDefaultEndpointURL, EASRProviderType InDefaultProviderType = EASRProviderType::AutoDetect, const FContainerConfig& ContainerConfig = FContainerConfig(), bool bAutoStartContainer = false);

	/**
	 * @brief Gets the currently active ASR provider.
	 * @return The active ASR provider, or nullptr if none is active.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ASR Provider")
	TScriptInterface<IASRProvider> GetActiveProvider() const;

	/**
	 * @brief Sets the active ASR provider by type and endpoint.
	 * @param ProviderType The type of provider to activate.
	 * @param EndpointURL The endpoint URL for the provider.
	 * @return True if the provider was successfully activated.
	 */
	UFUNCTION(BlueprintCallable, Category = "ASR Provider")
	bool SetActiveProvider(EASRProviderType ProviderType, const FString& EndpointURL);

	/**
	 * @brief Hot-swap to different provider/endpoint at runtime
	 * @param EndpointURL New endpoint URL
	 * @param ProviderType Provider type (or AutoDetect)
	 * @return true if swap successful
	 */
	UFUNCTION(BlueprintCallable, Category = "ASR Provider")
	bool SetProviderEndpoint(const FString& EndpointURL, EASRProviderType ProviderType = EASRProviderType::AutoDetect);

	/**
	 * @brief Request ASR transcription (uses current provider)
	 * @param Request ASR request parameters
	 * @param Callback Callback when transcription ready
	 */
	// NOTE: Not a UFUNCTION because TFunction callbacks are not supported by UHT
	void RequestTranscription(const FASRRequest& Request, TFunction<void(const FASRResponse&)> Callback);

	/**
	 * @brief Registers a custom ASR provider.
	 * Custom providers must implement the IASRProvider interface.
	 * @param CustomProvider The custom provider to register.
	 * @param ProviderName The name to associate with this custom provider.
	 */
	UFUNCTION(BlueprintCallable, Category = "ASR Provider")
	void RegisterCustomProvider(TScriptInterface<IASRProvider> CustomProvider, FName ProviderName);

	/**
	 * @brief Unregisters a custom ASR provider.
	 * @param ProviderName The name of the custom provider to unregister.
	 */
	UFUNCTION(BlueprintCallable, Category = "ASR Provider")
	void UnregisterCustomProvider(FName ProviderName);

	/**
	 * @brief Gets a list of all registered ASR provider names.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ASR Provider")
	TArray<FString> GetAllProviderNames() const;

	/**
	 * @brief Gets a list of models supported by the active provider.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ASR Provider")
	TArray<FString> GetActiveProviderSupportedModels() const;

	/**
	 * @brief Checks if the active provider supports streaming recognition.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ASR Provider")
	bool ActiveProviderSupportsStreaming() const;

private:
	friend class UASRTranscriptionCallbackProxy;

	/** The currently active ASR provider */
	UPROPERTY()
	TScriptInterface<IASRProvider> ActiveProvider;

	/** Map of registered ASR providers by type */
	UPROPERTY()
	TMap<EASRProviderType, TScriptInterface<IASRProvider>> DefaultProviders;

	/** Map of custom registered ASR providers by name */
	UPROPERTY()
	TMap<FName, TScriptInterface<IASRProvider>> CustomProviders;

	/** gRPC client to pass to providers */
	UPROPERTY()
	TObjectPtr<UAIGRPCClient> GRPCClientRef;

	/** Active callback proxy objects to keep them alive until responses arrive */
	UPROPERTY()
	TArray<TObjectPtr<UASRTranscriptionCallbackProxy>> ActiveCallbackProxies;

	/**
	 * @brief Automatically detects the provider type from the endpoint URL.
	 * @param EndpointURL The URL to analyze.
	 * @return The detected ASR provider type.
	 */
	EASRProviderType AutoDetectProviderType(const FString& EndpointURL) const;

	/** Container manager for auto-starting containers (optional) */
	UPROPERTY()
	TObjectPtr<class UContainerManagerDockerCLI> ContainerManager;

	void HandleCallbackProxyFinished(UASRTranscriptionCallbackProxy* Proxy);
};

