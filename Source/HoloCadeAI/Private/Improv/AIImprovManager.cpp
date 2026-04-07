// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "Improv/AIImprovManager.h"
#include "AIHTTPClient.h"
#include "AIGRPCClient.h"
#include "LLMProviderManager.h"
#include "ContainerManagerDockerCLI.h"

UAIImprovManager::UAIImprovManager()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	
	bIsInitialized = false;
	bIsGeneratingResponse = false;
	MaxConversationHistory = 10;
}

void UAIImprovManager::BeginPlay()
{
	Super::BeginPlay();

	if (!HTTPClient)
	{
		HTTPClient = NewObject<UAIHTTPClient>(this, TEXT("HTTPClient"));
	}
	if (!GRPCClient)
	{
		GRPCClient = NewObject<UAIGRPCClient>(this, TEXT("GRPCClient"));
	}
}

void UAIImprovManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	// Generic improv manager doesn't handle timing
	// Subclasses should override for experience-specific timing logic
}

bool UAIImprovManager::InitializeImprovManager()
{
	if (bIsInitialized)
	{
		UE_LOG(LogTemp, Warning, TEXT("AIImprovManager: Already initialized"));
		return true;
	}

	if (!ImprovConfig.bEnableImprov)
	{
		UE_LOG(LogTemp, Log, TEXT("AIImprovManager: Improv is disabled in config"));
		return false;
	}

	// Initialize LLM Provider Manager with container auto-start if configured
	if (!LLMProviderManager)
	{
		LLMProviderManager = NewObject<ULLMProviderManager>(this);
	}

	// Extract port from endpoint URL for container config (if not already set)
	FContainerConfig LLMContainerConfig = ImprovConfig.ContainerConfig;
	if (ImprovConfig.bAutoStartContainer && LLMContainerConfig.ContainerName.IsEmpty())
	{
		// Auto-generate container name from model name
		LLMContainerConfig.ContainerName = FString::Printf(TEXT("holocade-llm-%s"), 
			*ImprovConfig.LLMModelName.Replace(TEXT(":"), TEXT("-")).Replace(TEXT(" "), TEXT("-")));
		
		// Extract port from endpoint URL if not set
		if (LLMContainerConfig.HostPort == 8000 && LLMContainerConfig.ContainerPort == 8000)
		{
			FString EndpointURL = ImprovConfig.LocalLLMEndpointURL;
			FString PortStr;
			if (EndpointURL.Split(TEXT(":"), nullptr, &PortStr))
			{
				int32 Port = FCString::Atoi(*PortStr);
				if (Port > 0)
				{
					LLMContainerConfig.HostPort = Port;
				}
			}
		}

		// Set image name if not set (default to Llama 3.2 3B)
		if (LLMContainerConfig.ImageName.IsEmpty())
		{
			LLMContainerConfig.ImageName = TEXT("nvcr.io/nim/llama-3.2-3b-instruct:latest");
		}
	}

	// Initialize LLM provider with container auto-start
	if (!LLMProviderManager->InitializeProvider(
		ImprovConfig.LocalLLMEndpointURL,
		ImprovConfig.LLMProviderType,
		ImprovConfig.LLMModelName,
		LLMContainerConfig,
		ImprovConfig.bAutoStartContainer))
	{
		UE_LOG(LogTemp, Error, TEXT("AIImprovManager: Failed to initialize LLM provider"));
		// Continue anyway - might be using external LLM service
	}

	// Initialize gRPC client for TTS if using local TTS
	if (ImprovConfig.bUseLocalTTS && GRPCClient)
	{
		if (!GRPCClient->Initialize(ImprovConfig.LocalTTSEndpointURL))
		{
			UE_LOG(LogTemp, Error, TEXT("AIImprovManager: Failed to initialize gRPC client for TTS"));
			// Continue anyway - TTS might fail but LLM can still work
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("AIImprovManager: gRPC client initialized for TTS at %s"), 
				*ImprovConfig.LocalTTSEndpointURL);
		}
	}

	bIsInitialized = true;

	UE_LOG(LogTemp, Log, TEXT("AIImprovManager: Initialized with local LLM: %s, Local TTS: %s, Local Audio2Face: %s"), 
		*ImprovConfig.LocalLLMEndpointURL, 
		ImprovConfig.bUseLocalTTS ? *ImprovConfig.LocalTTSEndpointURL : TEXT("Cloud"),
		ImprovConfig.bUseLocalAudio2Face ? *ImprovConfig.LocalAudio2FaceEndpointURL : TEXT("Cloud"));
	return true;
}

FString UAIImprovManager::GenerateImprovResponse(const FString& Input)
{
	// Generic implementation - subclasses should override for full async pipeline
	UE_LOG(LogTemp, Log, TEXT("AIImprovManager: GenerateImprovResponse called (generic implementation)"));
	return TEXT("");
}

void UAIImprovManager::GenerateAndPlayImprovResponse(const FString& Input, bool bAsync)
{
	if (!bIsInitialized)
	{
		UE_LOG(LogTemp, Warning, TEXT("AIImprovManager: Cannot generate response - not initialized"));
		return;
	}

	if (bIsGeneratingResponse)
	{
		UE_LOG(LogTemp, Warning, TEXT("AIImprovManager: Already generating a response"));
		return;
	}

	bIsGeneratingResponse = true;
	CurrentInput = Input;

	// Build conversation context
	FString Context = BuildConversationContext(Input);

	// Request LLM response asynchronously
	RequestLLMResponseAsync(Input, ImprovConfig.SystemPrompt, ConversationHistory);
}

void UAIImprovManager::ClearConversationHistory()
{
	ConversationHistory.Empty();
	UE_LOG(LogTemp, Log, TEXT("AIImprovManager: Conversation history cleared"));
}

void UAIImprovManager::StopCurrentResponse()
{
	if (!bIsGeneratingResponse)
	{
		return;
	}

	bIsGeneratingResponse = false;
	CurrentInput.Empty();
	CurrentAIResponse.Empty();
	
	UE_LOG(LogTemp, Log, TEXT("AIImprovManager: Stopped current response"));
}

void UAIImprovManager::RequestLLMResponseAsync(const FString& Input, const FString& SystemPrompt, const TArray<FString>& InConversationHistory)
{
	// Phase 11: Build prompt with context for appropriate response size (generic)
	FString ContextualInput = BuildImprovPromptWithContext(Input, false);
	
	// Generic implementation - subclasses should override
	UE_LOG(LogTemp, Log, TEXT("AIImprovManager: RequestLLMResponseAsync called (generic implementation)"));
	// TODO: Implement generic LLM request
}

void UAIImprovManager::RequestTTSConversion(const FString& Text)
{
	// Generic implementation - subclasses should override
	UE_LOG(LogTemp, Log, TEXT("AIImprovManager: RequestTTSConversion called (generic implementation)"));
	// TODO: Implement generic TTS request
}

void UAIImprovManager::RequestAudio2FaceConversion(const FString& AudioFilePath)
{
	// Generic implementation - subclasses should override
	UE_LOG(LogTemp, Log, TEXT("AIImprovManager: RequestAudio2FaceConversion called (generic implementation)"));
	// TODO: Implement generic Audio2Face request
}

FString UAIImprovManager::BuildConversationContext(const FString& Input) const
{
	FString Context = ImprovConfig.SystemPrompt + TEXT("\n\n");
	
	// Add conversation history
	for (const FString& HistoryEntry : ConversationHistory)
	{
		Context += HistoryEntry + TEXT("\n");
	}
	
	// Add current input
	Context += TEXT("User: ") + Input + TEXT("\n");
	Context += TEXT("Assistant: ");
	
	return Context;
}

FString UAIImprovManager::BuildImprovPromptWithContext(const FString& Input, bool bIsTransition) const
{
	// Phase 11: Generic prompt context to ensure appropriate response size
	FString ContextualPrompt = Input;
	
	if (bIsTransition)
	{
		// Transition-specific context: brief connecting sentence
		ContextualPrompt = FString::Printf(TEXT("Generate a brief connecting sentence (1 sentence, 10-20 words) that smoothly transitions from the current conversation to this narrative line: \"%s\". Keep it natural and conversational."), *Input);
	}
	else
	{
		// Standard improv context: short, complete sentences
		ContextualPrompt = FString::Printf(TEXT("Respond to this in a short, complete sentence (1-2 sentences max, avoid single words or run-on paragraphs): %s"), *Input);
	}
	
	return ContextualPrompt;
}

FString UAIImprovManager::GetBufferedTransition(FName TargetState) const
{
	const FImprovTransition* Transition = BufferedTransitions.Find(TargetState);
	if (Transition && Transition->bIsReady)
	{
		return Transition->TransitionText;
	}
	return TEXT("");
}

bool UAIImprovManager::IsTransitionReady(FName TargetState) const
{
	const FImprovTransition* Transition = BufferedTransitions.Find(TargetState);
	return Transition && Transition->bIsReady;
}

void UAIImprovManager::RequestTransitionSentence(FName FromState, FName ToState, const FString& ContextText)
{
	if (!LLMProviderManager || !LLMProviderManager->IsProviderAvailable() || ContextText.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("AIImprovManager: Cannot request transition - LLM provider not available or context empty"));
		return;
	}

	// Build transition prompt using generic context builder
	FString TransitionPrompt = BuildImprovPromptWithContext(ContextText, true);
	
	// Create transition entry (not ready yet)
	FImprovTransition& Transition = BufferedTransitions.FindOrAdd(ToState);
	Transition.TargetStateName = ToState;
	Transition.bIsReady = false;
	Transition.GenerationStartTime = GetWorld()->GetTimeSeconds();
	Transition.TransitionText = TEXT("");

	UE_LOG(LogTemp, Log, TEXT("AIImprovManager: Requesting transition sentence from '%s' to '%s'"), 
		*FromState.ToString(), *ToState.ToString());

	// Request LLM response for transition (generic implementation using default LLM config)
	FLLMRequest LLMRequest;
	LLMRequest.PlayerInput = TransitionPrompt;
	LLMRequest.SystemPrompt = TEXT("You are a helpful AI assistant that generates brief, natural transition sentences.");
	LLMRequest.ConversationHistory = ConversationHistory;
	LLMRequest.ModelName = ImprovConfig.LLMModelName;
	LLMRequest.Temperature = ImprovConfig.LLMTemperature;
	LLMRequest.MaxTokens = 50;  // Short transitions only

	LLMProviderManager->RequestResponse(LLMRequest, [this, ToState](const FLLMResponse& Response)
	{
		if (Response.ErrorMessage.IsEmpty() && !Response.ResponseText.IsEmpty())
		{
			FImprovTransition* Transition = BufferedTransitions.Find(ToState);
			if (Transition)
			{
				Transition->TransitionText = Response.ResponseText;
				Transition->bIsReady = true;
				
				float GenerationTime = GetWorld()->GetTimeSeconds() - Transition->GenerationStartTime;
				UE_LOG(LogTemp, Log, TEXT("AIImprovManager: Transition sentence ready for state '%s' (generated in %.2fs): '%s'"), 
					*ToState.ToString(), GenerationTime, *Response.ResponseText);
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("AIImprovManager: Failed to generate transition sentence: %s"), *Response.ErrorMessage);
		}
	});
}

void UAIImprovManager::OnTTSConversionComplete(const FString& AudioFilePath, const TArray<uint8>& AudioData)
{
	// Generic implementation - triggers Audio2Face
	TempAudioFilePath = AudioFilePath;
	RequestAudio2FaceConversion(AudioFilePath);
}

void UAIImprovManager::OnAudio2FaceConversionComplete(bool bSuccess)
{
	// Generic implementation - subclasses should override to stream to experience-specific handlers
	if (bSuccess)
	{
		UE_LOG(LogTemp, Log, TEXT("AIImprovManager: Audio2Face conversion completed successfully"));
		OnImprovResponseFinished.Broadcast(CurrentAIResponse);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("AIImprovManager: Audio2Face conversion failed"));
	}
	
	bIsGeneratingResponse = false;
}

