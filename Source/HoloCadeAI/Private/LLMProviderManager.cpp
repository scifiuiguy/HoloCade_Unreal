// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "LLMProviderManager.h"
#include "LLMProviderOllama.h"
#include "LLMProviderOpenAICompatible.h"
#include "ContainerManagerDockerCLI.h"

ULLMProviderManager::ULLMProviderManager()
{
	CurrentProvider = nullptr;
	OllamaProvider = nullptr;
	OpenAIProvider = nullptr;
	ContainerManager = nullptr;
}

bool ULLMProviderManager::InitializeProvider(const FString& EndpointURL, ELLMProviderType ProviderType, const FString& ModelName, const FContainerConfig& ContainerConfig, bool bAutoStartContainer)
{
	// Auto-start container if requested
	if (bAutoStartContainer && !ContainerConfig.ImageName.IsEmpty())
	{
		// Create container manager if not already created
		if (!ContainerManager)
		{
			ContainerManager = NewObject<UContainerManagerDockerCLI>(this);
		}

		// Check if Docker is available
		if (!ContainerManager->IsDockerAvailable())
		{
			UE_LOG(LogTemp, Error, TEXT("LLMProviderManager: Docker is not available: %s"), 
				*ContainerManager->GetLastError());
			// Continue anyway - container might already be running externally
		}
		else
		{
			// Check if container is already running
			bool bIsRunning = false;
			bool bExists = false;
			if (ContainerManager->GetContainerStatus(ContainerConfig.ContainerName, bIsRunning, bExists))
			{
				if (!bIsRunning)
				{
					// Start container if not running
					UE_LOG(LogTemp, Log, TEXT("LLMProviderManager: Starting container '%s'..."), 
						*ContainerConfig.ContainerName);
					
					if (!ContainerManager->StartContainer(ContainerConfig))
					{
						UE_LOG(LogTemp, Error, TEXT("LLMProviderManager: Failed to start container '%s': %s"), 
							*ContainerConfig.ContainerName, *ContainerManager->GetLastError());
						// Continue anyway - container might start later or be managed externally
					}
					else
					{
						UE_LOG(LogTemp, Log, TEXT("LLMProviderManager: Container '%s' started successfully"), 
							*ContainerConfig.ContainerName);
					}
				}
				else
				{
					UE_LOG(LogTemp, Log, TEXT("LLMProviderManager: Container '%s' is already running"), 
						*ContainerConfig.ContainerName);
				}
			}
			else
			{
				// Container doesn't exist, create and start it
				UE_LOG(LogTemp, Log, TEXT("LLMProviderManager: Creating and starting container '%s'..."), 
					*ContainerConfig.ContainerName);
				
				if (!ContainerManager->StartContainer(ContainerConfig))
				{
					UE_LOG(LogTemp, Error, TEXT("LLMProviderManager: Failed to start container '%s': %s"), 
						*ContainerConfig.ContainerName, *ContainerManager->GetLastError());
					// Continue anyway - container might start later or be managed externally
				}
				else
				{
					UE_LOG(LogTemp, Log, TEXT("LLMProviderManager: Container '%s' started successfully"), 
						*ContainerConfig.ContainerName);
				}
			}
		}
	}

	// Determine provider type
	ELLMProviderType ActualProviderType = ProviderType;
	if (ActualProviderType == ELLMProviderType::AutoDetect)
	{
		ActualProviderType = DetectProviderType(EndpointURL);
	}

	// Create provider instance
	CurrentProvider = CreateProvider(ActualProviderType, EndpointURL, ModelName);
	if (!CurrentProvider)
	{
		UE_LOG(LogTemp, Error, TEXT("LLMProviderManager: Failed to create provider for type %d"), 
			(int32)ActualProviderType);
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("LLMProviderManager: Initialized provider '%s' with endpoint '%s'"), 
		*GetCurrentProviderName(), *EndpointURL);
	
	return true;
}

bool ULLMProviderManager::SetProviderEndpoint(const FString& EndpointURL, ELLMProviderType ProviderType, const FString& ModelName)
{
	// For hot-swapping, we don't auto-start containers (assume they're already running)
	return InitializeProvider(EndpointURL, ProviderType, ModelName, FContainerConfig(), false);
}

void ULLMProviderManager::RequestResponse(const FLLMRequest& Request, TFunction<void(const FLLMResponse&)> Callback)
{
	if (!CurrentProvider)
	{
		FLLMResponse ErrorResponse;
		ErrorResponse.bSuccess = false;
		ErrorResponse.ErrorMessage = TEXT("No provider is currently active");
		Callback(ErrorResponse);
		return;
	}

	// Forward request to current provider
	CurrentProvider->RequestResponse(Request, Callback);
}

FString ULLMProviderManager::GetCurrentProviderName() const
{
	if (!CurrentProvider)
	{
		return TEXT("None");
	}

	return CurrentProvider->GetProviderName();
}

TArray<FString> ULLMProviderManager::GetSupportedModels() const
{
	if (!CurrentProvider)
	{
		return TArray<FString>();
	}

	return CurrentProvider->GetSupportedModels();
}

bool ULLMProviderManager::IsProviderAvailable() const
{
	if (!CurrentProvider)
	{
		return false;
	}

	return CurrentProvider->IsAvailable();
}

void ULLMProviderManager::RegisterCustomProvider(TScriptInterface<ILLMProvider> Provider)
{
	if (!Provider.GetInterface())
	{
		UE_LOG(LogTemp, Warning, TEXT("LLMProviderManager: Attempted to register null custom provider"));
		return;
	}

	CustomProvider = Provider;
	UE_LOG(LogTemp, Log, TEXT("LLMProviderManager: Registered custom provider '%s'"), 
		*Provider->GetProviderName());
}

ELLMProviderType ULLMProviderManager::DetectProviderType(const FString& EndpointURL) const
{
	// Auto-detect based on endpoint URL
	if (EndpointURL.Contains(TEXT("11434")) || EndpointURL.Contains(TEXT("ollama")))
	{
		return ELLMProviderType::Ollama;
	}
	
	// Default to OpenAI-compatible (NIM, vLLM, OpenAI, Claude, etc.)
	return ELLMProviderType::OpenAICompatible;
}

ILLMProvider* ULLMProviderManager::CreateProvider(ELLMProviderType ProviderType, const FString& EndpointURL, const FString& ModelName)
{
	switch (ProviderType)
	{
	case ELLMProviderType::Ollama:
		if (!OllamaProvider)
		{
			OllamaProvider = NewObject<ULLMProviderOllama>(this);
		}
		// TODO: Initialize Ollama provider with HTTP client
		return OllamaProvider;

	case ELLMProviderType::OpenAICompatible:
		if (!OpenAIProvider)
		{
			OpenAIProvider = NewObject<ULLMProviderOpenAICompatible>(this);
		}
		// TODO: Initialize OpenAI-compatible provider with HTTP client
		return OpenAIProvider;

	case ELLMProviderType::Custom:
		return CustomProvider.GetInterface();

	default:
		UE_LOG(LogTemp, Error, TEXT("LLMProviderManager: Unknown provider type %d"), (int32)ProviderType);
		return nullptr;
	}
}

