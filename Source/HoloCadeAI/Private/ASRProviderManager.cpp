// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "ASRProviderManager.h"
#include "ASRProviderRiva.h"
#include "ASRProviderNIM.h"
#include "ContainerManagerDockerCLI.h"

void UASRTranscriptionCallbackProxy::Initialize(UASRProviderManager* InOwner, TFunction<void(const FASRResponse&)> InCallback)
{
	Owner = InOwner;
	NativeCallback = MoveTemp(InCallback);
}

void UASRTranscriptionCallbackProxy::HandleResponse(const FASRResponse& Response)
{
	if (NativeCallback)
	{
		NativeCallback(Response);
	}

	if (Owner.IsValid())
	{
		Owner->HandleCallbackProxyFinished(this);
	}
}

UASRProviderManager::UASRProviderManager()
{
	ActiveProvider = nullptr;
	GRPCClientRef = nullptr;
	ContainerManager = nullptr;
}

bool UASRProviderManager::Initialize(UAIGRPCClient* InGRPCClient, const FString& InDefaultEndpointURL, EASRProviderType InDefaultProviderType, const FContainerConfig& ContainerConfig, bool bAutoStartContainer)
{
	if (!InGRPCClient)
	{
		UE_LOG(LogTemp, Error, TEXT("ASRProviderManager: gRPC client is null"));
		return false;
	}

	GRPCClientRef = InGRPCClient;

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
			UE_LOG(LogTemp, Error, TEXT("ASRProviderManager: Docker is not available: %s"), 
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
					UE_LOG(LogTemp, Log, TEXT("ASRProviderManager: Starting container '%s'..."), 
						*ContainerConfig.ContainerName);
					
					if (!ContainerManager->StartContainer(ContainerConfig))
					{
						UE_LOG(LogTemp, Error, TEXT("ASRProviderManager: Failed to start container '%s': %s"), 
							*ContainerConfig.ContainerName, *ContainerManager->GetLastError());
						// Continue anyway - container might start later or be managed externally
					}
					else
					{
						UE_LOG(LogTemp, Log, TEXT("ASRProviderManager: Container '%s' started successfully"), 
							*ContainerConfig.ContainerName);
					}
				}
				else
				{
					UE_LOG(LogTemp, Log, TEXT("ASRProviderManager: Container '%s' is already running"), 
						*ContainerConfig.ContainerName);
				}
			}
			else
			{
				// Container doesn't exist, create and start it
				UE_LOG(LogTemp, Log, TEXT("ASRProviderManager: Creating and starting container '%s'..."), 
					*ContainerConfig.ContainerName);
				
				if (!ContainerManager->StartContainer(ContainerConfig))
				{
					UE_LOG(LogTemp, Error, TEXT("ASRProviderManager: Failed to start container '%s': %s"), 
						*ContainerConfig.ContainerName, *ContainerManager->GetLastError());
					// Continue anyway - container might start later or be managed externally
				}
				else
				{
					UE_LOG(LogTemp, Log, TEXT("ASRProviderManager: Container '%s' started successfully"), 
						*ContainerConfig.ContainerName);
				}
			}
		}
	}

	// Determine provider type
	EASRProviderType ActualProviderType = InDefaultProviderType;
	if (ActualProviderType == EASRProviderType::AutoDetect)
	{
		ActualProviderType = AutoDetectProviderType(InDefaultEndpointURL);
	}

	// Create provider instance based on type
	TScriptInterface<IASRProvider> Provider;
	
	switch (ActualProviderType)
	{
	case EASRProviderType::Riva:
	{
		UASRProviderRiva* RivaProvider = NewObject<UASRProviderRiva>(this);
		if (RivaProvider->Initialize(InGRPCClient, InDefaultEndpointURL))
		{
			Provider = RivaProvider;
			DefaultProviders.Add(EASRProviderType::Riva, Provider);
		}
		break;
	}
	case EASRProviderType::NIM:
	{
		UASRProviderNIM* NIMProvider = NewObject<UASRProviderNIM>(this);
		if (NIMProvider->Initialize(InGRPCClient, InDefaultEndpointURL))
		{
			Provider = NIMProvider;
			DefaultProviders.Add(EASRProviderType::NIM, Provider);
		}
		break;
	}
	default:
		UE_LOG(LogTemp, Error, TEXT("ASRProviderManager: Unknown provider type %d"), (int32)ActualProviderType);
		return false;
	}

	if (!Provider.GetInterface())
	{
		UE_LOG(LogTemp, Error, TEXT("ASRProviderManager: Failed to create provider for type %d"), 
			(int32)ActualProviderType);
		return false;
	}

	ActiveProvider = Provider;

	UE_LOG(LogTemp, Log, TEXT("ASRProviderManager: Initialized provider '%s' with endpoint '%s'"), 
		*Provider->GetProviderName(), *InDefaultEndpointURL);
	
	return true;
}

TScriptInterface<IASRProvider> UASRProviderManager::GetActiveProvider() const
{
	return ActiveProvider;
}

bool UASRProviderManager::SetActiveProvider(EASRProviderType ProviderType, const FString& EndpointURL)
{
	return SetProviderEndpoint(EndpointURL, ProviderType);
}

bool UASRProviderManager::SetProviderEndpoint(const FString& EndpointURL, EASRProviderType ProviderType)
{
	// For hot-swapping, we don't auto-start containers (assume they're already running)
	if (!GRPCClientRef)
	{
		UE_LOG(LogTemp, Error, TEXT("ASRProviderManager: gRPC client is null"));
		return false;
	}

	// Determine provider type
	EASRProviderType ActualProviderType = ProviderType;
	if (ActualProviderType == EASRProviderType::AutoDetect)
	{
		ActualProviderType = AutoDetectProviderType(EndpointURL);
	}

	// Check if provider already exists for this type
	if (DefaultProviders.Contains(ActualProviderType))
	{
		TScriptInterface<IASRProvider> ExistingProvider = DefaultProviders[ActualProviderType];
		if (ExistingProvider.GetInterface() && ExistingProvider->Initialize(GRPCClientRef, EndpointURL))
		{
			ActiveProvider = ExistingProvider;
			UE_LOG(LogTemp, Log, TEXT("ASRProviderManager: Swapped to existing provider '%s'"), 
				*ExistingProvider->GetProviderName());
			return true;
		}
	}

	// Create new provider instance
	TScriptInterface<IASRProvider> Provider;
	
	switch (ActualProviderType)
	{
	case EASRProviderType::Riva:
	{
		UASRProviderRiva* RivaProvider = NewObject<UASRProviderRiva>(this);
		if (RivaProvider->Initialize(GRPCClientRef, EndpointURL))
		{
			Provider = RivaProvider;
			DefaultProviders.Add(EASRProviderType::Riva, Provider);
		}
		break;
	}
	case EASRProviderType::NIM:
	{
		UASRProviderNIM* NIMProvider = NewObject<UASRProviderNIM>(this);
		if (NIMProvider->Initialize(GRPCClientRef, EndpointURL))
		{
			Provider = NIMProvider;
			DefaultProviders.Add(EASRProviderType::NIM, Provider);
		}
		break;
	}
	default:
		UE_LOG(LogTemp, Error, TEXT("ASRProviderManager: Unknown provider type %d"), (int32)ActualProviderType);
		return false;
	}

	if (!Provider.GetInterface())
	{
		UE_LOG(LogTemp, Error, TEXT("ASRProviderManager: Failed to create provider for type %d"), 
			(int32)ActualProviderType);
		return false;
	}

	ActiveProvider = Provider;

	UE_LOG(LogTemp, Log, TEXT("ASRProviderManager: Swapped to provider '%s' with endpoint '%s'"), 
		*Provider->GetProviderName(), *EndpointURL);
	
	return true;
}

void UASRProviderManager::RequestTranscription(const FASRRequest& Request, TFunction<void(const FASRResponse&)> Callback)
{
	if (!ActiveProvider.GetInterface())
	{
		FASRResponse ErrorResponse;
		ErrorResponse.bSuccess = false;
		ErrorResponse.ErrorMessage = TEXT("No provider is currently active");
		Callback(ErrorResponse);
		return;
	}

	// Convert TFunction to dynamic delegate via callback proxy
	UASRTranscriptionCallbackProxy* CallbackProxy = NewObject<UASRTranscriptionCallbackProxy>(this);
	CallbackProxy->Initialize(this, MoveTemp(Callback));
	ActiveCallbackProxies.Add(CallbackProxy);

	FOnASRResponseReceived DelegateCallback;
	DelegateCallback.BindUFunction(CallbackProxy, TEXT("HandleResponse"));

	// Forward request to active provider
	ActiveProvider->RequestASRTranscription(Request, DelegateCallback);
}

void UASRProviderManager::RegisterCustomProvider(TScriptInterface<IASRProvider> CustomProvider, FName ProviderName)
{
	if (!CustomProvider.GetInterface())
	{
		UE_LOG(LogTemp, Warning, TEXT("ASRProviderManager: Attempted to register null custom provider"));
		return;
	}

	CustomProviders.Add(ProviderName, CustomProvider);
	UE_LOG(LogTemp, Log, TEXT("ASRProviderManager: Registered custom provider '%s'"), *ProviderName.ToString());
}

void UASRProviderManager::UnregisterCustomProvider(FName ProviderName)
{
	if (CustomProviders.Remove(ProviderName) > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("ASRProviderManager: Unregistered custom provider '%s'"), *ProviderName.ToString());
	}
}

TArray<FString> UASRProviderManager::GetAllProviderNames() const
{
	TArray<FString> Names;
	
	for (const auto& Pair : DefaultProviders)
	{
		if (Pair.Value.GetInterface())
		{
			Names.Add(Pair.Value->GetProviderName());
		}
	}
	
	for (const auto& Pair : CustomProviders)
	{
		if (Pair.Value.GetInterface())
		{
			Names.Add(Pair.Value->GetProviderName());
		}
	}
	
	return Names;
}

TArray<FString> UASRProviderManager::GetActiveProviderSupportedModels() const
{
	if (!ActiveProvider.GetInterface())
	{
		return TArray<FString>();
	}

	return ActiveProvider->GetSupportedModels();
}

bool UASRProviderManager::ActiveProviderSupportsStreaming() const
{
	if (!ActiveProvider.GetInterface())
	{
		return false;
	}

	return ActiveProvider->SupportsStreaming();
}

EASRProviderType UASRProviderManager::AutoDetectProviderType(const FString& EndpointURL) const
{
	// Auto-detect based on endpoint URL
	// Riva typically uses port 50051
	// NIM ASR models can use various ports (50051, 50052, etc.)
	// For now, default to Riva if port is 50051, otherwise NIM
	
	if (EndpointURL.Contains(TEXT("50051")) && !EndpointURL.Contains(TEXT("50052")) && !EndpointURL.Contains(TEXT("50053")))
	{
		// Port 50051 is typically Riva (but could also be NIM)
		// Default to Riva for backward compatibility
		return EASRProviderType::Riva;
	}
	
	// Default to NIM for other ports (Parakeet, Canary, etc.)
	return EASRProviderType::NIM;
}

void UASRProviderManager::HandleCallbackProxyFinished(UASRTranscriptionCallbackProxy* Proxy)
{
	if (!Proxy)
	{
		return;
	}

	ActiveCallbackProxies.Remove(Proxy);
}

