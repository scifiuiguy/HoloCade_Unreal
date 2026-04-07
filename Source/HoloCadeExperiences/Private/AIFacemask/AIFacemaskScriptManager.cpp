// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "AIFacemask/AIFacemaskScriptManager.h"
#include "AIFacemask/AIFacemaskFaceController.h"
#include "AIFacemask/AIFacemaskImprovManager.h"
#include "HoloCadeAI/Public/AIHTTPClient.h"
#include "Json.h"
#include "JsonUtilities.h"

UAIFacemaskScriptManager::UAIFacemaskScriptManager()
{
	// Initialize facemask-specific members
	CurrentScriptLineIndex = -1;
	ScriptPlaybackTimer = 0.0f;
	CurrentScriptLineStartTime = 0.0f;
	bWaitingForStartDelay = false;
	StartDelayTimer = 0.0f;
}

void UAIFacemaskScriptManager::BeginPlay()
{
	Super::BeginPlay();
	
	// Find AIFacemaskFaceController component on the same actor
	AActor* Owner = GetOwner();
	if (Owner)
	{
		FaceController = Owner->FindComponentByClass<UAIFacemaskFaceController>();
		if (!FaceController)
		{
			UE_LOG(LogTemp, Warning, TEXT("UAIFacemaskScriptManager: No UAIFacemaskFaceController found on owner actor"));
		}
	}
}

void UAIFacemaskScriptManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsInitialized || !bIsPlayingScript)
	{
		return;
	}

	// Handle start delay
	if (bWaitingForStartDelay)
	{
		StartDelayTimer += DeltaTime;
		if (StartDelayTimer >= CurrentScript.StartDelay)
		{
			bWaitingForStartDelay = false;
			StartDelayTimer = 0.0f;
			// Start playing first script line
			if (CurrentScript.ScriptLines.Num() > 0)
			{
				StartScriptLine(0);
			}
		}
		return;
	}

	// Handle script line playback
	if (CurrentScriptLineIndex >= 0 && CurrentScriptLineIndex < CurrentScript.ScriptLines.Num())
	{
		const FAIFacemaskScriptLine& CurrentLine = CurrentScript.ScriptLines[CurrentScriptLineIndex];
		
		ScriptPlaybackTimer += DeltaTime;
		
		// Check if current line has finished (if we have duration estimate)
		if (CurrentLine.EstimatedDuration > 0.0f)
		{
			float ElapsedTime = ScriptPlaybackTimer - CurrentScriptLineStartTime;
			if (ElapsedTime >= CurrentLine.EstimatedDuration)
			{
				// Advance to next line or finish script
				AdvanceToNextScriptLine();
			}
		}
		// If no duration estimate, we rely on ACE server to signal completion
		// (This would require additional integration with ACE server callbacks)
	}
}

bool UAIFacemaskScriptManager::InitializeScriptManager(const FString& InAIServerBaseURL)
{
	ACEServerBaseURL = InAIServerBaseURL;
	return Super::InitializeScriptManager(InAIServerBaseURL);
}

bool UAIFacemaskScriptManager::PlayScript(FName ScriptID)
{
	// Map script ID to state name (for facemask-specific logic)
	// For facemask, ScriptID is the state name
	return TriggerScriptForState(ScriptID);
}

void UAIFacemaskScriptManager::StopCurrentScript()
{
	Super::StopCurrentScript();
	
	FName CurrentStateName = CurrentScript.AssociatedStateName;
	
	CurrentScriptLineIndex = -1;
	ScriptPlaybackTimer = 0.0f;
	CurrentScriptLineStartTime = 0.0f;
	bWaitingForStartDelay = false;
	StartDelayTimer = 0.0f;

	UE_LOG(LogTemp, Log, TEXT("UAIFacemaskScriptManager: Stopped script for state '%s'"), *CurrentStateName.ToString());
	
	// Stop face controller streaming if needed
	if (FaceController && FaceController->IsConnected())
	{
		// Note: Face controller will continue receiving data from ACE server
		// We may want to add a method to pause/resume streaming
	}
}

void UAIFacemaskScriptManager::PreBakeScript(FName ScriptID, bool bAsync)
{
	// Map to state name and call facemask-specific version
	PreBakeScriptForState(ScriptID, bAsync);
}

bool UAIFacemaskScriptManager::HasScript(FName ScriptID) const
{
	// Check facemask-specific script collection
	return ScriptCollection.GetScriptForState(ScriptID) != nullptr;
}

bool UAIFacemaskScriptManager::TriggerScriptForState(FName StateName)
{
	if (!bIsInitialized)
	{
		UE_LOG(LogTemp, Warning, TEXT("UAIFacemaskScriptManager: Cannot trigger script - not initialized"));
		return false;
	}

	// Stop any currently playing script
	if (bIsPlayingScript)
	{
		StopCurrentScript();
	}

	// Find script for this state
	FAIFacemaskScript* Script = ScriptCollection.GetScriptForState(StateName);
	if (!Script)
	{
		UE_LOG(LogTemp, Warning, TEXT("UAIFacemaskScriptManager: No script found for state '%s'"), *StateName.ToString());
		return false;
	}

	// Check if script is pre-baked
	if (!Script->bIsFullyPreBaked)
	{
		UE_LOG(LogTemp, Warning, TEXT("UAIFacemaskScriptManager: Script for state '%s' is not pre-baked. Pre-baking now..."), *StateName.ToString());
		// Attempt to pre-bake synchronously (this may take time)
		PreBakeScriptForState(StateName, false);
	}

	// Start playing the script
	CurrentScript = *Script;
	CurrentScriptLineIndex = -1;
	ScriptPlaybackTimer = 0.0f;
	bIsPlayingScript = true;
	bWaitingForStartDelay = (CurrentScript.StartDelay > 0.0f);
	StartDelayTimer = 0.0f;

	// Broadcast script started event
	OnScriptStarted.Broadcast(StateName, CurrentScript);

	UE_LOG(LogTemp, Log, TEXT("UAIFacemaskScriptManager: Started script for state '%s' (%d lines)"), 
		*StateName.ToString(), CurrentScript.ScriptLines.Num());

	// Request script playback from ACE server
	RequestScriptPlaybackFromACE(CurrentScript, 0);

	return true;
}

void UAIFacemaskScriptManager::PreBakeAllScripts(bool bAsync)
{
	UE_LOG(LogTemp, Log, TEXT("UAIFacemaskScriptManager: Pre-baking all scripts (async: %s)"), bAsync ? TEXT("true") : TEXT("false"));
	
	// NOOP: TODO - Implement async pre-baking
	// For now, pre-bake synchronously
	for (auto& ScriptPair : ScriptCollection.ScriptsByState)
	{
		PreBakeScriptForState(ScriptPair.Key, false);
	}
}

void UAIFacemaskScriptManager::PreBakeScriptForState(FName StateName, bool bAsync)
{
	FAIFacemaskScript* Script = ScriptCollection.GetScriptForState(StateName);
	if (!Script)
	{
		UE_LOG(LogTemp, Warning, TEXT("UAIFacemaskScriptManager: Cannot pre-bake - no script found for state '%s'"), *StateName.ToString());
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("UAIFacemaskScriptManager: Pre-baking script for state '%s' (async: %s)"), 
		*StateName.ToString(), bAsync ? TEXT("true") : TEXT("false"));

	// NOOP: TODO - Implement async pre-baking
	// For now, request pre-bake synchronously
	RequestScriptPreBakeFromACE(*Script);
}

FAIFacemaskScript UAIFacemaskScriptManager::GetScriptForState(FName StateName) const
{
	const FAIFacemaskScript* Script = ScriptCollection.GetScriptForState(StateName);
	if (Script)
	{
		return *Script;
	}
	return FAIFacemaskScript();
}

void UAIFacemaskScriptManager::HandleNarrativeStateChanged(FName OldState, FName NewState, int32 NewStateIndex)
{
	// Phase 11: Notify ImprovManager of state change for transition buffering
	AActor* Owner = GetOwner();
	if (Owner)
	{
		UAIFacemaskImprovManager* ImprovManager = Owner->FindComponentByClass<UAIFacemaskImprovManager>();
		if (ImprovManager)
		{
			ImprovManager->NotifyNarrativeStateChanged(OldState, NewState, NewStateIndex);
		}
	}

	if (bAutoTriggerOnStateChange)
	{
		UE_LOG(LogTemp, Log, TEXT("UAIFacemaskScriptManager: Narrative state changed to '%s', triggering script..."), *NewState.ToString());
		TriggerScriptForState(NewState);
	}
}

void UAIFacemaskScriptManager::RequestScriptPlayback(FName ScriptID)
{
	// Map to state name and call facemask-specific version
	FName StateName = ScriptID; // For facemask, ScriptID is the state name
	const FAIFacemaskScript* Script = ScriptCollection.GetScriptForState(StateName);
	if (Script)
	{
		RequestScriptPlaybackFromACE(*Script, 0);
	}
}

void UAIFacemaskScriptManager::RequestScriptPreBake(FName ScriptID)
{
	// Map to state name and call facemask-specific version
	FName StateName = ScriptID; // For facemask, ScriptID is the state name
	const FAIFacemaskScript* Script = ScriptCollection.GetScriptForState(StateName);
	if (Script)
	{
		RequestScriptPreBakeFromACE(*Script);
	}
}

void UAIFacemaskScriptManager::StartScriptLine(int32 LineIndex)
{
	if (!CurrentScript.ScriptLines.IsValidIndex(LineIndex))
	{
		UE_LOG(LogTemp, Error, TEXT("UAIFacemaskScriptManager: Invalid script line index %d"), LineIndex);
		return;
	}

	FAIFacemaskScriptLine& ScriptLine = CurrentScript.ScriptLines[LineIndex];
	
	// Phase 11: Check if line has been spoken (allow retreating but don't re-speak)
	if (ScriptLine.bHasBeenSpoken)
	{
		UE_LOG(LogTemp, Log, TEXT("UAIFacemaskScriptManager: Script line %d for state '%s' has already been spoken, skipping playback"), 
			LineIndex, *CurrentScript.AssociatedStateName.ToString());
		// Skip to next line or finish script
		AdvanceToNextScriptLine();
		return;
	}

	CurrentScriptLineIndex = LineIndex;
	CurrentScriptLineStartTime = ScriptPlaybackTimer;

	UE_LOG(LogTemp, Log, TEXT("UAIFacemaskScriptManager: Started script line %d: '%s'"), 
		LineIndex, *ScriptLine.TextPrompt);

	// Broadcast script line started event
	OnScriptLineStarted.Broadcast(CurrentScript.AssociatedStateName, LineIndex, ScriptLine);

	// Request this specific line from ACE server
	RequestScriptPlaybackFromACE(CurrentScript, LineIndex);
}

void UAIFacemaskScriptManager::AdvanceToNextScriptLine()
{
	if (CurrentScriptLineIndex < 0 || !CurrentScript.ScriptLines.IsValidIndex(CurrentScriptLineIndex))
	{
		return;
	}

	// Phase 11: Mark current line as spoken when it completes
	FAIFacemaskScriptLine& CurrentLine = CurrentScript.ScriptLines[CurrentScriptLineIndex];
	CurrentLine.bHasBeenSpoken = true;
	
	// Also update the script in the collection (so state persists)
	FAIFacemaskScript* ScriptInCollection = ScriptCollection.GetScriptForState(CurrentScript.AssociatedStateName);
	if (ScriptInCollection && CurrentScriptLineIndex < ScriptInCollection->ScriptLines.Num())
	{
		ScriptInCollection->ScriptLines[CurrentScriptLineIndex].bHasBeenSpoken = true;
	}
	
	UE_LOG(LogTemp, Log, TEXT("UAIFacemaskScriptManager: Marked script line %d as spoken for state '%s'"), 
		CurrentScriptLineIndex, *CurrentScript.AssociatedStateName.ToString());

	int32 NextLineIndex = CurrentScriptLineIndex + 1;

	// Check if there are more lines
	if (NextLineIndex < CurrentScript.ScriptLines.Num())
	{
		// Advance to next line
		StartScriptLine(NextLineIndex);
	}
	else
	{
		// Check if script should loop
		if (CurrentScript.bLoopScript)
		{
			// Loop back to first line
			StartScriptLine(0);
		}
		else
		{
			// Finish script
			FinishCurrentScript();
		}
	}
}

void UAIFacemaskScriptManager::FinishCurrentScript()
{
	FName StateName = CurrentScript.AssociatedStateName;
	
	CurrentScriptLineIndex = -1;
	ScriptPlaybackTimer = 0.0f;
	CurrentScriptLineStartTime = 0.0f;

	UE_LOG(LogTemp, Log, TEXT("UAIFacemaskScriptManager: Finished script for state '%s'"), *StateName.ToString());

	// Broadcast script finished event
	OnScriptFinished.Broadcast(StateName, CurrentScript);
	
	// Call base class to update playing state
	Super::StopCurrentScript();
}

void UAIFacemaskScriptManager::RequestScriptPlaybackFromACE(const FAIFacemaskScript& Script, int32 StartLineIndex)
{
	if (!HTTPClient || ACEServerBaseURL.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("UAIFacemaskScriptManager: Cannot request playback - HTTP client or server URL not configured"));
		return;
	}

	// Build playback request JSON
	TSharedPtr<FJsonObject> RequestJson = MakeShareable(new FJsonObject);
	RequestJson->SetStringField(TEXT("script_id"), Script.AssociatedStateName.ToString());
	RequestJson->SetNumberField(TEXT("start_line_index"), StartLineIndex);
	RequestJson->SetBoolField(TEXT("loop"), Script.bLoopScript);
	
	// Build script lines array
	TArray<TSharedPtr<FJsonValue>> ScriptLinesArray;
	for (const FAIFacemaskScriptLine& Line : Script.ScriptLines)
	{
		TSharedPtr<FJsonObject> LineJson = MakeShareable(new FJsonObject);
		LineJson->SetStringField(TEXT("script_line_id"), Line.ScriptLineID);
		LineJson->SetStringField(TEXT("text_prompt"), Line.TextPrompt);
		LineJson->SetStringField(TEXT("voice_type"), GetVoiceTypeString(Line.VoiceType));
		if (Line.VoiceType == EHoloCadeACEVoiceType::Custom && !Line.CustomVoiceModelID.IsEmpty())
		{
			LineJson->SetStringField(TEXT("custom_voice_model_id"), Line.CustomVoiceModelID);
		}
		LineJson->SetStringField(TEXT("emotion_preset"), GetEmotionPresetString(Line.EmotionPreset));
		if (!Line.PreBakedAudioPath.IsEmpty())
		{
			LineJson->SetStringField(TEXT("pre_baked_audio_path"), Line.PreBakedAudioPath);
		}
		
		ScriptLinesArray.Add(MakeShareable(new FJsonValueObject(LineJson)));
	}
	RequestJson->SetArrayField(TEXT("script_lines"), ScriptLinesArray);

	// Build URL
	FString PlaybackURL = ACEServerBaseURL;
	if (!PlaybackURL.EndsWith(TEXT("/")))
	{
		PlaybackURL += TEXT("/");
	}
	PlaybackURL += TEXT("api/playback/start");

	// Send HTTP POST request
	TMap<FString, FString> Headers;
	HTTPClient->PostJSON(PlaybackURL, RequestJson, Headers, [this, Script, StartLineIndex](const FAIHTTPResult& Result)
	{
		if (Result.bSuccess && Result.ResponseCode == 200)
		{
			UE_LOG(LogTemp, Log, TEXT("UAIFacemaskScriptManager: Script playback started successfully (State: %s, StartLine: %d)"), 
				*Script.AssociatedStateName.ToString(), StartLineIndex);
			
			// Parse response to get playback status
			TSharedPtr<FJsonObject> ResponseJson;
			if (UAIHTTPClient::ParseJSONResponse(Result.ResponseBody, ResponseJson) && ResponseJson.IsValid())
			{
				// Check if playback started successfully
				if (ResponseJson->HasField(TEXT("status")))
				{
					FString Status = ResponseJson->GetStringField(TEXT("status"));
					if (Status == TEXT("started"))
					{
						UE_LOG(LogTemp, Log, TEXT("UAIFacemaskScriptManager: ACE server confirmed playback started"));
					}
				}
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("UAIFacemaskScriptManager: Failed to start script playback (Code: %d, Error: %s)"), 
				Result.ResponseCode, *Result.ErrorMessage);
		}
	});
}

void UAIFacemaskScriptManager::RequestScriptPreBakeFromACE(const FAIFacemaskScript& Script)
{
	if (!HTTPClient || ACEServerBaseURL.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("UAIFacemaskScriptManager: Cannot request pre-bake - HTTP client or server URL not configured"));
		return;
	}

	FName StateName = Script.AssociatedStateName;
	
	// Mark script as being pre-baked
	ScriptsBeingPreBaked.Add(StateName, true);
	
	UE_LOG(LogTemp, Log, TEXT("UAIFacemaskScriptManager: Starting pre-bake for script (State: %s, %d lines)"), 
		*StateName.ToString(), Script.ScriptLines.Num());

	// Pre-bake each script line sequentially
	// For each line: TTS → Audio, then Audio → Facial data
	int32 LineIndex = 0;
	PreBakeScriptLineRecursive(Script, LineIndex);
}

void UAIFacemaskScriptManager::PreBakeScriptLineRecursive(const FAIFacemaskScript& Script, int32 LineIndex)
{
	if (LineIndex >= Script.ScriptLines.Num())
	{
		// All lines pre-baked
		FName StateName = Script.AssociatedStateName;
		ScriptsBeingPreBaked.Remove(StateName);
		
		// Mark script as fully pre-baked
		FAIFacemaskScript* MutableScript = ScriptCollection.GetScriptForState(StateName);
		if (MutableScript)
		{
			MutableScript->bIsFullyPreBaked = true;
		}
		
		UE_LOG(LogTemp, Log, TEXT("UAIFacemaskScriptManager: Pre-baking complete for script (State: %s)"), *StateName.ToString());
		
		// Broadcast pre-bake complete event
		OnScriptPreBakeComplete.Broadcast(StateName);
		
		return;
	}

	const FAIFacemaskScriptLine& ScriptLine = Script.ScriptLines[LineIndex];
	
	// Step 1: Request TTS conversion (capture ScriptLine by value for nested lambda)
	const FAIFacemaskScriptLine CapturedScriptLine = ScriptLine;
	RequestTTSConversion(CapturedScriptLine, [this, Script, LineIndex, CapturedScriptLine](const FString& AudioFilePath, float Duration)
	{
		if (AudioFilePath.IsEmpty())
		{
			UE_LOG(LogTemp, Error, TEXT("UAIFacemaskScriptManager: TTS conversion failed for line %d, skipping pre-bake"), LineIndex);
			// Continue with next line even if this one failed
			PreBakeScriptLineRecursive(Script, LineIndex + 1);
			return;
		}

		// Update script line with audio file path and duration
		FAIFacemaskScript* MutableScript = ScriptCollection.GetScriptForState(Script.AssociatedStateName);
		if (MutableScript && MutableScript->ScriptLines.IsValidIndex(LineIndex))
		{
			FAIFacemaskScriptLine& MutableLine = MutableScript->ScriptLines[LineIndex];
			MutableLine.PreBakedAudioPath = AudioFilePath;
			MutableLine.EstimatedDuration = Duration;
			MutableLine.bIsPreBaked = true;
		}

		// Step 2: Request Audio2Face conversion
		RequestAudio2FaceConversion(CapturedScriptLine, AudioFilePath, [this, Script, LineIndex](bool bSuccess)
		{
			if (!bSuccess)
			{
				UE_LOG(LogTemp, Warning, TEXT("UAIFacemaskScriptManager: Audio2Face conversion failed for line %d, but continuing"), LineIndex);
			}

			// Continue with next line
			PreBakeScriptLineRecursive(Script, LineIndex + 1);
		});
	});
}

void UAIFacemaskScriptManager::RequestTTSConversion(const FAIFacemaskScriptLine& ScriptLine, TFunction<void(const FString& AudioFilePath, float Duration)> Callback)
{
	if (!HTTPClient || ACEServerBaseURL.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("UAIFacemaskScriptManager: Cannot request TTS - HTTP client or server URL not configured"));
		if (Callback)
		{
			Callback(TEXT(""), 0.0f);
		}
		return;
	}

	// Build TTS request JSON
	TSharedPtr<FJsonObject> RequestJson = MakeShareable(new FJsonObject);
	RequestJson->SetStringField(TEXT("text"), ScriptLine.TextPrompt);
	RequestJson->SetStringField(TEXT("voice_type"), GetVoiceTypeString(ScriptLine.VoiceType));
	if (ScriptLine.VoiceType == EHoloCadeACEVoiceType::Custom && !ScriptLine.CustomVoiceModelID.IsEmpty())
	{
		RequestJson->SetStringField(TEXT("custom_voice_model_id"), ScriptLine.CustomVoiceModelID);
	}
	RequestJson->SetStringField(TEXT("script_line_id"), ScriptLine.ScriptLineID);
	RequestJson->SetBoolField(TEXT("pre_bake"), true);  // Indicate this is for pre-baking

	// Build URL
	FString TTSURL = ACEServerBaseURL;
	if (!TTSURL.EndsWith(TEXT("/")))
	{
		TTSURL += TEXT("/");
	}
	TTSURL += TEXT("api/tts/convert");

	// Send HTTP POST request
	TMap<FString, FString> Headers;
	HTTPClient->PostJSON(TTSURL, RequestJson, Headers, [this, Callback](const FAIHTTPResult& Result)
	{
		FString AudioFilePath;
		float Duration = 0.0f;
		
		if (Result.bSuccess && Result.ResponseCode == 200)
		{
			// Parse response
			TSharedPtr<FJsonObject> ResponseJson;
			if (UAIHTTPClient::ParseJSONResponse(Result.ResponseBody, ResponseJson) && ResponseJson.IsValid())
			{
				if (ResponseJson->HasField(TEXT("audio_file_path")))
				{
					AudioFilePath = ResponseJson->GetStringField(TEXT("audio_file_path"));
				}
				if (ResponseJson->HasField(TEXT("duration")))
				{
					Duration = ResponseJson->GetNumberField(TEXT("duration"));
				}
				
				UE_LOG(LogTemp, Log, TEXT("UAIFacemaskScriptManager: TTS conversion successful (Audio: %s, Duration: %.2fs)"), 
					*AudioFilePath, Duration);
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("UAIFacemaskScriptManager: TTS conversion failed (Code: %d, Error: %s)"), 
				Result.ResponseCode, *Result.ErrorMessage);
		}
		
		if (Callback)
		{
			Callback(AudioFilePath, Duration);
		}
	});
}

void UAIFacemaskScriptManager::RequestAudio2FaceConversion(const FAIFacemaskScriptLine& ScriptLine, const FString& AudioFilePath, TFunction<void(bool bSuccess)> Callback)
{
	if (!HTTPClient || ACEServerBaseURL.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("UAIFacemaskScriptManager: Cannot request Audio2Face - HTTP client or server URL not configured"));
		if (Callback)
		{
			Callback(false);
		}
		return;
	}

	// Build Audio2Face request JSON
	TSharedPtr<FJsonObject> RequestJson = MakeShareable(new FJsonObject);
	RequestJson->SetStringField(TEXT("audio_file_path"), AudioFilePath);
	RequestJson->SetStringField(TEXT("script_line_id"), ScriptLine.ScriptLineID);
	RequestJson->SetStringField(TEXT("emotion_preset"), GetEmotionPresetString(ScriptLine.EmotionPreset));
	if (ScriptLine.EmotionPreset == EHoloCadeACEEmotionPreset::Custom && ScriptLine.CustomEmotionParams.Num() > 0)
	{
		TSharedPtr<FJsonObject> EmotionParamsJson = MakeShareable(new FJsonObject);
		for (const auto& ParamPair : ScriptLine.CustomEmotionParams)
		{
			EmotionParamsJson->SetNumberField(ParamPair.Key, ParamPair.Value);
		}
		RequestJson->SetObjectField(TEXT("custom_emotion_params"), EmotionParamsJson);
	}
	RequestJson->SetBoolField(TEXT("pre_bake"), true);  // Indicate this is for pre-baking

	// Build URL
	FString Audio2FaceURL = ACEServerBaseURL;
	if (!Audio2FaceURL.EndsWith(TEXT("/")))
	{
		Audio2FaceURL += TEXT("/");
	}
	Audio2FaceURL += TEXT("api/audio2face/convert");

	// Send HTTP POST request
	TMap<FString, FString> Headers;
	HTTPClient->PostJSON(Audio2FaceURL, RequestJson, Headers, [this, Callback](const FAIHTTPResult& Result)
	{
		bool bSuccess = false;
		
		if (Result.bSuccess && Result.ResponseCode == 200)
		{
			// Parse response
			TSharedPtr<FJsonObject> ResponseJson;
			if (UAIHTTPClient::ParseJSONResponse(Result.ResponseBody, ResponseJson) && ResponseJson.IsValid())
			{
				if (ResponseJson->HasField(TEXT("status")))
				{
					FString Status = ResponseJson->GetStringField(TEXT("status"));
					bSuccess = (Status == TEXT("success") || Status == TEXT("completed"));
				}
				
				if (bSuccess)
				{
					UE_LOG(LogTemp, Log, TEXT("UAIFacemaskScriptManager: Audio2Face conversion successful"));
				}
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("UAIFacemaskScriptManager: Audio2Face conversion failed (Code: %d, Error: %s)"), 
				Result.ResponseCode, *Result.ErrorMessage);
		}
		
		if (Callback)
		{
			Callback(bSuccess);
		}
	});
}

FString UAIFacemaskScriptManager::GetVoiceTypeString(EHoloCadeACEVoiceType VoiceType) const
{
	switch (VoiceType)
	{
		case EHoloCadeACEVoiceType::Default:
			return TEXT("default");
		case EHoloCadeACEVoiceType::Male:
			return TEXT("male");
		case EHoloCadeACEVoiceType::Female:
			return TEXT("female");
		case EHoloCadeACEVoiceType::Custom:
			return TEXT("custom");
		default:
			return TEXT("default");
	}
}

FString UAIFacemaskScriptManager::GetEmotionPresetString(EHoloCadeACEEmotionPreset EmotionPreset) const
{
	switch (EmotionPreset)
	{
		case EHoloCadeACEEmotionPreset::Neutral:
			return TEXT("neutral");
		case EHoloCadeACEEmotionPreset::Happy:
			return TEXT("happy");
		case EHoloCadeACEEmotionPreset::Sad:
			return TEXT("sad");
		case EHoloCadeACEEmotionPreset::Angry:
			return TEXT("angry");
		case EHoloCadeACEEmotionPreset::Surprised:
			return TEXT("surprised");
		case EHoloCadeACEEmotionPreset::Fearful:
			return TEXT("fearful");
		case EHoloCadeACEEmotionPreset::Disgusted:
			return TEXT("disgusted");
		case EHoloCadeACEEmotionPreset::Custom:
			return TEXT("custom");
		default:
			return TEXT("neutral");
	}
}
