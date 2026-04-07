// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "AIFacemask/AIFacemaskImprovManager.h"
#include "AIFacemask/AIFacemaskFaceController.h"
#include "AIFacemask/AIFacemaskScriptManager.h"
#include "HoloCadeAI/Public/AIGRPCClient.h"
#include "HoloCadeAI/Public/AIHTTPClient.h"
#include "HoloCadeAI/Public/ILLMProvider.h"
#include "Json.h"
#include "JsonUtilities.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/Base64.h"

UAIFacemaskImprovManager::UAIFacemaskImprovManager()
{
	// Initialize facemask-specific members
	// Base class handles all initialization
}

void UAIFacemaskImprovManager::BeginPlay()
{
	Super::BeginPlay();
	
	// Find AIFacemaskFaceController component on the same actor
	AActor* Owner = GetOwner();
	if (Owner)
	{
		FaceController = Owner->FindComponentByClass<UAIFacemaskFaceController>();
		if (!FaceController)
		{
			UE_LOG(LogTemp, Warning, TEXT("UAIFacemaskImprovManager: No UAIFacemaskFaceController found on owner actor. Facial animation streaming will be disabled."));
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("UAIFacemaskImprovManager: Found UAIFacemaskFaceController - facial animation streaming enabled"));
		}

		// Phase 11: Find ScriptManager for transition buffering
		ScriptManager = Owner->FindComponentByClass<UAIFacemaskScriptManager>();
		if (!ScriptManager)
		{
			UE_LOG(LogTemp, Warning, TEXT("UAIFacemaskImprovManager: No UAIFacemaskScriptManager found on owner actor. Transition buffering will be disabled."));
		}
	}
}

void UAIFacemaskImprovManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	// Base class handles async operation tracking
	// No facemask-specific timing needed
}

bool UAIFacemaskImprovManager::InitializeImprovManager()
{
	// Copy facemask config to base config
	ImprovConfig = FacemaskImprovConfig.BaseConfig;
	
	// Initialize base class (which initializes LLMProviderManager, HTTPClient, GRPCClient)
	bool bSuccess = Super::InitializeImprovManager();
	
	if (bSuccess)
	{
		UE_LOG(LogTemp, Log, TEXT("UAIFacemaskImprovManager: Initialized with voice type: %d"), (int32)FacemaskImprovConfig.VoiceType);
	}
	
	return bSuccess;
}

void UAIFacemaskImprovManager::GenerateAndPlayImprovResponse(const FString& Input, bool bAsync)
{
	// Call base class which handles the async pipeline
	Super::GenerateAndPlayImprovResponse(Input, bAsync);
	
	// Facemask-specific logic can be added here if needed
	// The base class will call RequestLLMResponseAsync, which we override below
}

void UAIFacemaskImprovManager::StopCurrentResponse()
{
	Super::StopCurrentResponse();
	
	// Stop face controller streaming if needed
	if (FaceController && FaceController->IsConnected())
	{
		// Note: Face controller will continue receiving data from ACE server
		// We may want to add a method to pause/resume streaming
		UE_LOG(LogTemp, Log, TEXT("UAIFacemaskImprovManager: Stopped current response, face controller streaming continues"));
	}
}

void UAIFacemaskImprovManager::RequestLLMResponseAsync(const FString& Input, const FString& SystemPrompt, const TArray<FString>& InConversationHistory)
{
	// Phase 11: Apply prompt context (from base class generic implementation)
	FString ContextualInput = BuildImprovPromptWithContext(Input, false);
	
	if (!LLMProviderManager || !LLMProviderManager->IsProviderAvailable())
	{
		UE_LOG(LogTemp, Error, TEXT("UAIFacemaskImprovManager: Cannot request LLM - provider manager not available"));
		bIsGeneratingResponse = false;
		return;
	}

	// Build LLM request using provider manager interface
	FLLMRequest LLMRequest;
	LLMRequest.PlayerInput = ContextualInput;
	LLMRequest.SystemPrompt = SystemPrompt;
	LLMRequest.ConversationHistory = InConversationHistory;
	LLMRequest.ModelName = ImprovConfig.LLMModelName;
	LLMRequest.Temperature = ImprovConfig.LLMTemperature;
	LLMRequest.MaxTokens = ImprovConfig.MaxResponseTokens;

	bIsLLMRequestPending = true;

	UE_LOG(LogTemp, Log, TEXT("UAIFacemaskImprovManager: Requesting LLM response via provider manager (model: %s)"), *LLMRequest.ModelName);

	// Request response via provider manager (handles Ollama, OpenAI-compatible, etc.)
	LLMProviderManager->RequestResponse(LLMRequest, [this, Input](const FLLMResponse& Response)
	{
		bIsLLMRequestPending = false;

		if (Response.ErrorMessage.IsEmpty() && !Response.ResponseText.IsEmpty())
		{
			CurrentAIResponse = Response.ResponseText;
			
			// Add to conversation history
			ConversationHistory.Add(FString::Printf(TEXT("Player: %s"), *Input));
			ConversationHistory.Add(FString::Printf(TEXT("AI: %s"), *Response.ResponseText));
			
			// Trim conversation history if needed
			if (ConversationHistory.Num() > MaxConversationHistory * 2)
			{
				ConversationHistory.RemoveAt(0, ConversationHistory.Num() - MaxConversationHistory * 2);
			}

			// Broadcast response generated event
			OnImprovResponseGenerated.Broadcast(Input, Response.ResponseText);
			
			// Broadcast response started event
			OnImprovResponseStarted.Broadcast(Response.ResponseText);

			// Mark response as queued (will be marked as spoken when face animation starts)
			CurrentAIResponseState = EImprovResponseState::Queued;

			// Trigger TTS pipeline (which will trigger Audio2Face automatically)
			if (ImprovConfig.bUseLocalTTS)
			{
				RequestTTSConversion(Response.ResponseText);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("UAIFacemaskImprovManager: Cloud TTS not yet implemented"));
				OnTTSConversionComplete(TEXT(""), TArray<uint8>());
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("UAIFacemaskImprovManager: LLM request failed: %s"), *Response.ErrorMessage);
			bIsGeneratingResponse = false;
		}
	});
}

void UAIFacemaskImprovManager::RequestTTSConversion(const FString& Text)
{
	if (!ImprovConfig.bUseLocalTTS)
	{
		UE_LOG(LogTemp, Warning, TEXT("UAIFacemaskImprovManager: Cloud TTS not yet implemented"));
		return;
	}

	if (!GRPCClient || !GRPCClient->IsInitialized())
	{
		UE_LOG(LogTemp, Error, TEXT("UAIFacemaskImprovManager: Cannot request TTS - gRPC client not initialized"));
		return;
	}

	// Use facemask-specific voice type
	FString VoiceName = GetVoiceNameString(FacemaskImprovConfig.VoiceType);

	UE_LOG(LogTemp, Log, TEXT("UAIFacemaskImprovManager: Requesting TTS conversion from %s (voice: %s, text length: %d)"), 
		*ImprovConfig.LocalTTSEndpointURL, *VoiceName, Text.Len());

	// Build TTS request
	FAITTSRequest TTSRequest;
	TTSRequest.Text = Text;
	TTSRequest.VoiceName = VoiceName;
	TTSRequest.SampleRate = 48000;  // Standard sample rate
	TTSRequest.LanguageCode = TEXT("en-US");  // TODO: Make configurable

	bIsTTSRequestPending = true;

	// Request TTS synthesis via gRPC
	GRPCClient->RequestTTSSynthesis(TTSRequest, [this, Text](const FAITTSResponse& Response)
	{
		bIsTTSRequestPending = false;

		if (Response.AudioData.Num() == 0)
		{
			UE_LOG(LogTemp, Error, TEXT("UAIFacemaskImprovManager: TTS conversion returned empty audio data"));
			OnTTSConversionComplete(TEXT(""), TArray<uint8>());
			return;
		}

		// Save audio to temporary file
		FString TempDir = FPaths::ProjectSavedDir() / TEXT("Temp");
		IFileManager::Get().MakeDirectory(*TempDir, true);
		
		FString AudioFileName = FString::Printf(TEXT("improv_tts_%d.wav"), FDateTime::Now().ToUnixTimestamp());
		FString AudioFilePath = TempDir / AudioFileName;
		
		// Write audio data to file (assuming WAV format from Riva)
		if (FFileHelper::SaveArrayToFile(Response.AudioData, *AudioFilePath))
		{
			UE_LOG(LogTemp, Log, TEXT("UAIFacemaskImprovManager: TTS audio saved to: %s"), *AudioFilePath);
			TempAudioFilePath = AudioFilePath;
			OnTTSConversionComplete(AudioFilePath, Response.AudioData);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("UAIFacemaskImprovManager: Failed to save TTS audio to file"));
			OnTTSConversionComplete(TEXT(""), Response.AudioData);
		}
	});
}

void UAIFacemaskImprovManager::RequestAudio2FaceConversion(const FString& AudioFilePath)
{
	if (!ImprovConfig.bUseLocalAudio2Face)
	{
		UE_LOG(LogTemp, Warning, TEXT("UAIFacemaskImprovManager: Cloud Audio2Face not yet implemented"));
		return;
	}

	if (!HTTPClient || ImprovConfig.LocalAudio2FaceEndpointURL.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("UAIFacemaskImprovManager: Cannot request Audio2Face - HTTP client or endpoint URL not configured"));
		return;
	}

	if (AudioFilePath.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("UAIFacemaskImprovManager: Cannot request Audio2Face - audio file path is empty"));
		return;
	}

	// Read audio file and encode to base64
	TArray<uint8> AudioData;
	if (!FFileHelper::LoadFileToArray(AudioData, *AudioFilePath))
	{
		UE_LOG(LogTemp, Error, TEXT("UAIFacemaskImprovManager: Failed to load audio file: %s"), *AudioFilePath);
		return;
	}

	const FString Base64Audio = FBase64::Encode(AudioData);

	// Build Audio2Face request JSON
	TSharedPtr<FJsonObject> RequestJson = MakeShareable(new FJsonObject);
	RequestJson->SetStringField(TEXT("audio_file"), Base64Audio);
	RequestJson->SetStringField(TEXT("format"), TEXT("wav"));
	RequestJson->SetBoolField(TEXT("stream"), true);  // Stream facial animation frames in real-time

	// Build URL
	FString Audio2FaceURL = ImprovConfig.LocalAudio2FaceEndpointURL;
	if (!Audio2FaceURL.EndsWith(TEXT("/")))
	{
		Audio2FaceURL += TEXT("/");
	}
	Audio2FaceURL += TEXT("api/audio2face/convert");

	UE_LOG(LogTemp, Log, TEXT("UAIFacemaskImprovManager: Requesting Audio2Face conversion from %s (audio: %s, size: %d bytes)"), 
		*Audio2FaceURL, *AudioFilePath, AudioData.Num());

	bIsAudio2FaceRequestPending = true;

	// Send HTTP POST request
	TMap<FString, FString> Headers;
	HTTPClient->PostJSON(Audio2FaceURL, RequestJson, Headers, [this](const FAIHTTPResult& Result)
	{
		bIsAudio2FaceRequestPending = false;

		if (Result.bSuccess && Result.ResponseCode == 200)
		{
			UE_LOG(LogTemp, Log, TEXT("UAIFacemaskImprovManager: Audio2Face conversion started successfully"));
			
			// Parse response to get streaming endpoint or initial data
			TSharedPtr<FJsonObject> ResponseJson;
			if (UAIHTTPClient::ParseJSONResponse(Result.ResponseBody, ResponseJson) && ResponseJson.IsValid())
			{
				// Check if response contains streaming endpoint or initial animation data
				if (ResponseJson->HasField(TEXT("stream_endpoint")))
				{
					FString StreamEndpoint = ResponseJson->GetStringField(TEXT("stream_endpoint"));
					UE_LOG(LogTemp, Log, TEXT("UAIFacemaskImprovManager: Audio2Face streaming endpoint: %s"), *StreamEndpoint);
					// TODO: Connect to WebSocket stream endpoint for real-time facial animation data
					// For now, the face controller should already be connected to the ACE endpoint
				}
				else if (ResponseJson->HasField(TEXT("status")))
				{
					FString Status = ResponseJson->GetStringField(TEXT("status"));
					if (Status == TEXT("started") || Status == TEXT("processing"))
					{
						UE_LOG(LogTemp, Log, TEXT("UAIFacemaskImprovManager: Audio2Face conversion in progress"));
					}
				}
			}

			OnAudio2FaceConversionComplete(true);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("UAIFacemaskImprovManager: Audio2Face conversion failed (Code: %d, Error: %s)"), 
				Result.ResponseCode, *Result.ErrorMessage);
			OnAudio2FaceConversionComplete(false);
		}
	});
}

void UAIFacemaskImprovManager::OnTTSConversionComplete(const FString& AudioFilePath, const TArray<uint8>& AudioData)
{
	if (AudioFilePath.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("UAIFacemaskImprovManager: TTS conversion completed but no audio file path"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("UAIFacemaskImprovManager: TTS conversion complete, triggering Audio2Face conversion"));
	
	// Automatically trigger Audio2Face conversion after TTS completes (callback chain)
	if (ImprovConfig.bUseLocalAudio2Face)
	{
		RequestAudio2FaceConversion(AudioFilePath);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("UAIFacemaskImprovManager: Audio2Face disabled, skipping conversion"));
		OnAudio2FaceConversionComplete(false);
	}
}

void UAIFacemaskImprovManager::OnAudio2FaceConversionComplete(bool bSuccess)
{
	if (bSuccess)
	{
		UE_LOG(LogTemp, Log, TEXT("UAIFacemaskImprovManager: Audio2Face conversion complete, facial animation should be streaming"));
		
		// Note: Facial animation data is streamed to FaceController via WebSocket
		// The FaceController is already connected to the ACE endpoint
		// Audio2Face server streams data to that endpoint, which FaceController receives
		
		// Mark response as spoken when face animation starts (Audio2Face conversion complete = animation streaming)
		MarkCurrentResponseAsSpoken();
		
		// Broadcast response finished event
		OnImprovResponseFinished.Broadcast(CurrentAIResponse);
		
		// Clean up temporary audio file
		if (!TempAudioFilePath.IsEmpty() && FPaths::FileExists(TempAudioFilePath))
		{
			IFileManager::Get().Delete(*TempAudioFilePath);
			TempAudioFilePath.Empty();
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("UAIFacemaskImprovManager: Audio2Face conversion failed"));
	}
	
	bIsGeneratingResponse = false;
}

FString UAIFacemaskImprovManager::GetVoiceNameString(EHoloCadeACEVoiceType VoiceType) const
{
	switch (VoiceType)
	{
		case EHoloCadeACEVoiceType::Default:
			return TEXT("English-US-Female");
		case EHoloCadeACEVoiceType::Male:
			return TEXT("English-US-Male");
		case EHoloCadeACEVoiceType::Female:
			return TEXT("English-US-Female");
		case EHoloCadeACEVoiceType::Custom:
			return FacemaskImprovConfig.CustomVoiceModelID.IsEmpty() ? TEXT("Custom") : FacemaskImprovConfig.CustomVoiceModelID;
		default:
			return TEXT("English-US-Female");
	}
}

void UAIFacemaskImprovManager::MarkCurrentResponseAsSpoken()
{
	if (CurrentAIResponseState == EImprovResponseState::Queued)
	{
		CurrentAIResponseState = EImprovResponseState::Spoken;
		UE_LOG(LogTemp, Log, TEXT("UAIFacemaskImprovManager: Marked current response as spoken (face animation started)"));
	}
}

void UAIFacemaskImprovManager::NotifyNarrativeStateChanged(FName OldState, FName NewState, int32 NewStateIndex)
{
	if (!ScriptManager)
	{
		return;
	}

	// Phase 11: Check if current state's sentence has been spoken
	FAIFacemaskScript NewStateScript = ScriptManager->GetScriptForState(NewState);
	bool bCurrentStateSpoken = false;
	if (NewStateScript.ScriptLines.Num() > 0)
	{
		bCurrentStateSpoken = NewStateScript.ScriptLines[0].bHasBeenSpoken;
	}

	// Phase 11: Scenario A - Current state's sentence NOT spoken, improv active
	// Action: LLM immediately starts calculating transition sentence
	if (!bCurrentStateSpoken && bIsGeneratingResponse)
	{
		FString ContextText = NewStateScript.ScriptLines.Num() > 0 ? NewStateScript.ScriptLines[0].TextPrompt : TEXT("");
		RequestTransitionSentence(OldState, NewState, ContextText);
	}

	// Phase 11: Scenario B - Current state's sentence ALREADY spoken, improv begins
	// Action: LLM immediately starts buffering next state's transition
	// (This will be handled when improv actually starts - we'll check for next state)
	
	// Phase 11: Scenario C - Improv active, actor advances to next state (unspoken sentence)
	// Action: Transition sentence (if buffered) plays immediately, then narrative sentence
	// (This is handled by checking if transition is ready when state changes)
}

// Base class now handles LLM call by default - no override needed unless we want to customize
// (Currently using base class implementation which is sufficient for facemask)
