#include "LLMProviderOllama.h"
#include "AIHTTPClient.h"

ULLMProviderOllama::ULLMProviderOllama()
{
	HTTPClient = nullptr;
}

void ULLMProviderOllama::Initialize(const FString& InEndpointURL)
{
	EndpointURL = InEndpointURL;
	bIsInitialized = !EndpointURL.IsEmpty();

	if (!HTTPClient)
	{
		HTTPClient = NewObject<UAIHTTPClient>(this);
	}
}

void ULLMProviderOllama::RequestResponse(const FLLMRequest& Request, TFunction<void(const FLLMResponse&)> Callback)
{
	if (!Callback)
	{
		return;
	}

	if (!bIsInitialized || !HTTPClient)
	{
		FLLMResponse Response;
		Response.bSuccess = false;
		Response.ErrorMessage = TEXT("Ollama provider not initialized");
		Callback(Response);
		return;
	}

	// NOOP: This functionality will be implemented in a future update
	FLLMResponse Response;
	Response.bSuccess = false;
	Response.ErrorMessage = TEXT("Ollama provider request handling not yet implemented");
	Callback(Response);
}

bool ULLMProviderOllama::IsAvailable() const
{
	return bIsInitialized;
}

TArray<FString> ULLMProviderOllama::GetSupportedModels() const
{
	return TArray<FString>();
}








