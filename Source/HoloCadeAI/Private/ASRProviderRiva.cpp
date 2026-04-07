#include "ASRProviderRiva.h"
#include "AIGRPCClient.h"

bool UASRProviderRiva::Initialize_Implementation(UAIGRPCClient* InGRPCClient, const FString& InEndpointURL)
{
	GRPCClient = InGRPCClient;
	EndpointURL = InEndpointURL;

	bIsInitialized = (GRPCClient != nullptr) && !EndpointURL.IsEmpty();

	if (!bIsInitialized)
	{
		UE_LOG(LogTemp, Warning, TEXT("UASRProviderRiva: Failed to initialize - missing gRPC client or endpoint URL"));
	}

	return bIsInitialized;
}

void UASRProviderRiva::RequestASRTranscription_Implementation(const FASRRequest& Request, const FOnASRResponseReceived& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}

	if (!bIsInitialized)
	{
		FASRResponse ErrorResponse;
		ErrorResponse.bSuccess = false;
		ErrorResponse.ErrorMessage = TEXT("Riva provider not initialized");
		Callback.ExecuteIfBound(ErrorResponse);
		return;
	}

	// NOOP: This functionality will be implemented in a future update
	FASRResponse Response;
	Response.bSuccess = false;
	Response.ErrorMessage = TEXT("Riva ASR provider not yet implemented");
	Callback.ExecuteIfBound(Response);
}

bool UASRProviderRiva::IsAvailable_Implementation() const
{
	return bIsInitialized;
}

FString UASRProviderRiva::GetProviderName_Implementation() const
{
	return TEXT("NVIDIA Riva ASR");
}

TArray<FString> UASRProviderRiva::GetSupportedModels_Implementation() const
{
	TArray<FString> Models;
	Models.Add(TEXT("Riva Conformer-Transducer EN-US"));
	Models.Add(TEXT("Riva Citrinet EN-US"));
	return Models;
}

bool UASRProviderRiva::SupportsStreaming_Implementation() const
{
	return true;
}

