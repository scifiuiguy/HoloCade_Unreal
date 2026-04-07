// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "AIAPI.h"  // For HOLOCADEAI_API macro
#include "ILLMProvider.generated.h"

/**
 * LLM Request Parameters
 */
USTRUCT(BlueprintType)
struct HOLOCADEAI_API FLLMRequest
{
	GENERATED_BODY()

	/** Player input text */
	UPROPERTY(BlueprintReadWrite, Category = "LLM")
	FString PlayerInput;

	/** System prompt/character context */
	UPROPERTY(BlueprintReadWrite, Category = "LLM")
	FString SystemPrompt;

	/** Conversation history (formatted as "Player: ..." or "AI: ...") */
	UPROPERTY(BlueprintReadWrite, Category = "LLM")
	TArray<FString> ConversationHistory;

	/** Model name/ID to use */
	UPROPERTY(BlueprintReadWrite, Category = "LLM")
	FString ModelName;

	/** Temperature (0.0 = deterministic, 1.0+ = creative) */
	UPROPERTY(BlueprintReadWrite, Category = "LLM")
	float Temperature = 0.7f;

	/** Maximum response tokens */
	UPROPERTY(BlueprintReadWrite, Category = "LLM")
	int32 MaxTokens = 150;

	FLLMRequest()
		: Temperature(0.7f)
		, MaxTokens(150)
	{}
};

/**
 * LLM Response
 */
USTRUCT(BlueprintType)
struct HOLOCADEAI_API FLLMResponse
{
	GENERATED_BODY()

	/** Generated response text */
	UPROPERTY(BlueprintReadOnly, Category = "LLM")
	FString ResponseText;

	/** Indicates whether the provider call succeeded */
	UPROPERTY(BlueprintReadOnly, Category = "LLM")
	bool bSuccess = false;

	/** Whether the response is complete (vs partial/streaming) */
	UPROPERTY(BlueprintReadOnly, Category = "LLM")
	bool bIsComplete = true;

	/** Error message if request failed */
	UPROPERTY(BlueprintReadOnly, Category = "LLM")
	FString ErrorMessage;

	FLLMResponse()
		: bSuccess(false)
		, bIsComplete(true)
	{}
};

/**
 * LLM Provider Interface
 * 
 * Extensible interface for LLM backends, similar to MCP (Model Context Protocol).
 * Enables hot-swapping LLM providers at runtime without code changes.
 * 
 * **NVIDIA NIM Containerized Approach:**
 * NIM runs as Docker containers, making it perfect for hot-swapping:
 * - Each model runs in its own container
 * - Containers can be started/stopped independently
 * - Multiple models can run simultaneously on different ports
 * - Easy to swap models by changing endpoint URL
 * 
 * **Supported Providers:**
 * - NVIDIA NIM (containerized, hot-swappable)
 * - Ollama (local, supports LoRA)
 * - vLLM (high-performance inference)
 * - Claude API (cloud)
 * - OpenAI API (cloud)
 * - Any custom provider implementing this interface
 * 
 * **Hot-Swapping Workflow:**
 * 1. Start new LLM container/service
 * 2. Update endpoint URL in config
 * 3. System automatically uses new provider (no code changes)
 * 
 * **Example with NIM:**
 * ```bash
 * # Start Llama 3.2 container
 * docker run -d -p 8000:8000 nvcr.io/nim/llama-3.2-3b-instruct:latest
 * 
 * # Later, swap to Mistral
 * docker stop <llama-container>
 * docker run -d -p 8001:8000 nvcr.io/nim/mistral-7b-instruct:latest
 * # Update config: LocalLLMEndpointURL = "http://localhost:8001"
 * ```
 */
UINTERFACE(MinimalAPI, BlueprintType)
class ULLMProvider : public UInterface
{
	GENERATED_BODY()
};

class HOLOCADEAI_API ILLMProvider
{
	GENERATED_BODY()

public:
	/**
	 * Request LLM response (async)
	 * @param Request - LLM request parameters
	 * @param Callback - Callback function called when response is ready
	 */
	virtual void RequestResponse(const FLLMRequest& Request, TFunction<void(const FLLMResponse&)> Callback) = 0;

	/**
	 * Check if provider is available/ready
	 * @return true if provider can handle requests
	 */
	virtual bool IsAvailable() const = 0;

	/**
	 * Get provider name/identifier
	 */
	virtual FString GetProviderName() const = 0;

	/**
	 * Get supported model names (for discovery)
	 */
	virtual TArray<FString> GetSupportedModels() const = 0;
};

