#include "ASRProviderNIM.h"
#include "AIGRPCClient.h"

bool UASRProviderNIM::Initialize_Implementation(UAIGRPCClient* InGRPCClient, const FString& InEndpointURL)
{
	GRPCClient = InGRPCClient;
	EndpointURL = InEndpointURL;
	ModelType = DetectModelType(EndpointURL);

	bIsInitialized = (GRPCClient != nullptr) && !EndpointURL.IsEmpty();

	if (!bIsInitialized)
	{
		UE_LOG(LogTemp, Warning, TEXT("UASRProviderNIM: Failed to initialize - missing gRPC client or endpoint URL"));
	}

	return bIsInitialized;
}

void UASRProviderNIM::RequestASRTranscription_Implementation(const FASRRequest& Request, const FOnASRResponseReceived& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}

	if (!bIsInitialized)
	{
		FASRResponse ErrorResponse;
		ErrorResponse.bSuccess = false;
		ErrorResponse.ErrorMessage = TEXT("NIM provider not initialized");
		Callback.ExecuteIfBound(ErrorResponse);
		return;
	}

	// NOOP: This functionality will be implemented in a future update
	FASRResponse Response;
	Response.bSuccess = false;
	Response.ErrorMessage = TEXT("NIM ASR provider not yet implemented");
	Callback.ExecuteIfBound(Response);
}

bool UASRProviderNIM::IsAvailable_Implementation() const
{
	return bIsInitialized;
}

FString UASRProviderNIM::GetProviderName_Implementation() const
{
	return TEXT("NVIDIA NIM ASR");
}

TArray<FString> UASRProviderNIM::GetSupportedModels_Implementation() const
{
	TArray<FString> Models;
	Models.Add(TEXT("Parakeet 0.6B EN"));
	Models.Add(TEXT("Parakeet 1.1B Multilingual"));
	Models.Add(TEXT("Canary 1B Multilingual"));
	Models.Add(TEXT("Whisper Large"));
	return Models;
}

bool UASRProviderNIM::SupportsStreaming_Implementation() const
{
	return ModelType != ENIMASRModelType::Whisper_Large &&
		ModelType != ENIMASRModelType::Whisper_Medium &&
		ModelType != ENIMASRModelType::Whisper_Small;
}

ENIMASRModelType UASRProviderNIM::DetectModelType(const FString& InEndpointURL) const
{
	if (InEndpointURL.Contains(TEXT("parakeet"), ESearchCase::IgnoreCase))
	{
		return ENIMASRModelType::Parakeet_1_1B_Multilingual;
	}

	if (InEndpointURL.Contains(TEXT("canary"), ESearchCase::IgnoreCase))
	{
		return ENIMASRModelType::Canary_1B_Multilingual;
	}

	if (InEndpointURL.Contains(TEXT("whisper"), ESearchCase::IgnoreCase))
	{
		return ENIMASRModelType::Whisper_Large;
	}

	return ENIMASRModelType::AutoDetect;
}








