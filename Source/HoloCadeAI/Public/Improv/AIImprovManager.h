// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AIHTTPClient.h"
#include "AIGRPCClient.h"
#include "LLMProviderManager.h"
#include "IContainerManager.h"
#include "AIImprovManager.generated.h"

// Forward declarations (must be at global scope for UHT)
class UAIHTTPClient;
class UAIGRPCClient;
class ULLMProviderManager;

/**
 * Generic configuration for improvised responses
 * NOTE: USTRUCT must be at global scope (Unreal Engine limitation)
 */
USTRUCT(BlueprintType)
struct HOLOCADEAI_API FAIImprovConfig
{
	GENERATED_BODY()

	/** Whether improvised responses are enabled */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Improv")
	bool bEnableImprov = true;

	/** 
	 * Local LLM endpoint URL
	 * Supports multiple backends:
	 * - Ollama: "http://localhost:11434"
	 * - vLLM: "http://localhost:8000"
	 * - NVIDIA NIM: "http://localhost:8000" (containerized, hot-swappable)
	 * - Any OpenAI-compatible API endpoint
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Improv")
	FString LocalLLMEndpointURL;

	/** 
	 * LLM model name/ID
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Improv")
	FString LLMModelName;

	/**
	 * LLM Provider Type
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Improv")
	ELLMProviderType LLMProviderType = ELLMProviderType::AutoDetect;

	/**
	 * Whether to auto-start container if not running (for NIM containers)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Improv|Container")
	bool bAutoStartContainer = false;

	/**
	 * Container configuration (only used if bAutoStartContainer is true)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Improv|Container")
	FContainerConfig ContainerConfig;

	/** System prompt/character context for the AI */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Improv", meta = (MultiLine = true))
	FString SystemPrompt;

	/** Maximum response length in tokens */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Improv", meta = (ClampMin = "10", ClampMax = "500"))
	int32 MaxResponseTokens = 150;

	/** Temperature for LLM generation (0.0 = deterministic, 1.0+ = creative) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Improv", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float LLMTemperature = 0.7f;

	/** Whether to use local TTS or cloud TTS */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Improv")
	bool bUseLocalTTS = true;

	/** 
	 * Local TTS endpoint URL
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Improv")
	FString LocalTTSEndpointURL;

	/** Whether to use local Audio2Face or cloud Audio2Face */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Improv")
	bool bUseLocalAudio2Face = true;

	/** 
	 * Local Audio2Face endpoint URL
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Improv")
	FString LocalAudio2FaceEndpointURL;

	FAIImprovConfig()
		: bEnableImprov(true)
		, LocalLLMEndpointURL(TEXT("http://localhost:8000"))
		, LLMModelName(TEXT("llama-3.2-3b-instruct"))
		, LLMProviderType(ELLMProviderType::OpenAICompatible)
		, bAutoStartContainer(false)
		, SystemPrompt(TEXT("You are a helpful AI assistant."))
		, MaxResponseTokens(150)
		, LLMTemperature(0.7f)
		, bUseLocalTTS(true)
		, LocalTTSEndpointURL(TEXT("http://localhost:50051"))
		, bUseLocalAudio2Face(true)
		, LocalAudio2FaceEndpointURL(TEXT("http://localhost:8000"))
	{}
};

/**
 * Delegate for improvised response events
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnImprovResponseGenerated, const FString&, Input, const FString&, AIResponse);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnImprovResponseStarted, const FString&, AIResponse);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnImprovResponseFinished, const FString&, AIResponse);

/**
 * Generic Improv Manager Component
 * 
 * Base class for managing real-time improvised AI responses.
 * Provides generic LLM + TTS + Audio2Face pipeline without experience-specific logic.
 * 
 * Subclasses should extend this for experience-specific needs:
 * - Face controller integration (for facial animation)
 * - Experience-specific voice/emotion settings
 * - Experience-specific response formatting
 * 
 * WORKFLOW:
 * 1. Receive text input
 * 2. Local LLM generates improvised response
 * 3. Local TTS converts text → audio
 * 4. Local Audio2Face converts audio → facial animation (or other output)
 * 5. Output streamed to experience-specific handler
 * 
 * NOTE: UCLASS must be at global scope (Unreal Engine limitation)
 */
UCLASS(ClassGroup=(HoloCade), meta=(BlueprintSpawnableComponent))
class HOLOCADEAI_API UAIImprovManager : public UActorComponent
{
	GENERATED_BODY()

public:
	UAIImprovManager();

	/** Configuration for improvised responses */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Improv")
	FAIImprovConfig ImprovConfig;

	/** Conversation history (for context-aware responses) */
	UPROPERTY(BlueprintReadOnly, Category = "AI|Improv")
	TArray<FString> ConversationHistory;

	/** Maximum conversation history entries to keep */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Improv", meta = (ClampMin = "1", ClampMax = "50"))
	int32 MaxConversationHistory = 10;

	/** Event fired when an improvised response is generated (text only) */
	UPROPERTY(BlueprintAssignable, Category = "AI|Improv")
	FOnImprovResponseGenerated OnImprovResponseGenerated;

	/** Event fired when improvised response playback starts */
	UPROPERTY(BlueprintAssignable, Category = "AI|Improv")
	FOnImprovResponseStarted OnImprovResponseStarted;

	/** Event fired when improvised response playback finishes */
	UPROPERTY(BlueprintAssignable, Category = "AI|Improv")
	FOnImprovResponseFinished OnImprovResponseFinished;

	/**
	 * Initialize the improv manager
	 * @return true if initialization was successful
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|Improv")
	virtual bool InitializeImprovManager();

	/**
	 * Generate an improvised response to input
	 * @param Input - Text input/question
	 * @return Generated AI response text (empty if generation failed)
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|Improv")
	virtual FString GenerateImprovResponse(const FString& Input);

	/**
	 * Generate and play an improvised response (text → LLM → TTS → Audio2Face)
	 * @param Input - Text input/question
	 * @param bAsync - If true, generation happens asynchronously
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|Improv")
	virtual void GenerateAndPlayImprovResponse(const FString& Input, bool bAsync = true);

	/**
	 * Clear conversation history
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|Improv")
	virtual void ClearConversationHistory();

	/**
	 * Check if improv is currently generating/playing a response
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI|Improv")
	bool IsGeneratingResponse() const { return bIsGeneratingResponse; }

	/**
	 * Stop current improv response generation/playback
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|Improv")
	virtual void StopCurrentResponse();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Whether the improv manager is initialized */
	bool bIsInitialized = false;

	/** Whether we're currently generating a response */
	bool bIsGeneratingResponse = false;

	/** Current input being processed */
	FString CurrentInput;

	/** Current AI response being generated/played */
	FString CurrentAIResponse;

	/**
	 * Request LLM response asynchronously
	 */
	virtual void RequestLLMResponseAsync(const FString& Input, const FString& SystemPrompt, const TArray<FString>& InConversationHistory);

	/**
	 * Request TTS conversion
	 * Subclasses can override for custom TTS handling
	 */
	virtual void RequestTTSConversion(const FString& Text);

	/**
	 * Request Audio2Face conversion
	 * Subclasses can override for custom Audio2Face handling
	 */
	virtual void RequestAudio2FaceConversion(const FString& AudioFilePath);

	/**
	 * Build conversation context for LLM
	 */
	virtual FString BuildConversationContext(const FString& Input) const;

	/**
	 * Phase 11: Build prompt with context for appropriate response size
	 * Generic implementation - subclasses can override for experience-specific context
	 */
	virtual FString BuildImprovPromptWithContext(const FString& Input, bool bIsTransition = false) const;

	/**
	 * Phase 11: Get buffered transition sentence for a target state
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI|Improv|Transition")
	FString GetBufferedTransition(FName TargetState) const;

	/**
	 * Phase 11: Check if transition is ready for a target state
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI|Improv|Transition")
	bool IsTransitionReady(FName TargetState) const;

	/**
	 * Phase 11: Request transition sentence generation
	 * Generic implementation - subclasses can override for experience-specific logic
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|Improv|Transition")
	virtual void RequestTransitionSentence(FName FromState, FName ToState, const FString& ContextText);

	/**
	 * Handle TTS conversion completion - triggers Audio2Face automatically
	 * Subclasses can override for custom handling
	 */
	virtual void OnTTSConversionComplete(const FString& AudioFilePath, const TArray<uint8>& AudioData);

	/**
	 * Handle Audio2Face conversion completion
	 * Subclasses can override to stream to experience-specific handlers
	 */
	virtual void OnAudio2FaceConversionComplete(bool bSuccess);

	/** HTTP client for LLM and Audio2Face requests */
	UPROPERTY()
	TObjectPtr<UAIHTTPClient> HTTPClient;

	/** gRPC client for TTS requests */
	UPROPERTY()
	TObjectPtr<UAIGRPCClient> GRPCClient;

	/** LLM Provider Manager (enables hot-swapping, extensibility) */
	UPROPERTY()
	TObjectPtr<ULLMProviderManager> LLMProviderManager;

	/** Async operation tracking */
	bool bIsLLMRequestPending = false;
	bool bIsTTSRequestPending = false;
	bool bIsAudio2FaceRequestPending = false;

	/** Temporary audio file path for TTS output */
	FString TempAudioFilePath;

protected:
	/**
	 * Phase 11: Transition buffer structure (generic)
	 */
	struct FImprovTransition
	{
		FString TransitionText;
		FName TargetStateName;
		bool bIsReady = false;
		float GenerationStartTime = 0.0f;
	};

	/** Phase 11: Buffered transitions by target state (generic) - protected so subclasses can access */
	TMap<FName, FImprovTransition> BufferedTransitions;
};
