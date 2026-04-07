// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ILLMProvider.h"
#include "LLMProviderOpenAICompatible.generated.h"

/**
 * OpenAI-Compatible LLM Provider
 * 
 * Implements ILLMProvider for OpenAI-compatible APIs.
 * Supports:
 * - NVIDIA NIM (containerized, hot-swappable)
 * - vLLM
 * - OpenAI API
 * - Claude API (if OpenAI-compatible)
 * - Any other OpenAI-compatible service
 * 
 * **NVIDIA NIM Hot-Swapping:**
 * NIM runs as Docker containers, enabling easy model swapping:
 * - Each model container exposes OpenAI-compatible API on port 8000
 * - Swap models by changing endpoint URL to different container port
 * - No code changes required - just update config
 */
UCLASS()
class HOLOCADEAI_API ULLMProviderOpenAICompatible : public UObject, public ILLMProvider
{
	GENERATED_BODY()

public:
	ULLMProviderOpenAICompatible();

	/** OpenAI-compatible endpoint URL (e.g., "http://localhost:8000" for NIM) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LLM Provider")
	FString EndpointURL;

	/** API key (optional, for cloud services) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LLM Provider")
	FString APIKey;

	/** Initialize provider */
	UFUNCTION(BlueprintCallable, Category = "LLM Provider")
	void Initialize(const FString& InEndpointURL, const FString& InAPIKey = TEXT(""));

	// ILLMProvider interface
	virtual void RequestResponse(const FLLMRequest& Request, TFunction<void(const FLLMResponse&)> Callback) override;
	virtual bool IsAvailable() const override;
	virtual FString GetProviderName() const override { return TEXT("OpenAI-Compatible"); }
	virtual TArray<FString> GetSupportedModels() const override;

private:
	class UAIHTTPClient* HTTPClient;
	bool bIsInitialized = false;
};

