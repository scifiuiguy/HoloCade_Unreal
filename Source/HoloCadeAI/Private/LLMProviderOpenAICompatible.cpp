#include "LLMProviderOpenAICompatible.h"
#include "AIHTTPClient.h"

ULLMProviderOpenAICompatible::ULLMProviderOpenAICompatible()
{
	HTTPClient = nullptr;
}

void ULLMProviderOpenAICompatible::Initialize(const FString& InEndpointURL, const FString& InAPIKey)
{
	EndpointURL = InEndpointURL;
	APIKey = InAPIKey;
	bIsInitialized = !EndpointURL.IsEmpty();

	if (!HTTPClient)
	{
		HTTPClient = NewObject<UAIHTTPClient>(this);
	}
}

void ULLMProviderOpenAICompatible::RequestResponse(const FLLMRequest& Request, TFunction<void(const FLLMResponse&)> Callback)
{
	if (!Callback)
	{
		return;
	}

	if (!bIsInitialized || !HTTPClient)
	{
		FLLMResponse Response;
		Response.bSuccess = false;
		Response.ErrorMessage = TEXT("OpenAI-compatible provider not initialized");
		Callback(Response);
		return;
	}

	// NOOP: This functionality will be implemented in a future update
	FLLMResponse Response;
	Response.bSuccess = false;
	Response.ErrorMessage = TEXT("OpenAI-compatible provider request handling not yet implemented");
	Callback(Response);
}

bool ULLMProviderOpenAICompatible::IsAvailable() const
{
	return bIsInitialized;
}

TArray<FString> ULLMProviderOpenAICompatible::GetSupportedModels() const
{
	return TArray<FString>();
}








