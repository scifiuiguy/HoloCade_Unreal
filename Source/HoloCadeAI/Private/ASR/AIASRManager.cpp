// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "ASR/AIASRManager.h"
#include "AIGRPCClient.h"
#include "ASRProviderManager.h"
#include "ContainerManagerDockerCLI.h"

UAIASRManager::UAIASRManager()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	
	bIsInitialized = false;
	VoiceActivityTimer = 0.0f;
	SilenceThreshold = 1.0f;
}

void UAIASRManager::BeginPlay()
{
	Super::BeginPlay();

	if (!GRPCClient)
	{
		GRPCClient = NewObject<UAIGRPCClient>(this, TEXT("GRPCClient"));
	}
	if (!ASRProviderManager)
	{
		ASRProviderManager = NewObject<UASRProviderManager>(this, TEXT("ASRProviderManager"));
	}
}

void UAIASRManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsInitialized)
	{
		return;
	}

	// Voice activity detection - check for silence
	VoiceActivityTimer += DeltaTime;
	
	if (VoiceActivityTimer >= SilenceThreshold)
	{
		// Check all sources for silence
		for (auto& SourcePair : SourceSpeakingStates)
		{
			int32 SourceId = SourcePair.Key;
			bool bWasSpeaking = SourcePair.Value;
			
			if (bWasSpeaking && !SourceTranscribingStates.Contains(SourceId))
			{
				// Source was speaking but now silent - trigger transcription
				TriggerTranscriptionForSource(SourceId);
			}
		}
		
		VoiceActivityTimer = 0.0f;
	}
}

bool UAIASRManager::InitializeASRManager()
{
	if (bIsInitialized)
	{
		UE_LOG(LogTemp, Warning, TEXT("AIASRManager: Already initialized"));
		return true;
	}

	if (!ASRConfig.bEnableASR)
	{
		UE_LOG(LogTemp, Log, TEXT("AIASRManager: ASR is disabled in config"));
		return false;
	}

	// Initialize ASR Provider Manager with container auto-start if configured
	FContainerConfig ASRContainerConfig = ASRConfig.ContainerConfig;
	if (ASRConfig.bAutoStartContainer && ASRContainerConfig.ContainerName.IsEmpty())
	{
		// Auto-generate container name from endpoint URL
		ASRContainerConfig.ContainerName = FString::Printf(TEXT("holocade-asr-%s"),
			*ASRConfig.LocalASREndpointURL.Replace(TEXT(":"), TEXT("-")).Replace(TEXT("."), TEXT("-")));

		// Extract port from endpoint URL if not set
		if (ASRContainerConfig.HostPort == 0 && ASRContainerConfig.ContainerPort == 0)
		{
			FString EndpointURL = ASRConfig.LocalASREndpointURL;
			FString PortStr;
			if (EndpointURL.Split(TEXT(":"), nullptr, &PortStr))
			{
				int32 Port = FCString::Atoi(*PortStr);
				if (Port > 0)
				{
					ASRContainerConfig.HostPort = Port;
					ASRContainerConfig.ContainerPort = 50051; // Default gRPC port for Riva/NIM ASR
				}
			}
		}

		// Set image name if not set (default to Riva ASR)
		if (ASRContainerConfig.ImageName.IsEmpty())
		{
			ASRContainerConfig.ImageName = TEXT("nvcr.io/nim/riva-asr:latest");
		}
	}

	// Initialize ASR provider with container auto-start
	if (!ASRProviderManager->Initialize(
		GRPCClient,
		ASRConfig.LocalASREndpointURL,
		EASRProviderType::AutoDetect,
		ASRContainerConfig,
		ASRConfig.bAutoStartContainer))
	{
		UE_LOG(LogTemp, Error, TEXT("AIASRManager: Failed to initialize ASR provider manager"));
		return false;
	}

	bIsInitialized = true;

	UE_LOG(LogTemp, Log, TEXT("AIASRManager: Initialized with local ASR: %s (language: %s)"),
		ASRConfig.bUseLocalASR ? *ASRConfig.LocalASREndpointURL : TEXT("Cloud"),
		*ASRConfig.LanguageCode);

	return true;
}

void UAIASRManager::ProcessAudio(int32 SourceId, const TArray<float>& AudioData, int32 SampleRate)
{
	if (!bIsInitialized)
	{
		return;
	}

	// Buffer audio for this source
	if (!SourceAudioBuffers.Contains(SourceId))
	{
		SourceAudioBuffers.Add(SourceId, TArray<float>());
		SourceAudioStartTimes.Add(SourceId, 0.0f);
		SourceSpeakingStates.Add(SourceId, false);
	}

	TArray<float>& Buffer = SourceAudioBuffers[SourceId];
	Buffer.Append(AudioData);

	// Update speaking state based on voice activity detection
	bool bIsSpeaking = DetectVoiceActivity(AudioData);
	SourceSpeakingStates[SourceId] = bIsSpeaking;

	// Update start time if just started speaking
	if (bIsSpeaking && SourceAudioStartTimes[SourceId] == 0.0f)
	{
		SourceAudioStartTimes[SourceId] = GetWorld()->GetTimeSeconds();
	}
}

void UAIASRManager::OnPlayerAudioReceived(int32 PlayerId, const TArray<float>& AudioData, int32 SampleRate, const FVector& Position)
{
	// Generic implementation - treat player as source
	ProcessAudio(PlayerId, AudioData, SampleRate);
}

void UAIASRManager::TriggerTranscriptionForSource(int32 SourceId)
{
	if (!bIsInitialized)
	{
		return;
	}

	if (SourceTranscribingStates.Contains(SourceId) && SourceTranscribingStates[SourceId])
	{
		return; // Already transcribing
	}

	if (!SourceAudioBuffers.Contains(SourceId) || SourceAudioBuffers[SourceId].Num() == 0)
	{
		return; // No audio to transcribe
	}

	// Check audio duration
	float AudioDuration = GetWorld()->GetTimeSeconds() - SourceAudioStartTimes[SourceId];
	if (AudioDuration < ASRConfig.MinAudioDuration)
	{
		ClearSourceAudioBuffer(SourceId);
		return; // Too short
	}

	if (AudioDuration > ASRConfig.MaxAudioDuration)
	{
		// Truncate to max duration
		int32 MaxSamples = static_cast<int32>(ASRConfig.MaxAudioDuration * 48000.0f); // Assume 48kHz
		TArray<float>& Buffer = SourceAudioBuffers[SourceId];
		if (Buffer.Num() > MaxSamples)
		{
			Buffer.SetNum(MaxSamples);
		}
	}

	// Mark as transcribing
	SourceTranscribingStates.Add(SourceId, true);
	OnTranscriptionStarted.Broadcast(SourceId);

	// Request transcription
	TArray<float> AudioData = SourceAudioBuffers[SourceId];
	RequestASRTranscription(SourceId, AudioData, 48000); // Assume 48kHz from Mumble

	// Clear buffer
	ClearSourceAudioBuffer(SourceId);
}

bool UAIASRManager::IsSourceBeingTranscribed(int32 SourceId) const
{
	return SourceTranscribingStates.Contains(SourceId) && SourceTranscribingStates[SourceId];
}

void UAIASRManager::RequestASRTranscription(int32 SourceId, const TArray<float>& AudioData, int32 SampleRate)
{
	// Generic implementation - uses ASR provider manager
	if (!ASRProviderManager)
	{
		UE_LOG(LogTemp, Error, TEXT("AIASRManager: ASR provider manager not initialized"));
		HandleTranscriptionResult(SourceId, TEXT(""));
		return;
	}

	// Convert PCM float to bytes
	TArray<uint8> AudioBytes = ConvertPCMFloatToBytes(AudioData, SampleRate);

	// Create ASR request
	FASRRequest Request;
	Request.AudioData = AudioBytes;
	Request.SampleRate = SampleRate;
	Request.LanguageCode = ASRConfig.LanguageCode;
	Request.bUseStreaming = true;  // Use streaming for real-time ASR

	// Request transcription via provider manager
	ASRProviderManager->RequestTranscription(Request, [this, SourceId](const FASRResponse& Response)
	{
		if (Response.bSuccess)
		{
			HandleTranscriptionResult(SourceId, Response.TranscribedText);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("AIASRManager: Transcription failed for source %d: %s"), SourceId, *Response.ErrorMessage);
			HandleTranscriptionResult(SourceId, TEXT(""));
		}
	});
}

void UAIASRManager::HandleTranscriptionResult(int32 SourceId, const FString& TranscribedText)
{
	// Generic implementation - just broadcasts event
	// Subclasses can override for experience-specific handling (e.g., trigger improv)
	
	SourceTranscribingStates.Add(SourceId, false);
	OnTranscriptionComplete.Broadcast(SourceId, TranscribedText);
	
	UE_LOG(LogTemp, Log, TEXT("AIASRManager: Transcription complete for source %d: %s"), SourceId, *TranscribedText);
}

bool UAIASRManager::DetectVoiceActivity(const TArray<float>& AudioData) const
{
	// Simple energy-based VAD
	if (AudioData.Num() == 0)
	{
		return false;
	}

	float Energy = 0.0f;
	for (float Sample : AudioData)
	{
		Energy += FMath::Abs(Sample);
	}
	Energy /= AudioData.Num();

	// Threshold for voice activity (can be tuned)
	return Energy > 0.01f;
}

void UAIASRManager::ClearSourceAudioBuffer(int32 SourceId)
{
	SourceAudioBuffers.Remove(SourceId);
	SourceAudioStartTimes.Remove(SourceId);
	SourceSpeakingStates.Remove(SourceId);
}

TArray<uint8> UAIASRManager::ConvertPCMFloatToBytes(const TArray<float>& FloatAudio, int32 SampleRate) const
{
	TArray<uint8> Bytes;
	Bytes.Reserve(FloatAudio.Num() * 2); // 16-bit = 2 bytes per sample

	for (float Sample : FloatAudio)
	{
		// Clamp to [-1.0, 1.0] range
		Sample = FMath::Clamp(Sample, -1.0f, 1.0f);
		
		// Convert to 16-bit PCM (little-endian)
		int16 IntSample = static_cast<int16>(Sample * 32767.0f);
		
		// Add bytes (little-endian)
		Bytes.Add(static_cast<uint8>(IntSample & 0xFF));
		Bytes.Add(static_cast<uint8>((IntSample >> 8) & 0xFF));
	}

	return Bytes;
}

