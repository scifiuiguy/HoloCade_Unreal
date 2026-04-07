// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ILLMProvider.h"
#include "IContainerManager.h"
#include "LLMProviderManager.generated.h"

// Forward declarations
class ULLMProviderOllama;
class ULLMProviderOpenAICompatible;

/**
 * LLM Provider Type
 */
UENUM(BlueprintType)
enum class ELLMProviderType : uint8
{
	/** Auto-detect from endpoint URL */
	AutoDetect UMETA(DisplayName = "Auto-Detect"),
	
	/** Ollama provider */
	Ollama UMETA(DisplayName = "Ollama"),
	
	/** OpenAI-compatible provider (NIM, vLLM, OpenAI, Claude, etc.) */
	OpenAICompatible UMETA(DisplayName = "OpenAI-Compatible"),
	
	/** Custom provider (implement ILLMProvider) */
	Custom UMETA(DisplayName = "Custom")
};

/**
 * LLM Provider Manager
 * 
 * Manages LLM provider instances and enables hot-swapping at runtime.
 * Similar to MCP (Model Context Protocol) - provides extensible LLM backend system.
 * 
 * **NVIDIA NIM Containerized Hot-Swapping:**
 * 
 * NIM runs as Docker containers, making hot-swapping seamless:
 * 
 * ```bash
 * # Start Llama 3.2 container
 * docker run -d -p 8000:8000 --gpus all nvcr.io/nim/llama-3.2-3b-instruct:latest
 * 
 * # Later, swap to Mistral (different port)
 * docker run -d -p 8001:8000 --gpus all nvcr.io/nim/mistral-7b-instruct:latest
 * 
 * # In Unreal: Update config endpoint URL from localhost:8000 to localhost:8001
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
 * ULLMProviderManager* ProviderManager = GetProviderManager();
 * 
 * // Hot-swap to different model
 * ProviderManager->SetProviderEndpoint("http://localhost:8001", ELLMProviderType::OpenAICompatible);
 * 
 * // Request response (uses current provider)
 * FLLMRequest Request;
 * Request.PlayerInput = "Hello!";
 * ProviderManager->RequestResponse(Request, [](const FLLMResponse& Response) {
 *     UE_LOG(LogTemp, Log, TEXT("Response: %s"), *Response.ResponseText);
 * });
 * ```
 */
UCLASS()
class HOLOCADEAI_API ULLMProviderManager : public UObject
{
	GENERATED_BODY()

public:
	ULLMProviderManager();

	/**
	 * Initialize provider manager
	 * @param EndpointURL - LLM endpoint URL
	 * @param ProviderType - Provider type (or AutoDetect)
	 * @param ModelName - Model name/ID
	 * @param ContainerConfig - Optional container config for auto-start (if bAutoStartContainer is true)
	 * @param bAutoStartContainer - Whether to auto-start container if not running
	 * @return true if initialization successful
	 */
	UFUNCTION(BlueprintCallable, Category = "LLM Provider")
	bool InitializeProvider(const FString& EndpointURL, ELLMProviderType ProviderType = ELLMProviderType::AutoDetect, const FString& ModelName = TEXT(""), const FContainerConfig& ContainerConfig = FContainerConfig(), bool bAutoStartContainer = false);

	/**
	 * Hot-swap to different provider/endpoint at runtime
	 * @param EndpointURL - New endpoint URL
	 * @param ProviderType - Provider type (or AutoDetect)
	 * @param ModelName - Model name/ID
	 * @return true if swap successful
	 */
	UFUNCTION(BlueprintCallable, Category = "LLM Provider")
	bool SetProviderEndpoint(const FString& EndpointURL, ELLMProviderType ProviderType = ELLMProviderType::AutoDetect, const FString& ModelName = TEXT(""));

	/**
	 * Request LLM response (uses current provider)
	 * @param Request - LLM request parameters
	 * @param Callback - Callback when response ready
	 */
	// NOTE: Not a UFUNCTION because TFunction callbacks are not supported by UHT
	void RequestResponse(const FLLMRequest& Request, TFunction<void(const FLLMResponse&)> Callback);

	/**
	 * Get current provider
	 * NOTE: Returns raw pointer - not exposed to Blueprint due to interface pointer limitation
	 */
	ILLMProvider* GetCurrentProvider() const { return CurrentProvider; }

	/**
	 * Get current provider name
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "LLM Provider")
	FString GetCurrentProviderName() const;

	/**
	 * Get supported models from current provider
	 */
	UFUNCTION(BlueprintCallable, Category = "LLM Provider")
	TArray<FString> GetSupportedModels() const;

	/**
	 * Check if provider is available
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "LLM Provider")
	bool IsProviderAvailable() const;

	/**
	 * Register custom provider (for extensibility)
	 * @param Provider - Custom provider implementing ILLMProvider
	 */
	UFUNCTION(BlueprintCallable, Category = "LLM Provider")
	void RegisterCustomProvider(TScriptInterface<ILLMProvider> Provider);

private:
	/**
	 * Auto-detect provider type from endpoint URL
	 */
	ELLMProviderType DetectProviderType(const FString& EndpointURL) const;

	/**
	 * Create provider instance based on type
	 */
	ILLMProvider* CreateProvider(ELLMProviderType ProviderType, const FString& EndpointURL, const FString& ModelName);

	/** Current LLM provider */
	ILLMProvider* CurrentProvider = nullptr;

	/** Ollama provider instance */
	UPROPERTY()
	TObjectPtr<class ULLMProviderOllama> OllamaProvider;

	/** OpenAI-compatible provider instance */
	UPROPERTY()
	TObjectPtr<class ULLMProviderOpenAICompatible> OpenAIProvider;

	/** Custom provider instance */
	UPROPERTY()
	TScriptInterface<ILLMProvider> CustomProvider;

	/** Container manager for auto-starting containers (optional) */
	UPROPERTY()
	TObjectPtr<class UContainerManagerDockerCLI> ContainerManager;
};

