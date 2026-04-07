// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ILLMProvider.h"
#include "LLMProviderOllama.generated.h"

/**
 * Ollama LLM Provider
 * 
 * Implements ILLMProvider for Ollama API.
 * Supports local Ollama instances and custom LoRA models.
 */
UCLASS()
class HOLOCADEAI_API ULLMProviderOllama : public UObject, public ILLMProvider
{
	GENERATED_BODY()

public:
	ULLMProviderOllama();

	/** Ollama endpoint URL (e.g., "http://localhost:11434") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LLM Provider")
	FString EndpointURL;

	/** Initialize provider */
	UFUNCTION(BlueprintCallable, Category = "LLM Provider")
	void Initialize(const FString& InEndpointURL);

	// ILLMProvider interface
	virtual void RequestResponse(const FLLMRequest& Request, TFunction<void(const FLLMResponse&)> Callback) override;
	virtual bool IsAvailable() const override;
	virtual FString GetProviderName() const override { return TEXT("Ollama"); }
	virtual TArray<FString> GetSupportedModels() const override;

private:
	class UAIHTTPClient* HTTPClient;
	bool bIsInitialized = false;
};

