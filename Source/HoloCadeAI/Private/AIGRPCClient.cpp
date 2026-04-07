// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "AIGRPCClient.h"

#if WITH_TURBOLINK
#include "TurboLinkGrpcManager.h"
#include "TurboLinkGrpcService.h"
#include "TurboLinkGrpcClient.h"
#endif // WITH_TURBOLINK

UAIGRPCClient::UAIGRPCClient()
{
	bIsInitialized = false;
	ServerAddress = TEXT("");
	
#if WITH_TURBOLINK
	GrpcManager = nullptr;
	ASRClient = nullptr;
	TTSClient = nullptr;
#endif // WITH_TURBOLINK
}

UAIGRPCClient::~UAIGRPCClient()
{
}

bool UAIGRPCClient::Initialize(const FString& InServerAddress)
{
	if (InServerAddress.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("AIGRPCClient: Cannot initialize - server address is empty"));
		return false;
	}

	ServerAddress = InServerAddress;

#if WITH_TURBOLINK
	// TODO: Initialize TurboLink gRPC manager and clients
	// Example structure (actual API may differ):
	// 
	// 1. Get TurboLink manager: GrpcManager = GetWorld()->GetSubsystem<UTurboLinkGrpcManager>();
	// 2. Create ASR service client: ASRClient = GrpcManager->CreateClient(...);
	// 3. Create TTS service client: TTSClient = GrpcManager->CreateClient(...);
	// 4. Connect to server: ASRClient->Connect(ServerAddress); TTSClient->Connect(ServerAddress);
	//
	// Note: Actual TurboLink API structure needs to be verified after installation.
	// Check TurboLink documentation/examples for correct initialization pattern.
	
	UE_LOG(LogTemp, Warning, TEXT("AIGRPCClient: TurboLink detected but initialization not yet implemented"));
	UE_LOG(LogTemp, Warning, TEXT("                        Please implement TurboLink initialization in AIGRPCClient.cpp"));
	bIsInitialized = false;  // Set to false until TurboLink is properly initialized
#else
	UE_LOG(LogTemp, Warning, TEXT("AIGRPCClient: TurboLink not available - using NOOP implementation"));
	UE_LOG(LogTemp, Warning, TEXT("                        Install TurboLink for gRPC functionality:"));
	UE_LOG(LogTemp, Warning, TEXT("                        Run: .\\Source\\AIFacemask\\Common\\SetupTurboLink.ps1"));
	bIsInitialized = true;  // Allow NOOP mode for development
#endif // WITH_TURBOLINK

	UE_LOG(LogTemp, Log, TEXT("AIGRPCClient: Initialized with server address: %s"), *ServerAddress);
	
	return bIsInitialized;
}

void UAIGRPCClient::RequestASRTranscription(const FAIASRRequest& Request, TFunction<void(const FAIASRResponse&)> Callback)
{
	if (!bIsInitialized)
	{
		UE_LOG(LogTemp, Error, TEXT("AIGRPCClient: Cannot request ASR - not initialized"));
		FAIASRResponse ErrorResponse;
		if (Callback)
		{
			Callback(ErrorResponse);
		}
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("AIGRPCClient: Requesting ASR transcription (audio samples: %d, sample rate: %d, language: %s)"), 
		Request.AudioData.Num(), Request.SampleRate, *Request.LanguageCode);

	// NOOP: TODO - Execute actual gRPC call to NVIDIA Riva ASR service
	// Service: nvidia.riva.asr.RivaSpeechRecognition
	// Method: Recognize
	// Input: audio_data (bytes), sample_rate (int32), language_code (string)
	// Output: transcript (string), confidence (float), is_final (bool)
	//
	// Implementation options:
	// 1. Use UnrealGrpc plugin if available
	// 2. Use HTTP REST if NVIDIA Riva exposes REST endpoints
	// 3. Call external gRPC tool (grpcurl) via command line
	// 4. Integrate gRPC C++ library directly
	
	ExecuteASRCall(Request, Callback);
}

void UAIGRPCClient::RequestTTSSynthesis(const FAITTSRequest& Request, TFunction<void(const FAITTSResponse&)> Callback)
{
	if (!bIsInitialized)
	{
		UE_LOG(LogTemp, Error, TEXT("AIGRPCClient: Cannot request TTS - not initialized"));
		FAITTSResponse ErrorResponse;
		if (Callback)
		{
			Callback(ErrorResponse);
		}
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("AIGRPCClient: Requesting TTS synthesis (text length: %d, voice: %s, sample rate: %d)"), 
		Request.Text.Len(), *Request.VoiceName, Request.SampleRate);

	// NOOP: TODO - Execute actual gRPC call to NVIDIA Riva TTS service
	// Service: nvidia.riva.tts.RivaSpeechSynthesis
	// Method: Synthesize
	// Input: text (string), voice_name (string), sample_rate (int32), language_code (string)
	// Output: audio_data (bytes), sample_rate (int32), audio_format (string)
	//
	// Implementation options:
	// 1. Use UnrealGrpc plugin if available
	// 2. Use HTTP REST if NVIDIA Riva exposes REST endpoints
	// 3. Call external gRPC tool (grpcurl) via command line
	// 4. Integrate gRPC C++ library directly
	
	ExecuteTTSCall(Request, Callback);
}

void UAIGRPCClient::ExecuteASRCall(const FAIASRRequest& Request, TFunction<void(const FAIASRResponse&)> Callback)
{
#if WITH_TURBOLINK
	// TODO: Implement TurboLink gRPC call for ASR
	// Example structure (actual API may differ):
	//
	// 1. Create request message using TurboLink's generated protobuf classes
	// 2. Set request fields:
	//    - audio_data: Request.AudioData
	//    - sample_rate: Request.SampleRate
	//    - language_code: Request.LanguageCode
	// 3. Call ASR service via TurboLink client: ASRClient->CallMethod(...)
	// 4. Handle async response in callback
	// 5. Parse response and convert to FAIASRResponse
	// 6. Call user callback with result
	//
	// Note: Actual TurboLink API structure needs to be verified after installation.
	// Check TurboLink documentation/examples for correct call pattern.
	// You'll need to generate protobuf classes from NVIDIA Riva .proto files.
	
	UE_LOG(LogTemp, Warning, TEXT("AIGRPCClient: TurboLink ASR call not yet implemented"));
	
	FAIASRResponse Response;
	Response.TranscribedText = TEXT("[TurboLink: ASR implementation pending]");
	Response.Confidence = 0.0f;
	Response.bIsFinal = true;

	if (Callback)
	{
		Callback(Response);
	}
#else
	// NOOP: TurboLink not available
	FAIASRResponse Response;
	Response.TranscribedText = TEXT("[NOOP: TurboLink not installed - gRPC ASR unavailable]");
	Response.Confidence = 0.0f;
	Response.bIsFinal = true;

	if (Callback)
	{
		Callback(Response);
	}
#endif // WITH_TURBOLINK
}

void UAIGRPCClient::ExecuteTTSCall(const FAITTSRequest& Request, TFunction<void(const FAITTSResponse&)> Callback)
{
#if WITH_TURBOLINK
	// TODO: Implement TurboLink gRPC call for TTS
	// Example structure (actual API may differ):
	//
	// 1. Create request message using TurboLink's generated protobuf classes
	// 2. Set request fields:
	//    - text: Request.Text
	//    - voice_name: Request.VoiceName
	//    - sample_rate: Request.SampleRate
	//    - language_code: Request.LanguageCode
	// 3. Call TTS service via TurboLink client: TTSClient->CallMethod(...)
	// 4. Handle async response in callback
	// 5. Parse response and convert to FAITTSResponse
	// 6. Call user callback with result
	//
	// Note: Actual TurboLink API structure needs to be verified after installation.
	// Check TurboLink documentation/examples for correct call pattern.
	// You'll need to generate protobuf classes from NVIDIA Riva .proto files.
	
	UE_LOG(LogTemp, Warning, TEXT("AIGRPCClient: TurboLink TTS call not yet implemented"));
	
	FAITTSResponse Response;
	Response.AudioData.Empty();
	Response.SampleRate = Request.SampleRate;
	Response.AudioFormat = TEXT("pcm");

	if (Callback)
	{
		Callback(Response);
	}
#else
	// NOOP: TurboLink not available
	FAITTSResponse Response;
	Response.AudioData.Empty();
	Response.SampleRate = Request.SampleRate;
	Response.AudioFormat = TEXT("pcm");

	if (Callback)
	{
		Callback(Response);
	}
#endif // WITH_TURBOLINK
}

