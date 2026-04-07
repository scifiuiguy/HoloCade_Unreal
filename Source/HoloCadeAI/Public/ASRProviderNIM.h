// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "IASRProvider.h"
#include "ASRProviderNIM.generated.h"

class UAIGRPCClient;

/**
 * @brief NIM ASR Model Type
 */
UENUM(BlueprintType)
enum class ENIMASRModelType : uint8
{
	Parakeet_0_6B_English UMETA(DisplayName = "Parakeet 0.6B English"),
	Parakeet_1_1B_Multilingual UMETA(DisplayName = "Parakeet 1.1B Multilingual"),
	Canary_1B_Multilingual UMETA(DisplayName = "Canary 1B Multilingual"),
	Whisper_Small UMETA(DisplayName = "Whisper Small"),
	Whisper_Medium UMETA(DisplayName = "Whisper Medium"),
	Whisper_Large UMETA(DisplayName = "Whisper Large"),
	AutoDetect UMETA(DisplayName = "Auto-Detect")
};

/**
 * @brief ASR Provider for NVIDIA NIM ASR models (Parakeet, Canary, Whisper).
 * Implements the IASRProvider interface for communication with NVIDIA NIM ASR services.
 * 
 * **NVIDIA NIM ASR Models:**
 * - Parakeet (0.6B English, 1.1B Multilingual) - ✅ Streaming gRPC support
 * - Canary (1B Multilingual) - ✅ Streaming gRPC support, includes translation
 * - Whisper (Small, Medium, Large) - ⚠️ gRPC offline only (not suitable for real-time)
 * 
 * **All NIM ASR models:**
 * - Containerized (Docker)
 * - gRPC protocol (streaming for Parakeet/Canary, offline only for Whisper)
 * - Hot-swappable (change endpoint URL to swap models)
 */
UCLASS(BlueprintType)
class HOLOCADEAI_API UASRProviderNIM : public UObject, public IASRProvider
{
	GENERATED_BODY()

public:

	/** Model type (auto-detected from endpoint if not specified) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ASR Provider|NIM")
	ENIMASRModelType ModelType = ENIMASRModelType::AutoDetect;

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

	/**
	 * @brief Auto-detect model type from endpoint URL or container name
	 */
	ENIMASRModelType DetectModelType(const FString& InEndpointURL) const;
};

