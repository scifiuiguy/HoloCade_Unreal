// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "IASRProvider.h"
#include "ASRProviderRiva.generated.h"

class UAIGRPCClient;

/**
 * @brief ASR Provider for NVIDIA Riva ASR.
 * Implements the IASRProvider interface for communication with NVIDIA Riva ASR services.
 * 
 * **NVIDIA Riva ASR:**
 * - Containerized (Docker) or local SDK installation
 * - gRPC protocol (streaming + offline)
 * - Production-ready, optimized for real-time
 * - Available via NIM containers or standalone Riva containers
 */
UCLASS(BlueprintType)
class HOLOCADEAI_API UASRProviderRiva : public UObject, public IASRProvider
{
	GENERATED_BODY()

public:
	virtual bool Initialize_Implementation(UAIGRPCClient* InGRPCClient, const FString& InEndpointURL) override;
	virtual void RequestASRTranscription_Implementation(const FASRRequest& Request, const FOnASRResponseReceived& Callback) override;
	virtual bool IsAvailable_Implementation() const override;
	virtual FString GetProviderName_Implementation() const override;
	virtual TArray<FString> GetSupportedModels_Implementation() const override;
	virtual bool SupportsStreaming_Implementation() const override;

private:
	UPROPERTY()
	TObjectPtr<UAIGRPCClient> GRPCClient;
	FString EndpointURL;
	bool bIsInitialized = false;
};

