// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Json.h"
#include "Retail.h"
#include "ArcadePaymentManager.generated.h"

/**
 * Payment Provider Types
 */
UENUM(BlueprintType)
enum class EPaymentProvider : uint8
{
	Embed			UMETA(DisplayName = "Embed"),
	Nayax			UMETA(DisplayName = "Nayax"),
	Intercard		UMETA(DisplayName = "Intercard"),
	CoreCashless	UMETA(DisplayName = "Core Cashless"),
	Cantaloupe		UMETA(DisplayName = "Cantaloupe")
};

/**
 * Payment Configuration Structure
 */
USTRUCT(BlueprintType)
struct RETAIL_API FPaymentConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Payment")
	EPaymentProvider Provider = EPaymentProvider::Embed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Payment")
	FString ApiKey;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Payment")
	FString BaseUrl;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Payment")
	FString CardId;
};

/**
 * Arcade Payment Manager - Handles cashless tap card payment interface for VR tap-to-play capability.
 * 
 * Supports multiple payment providers (Embed, Nayax, Intercard, Core Cashless, Cantaloupe) and provides
 * webhook server for receiving payment confirmations and API methods for checking balances and allocating tokens.
 */
UCLASS(BlueprintType, ClassGroup=(HoloCade))
class RETAIL_API AArcadePaymentManager : public AActor
{
	GENERATED_BODY()

public:
	AArcadePaymentManager(const FObjectInitializer& ObjectInitializer);

	/** Payment configuration */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Payment")
	FPaymentConfig Config;

	/** Start a VR session for the given card ID with the specified balance */
	UFUNCTION(BlueprintCallable, Category = "Payment")
	void StartSession(const FString& CardId, float Balance);

	/** Check the balance for a card ID (async callback) - C++ only (TFunction cannot be exposed to Blueprint) */
	void CheckBalance(const FString& CardId, TFunction<void(float)> Callback);

	/** Allocate tokens/credits for gameplay (async callback) - C++ only (TFunction cannot be exposed to Blueprint) */
	void AllocateTokens(const FString& StationId, float Amount, TFunction<void(bool)> Callback);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Start the webhook server to receive payment confirmations */
	void StartWebhookServer();

	/** Build provider-specific API endpoint URL */
	FString BuildEndpoint(const FString& Action, const TArray<FString>& Parts) const;

	/** Get webhook path based on provider */
	FString GetWebhookPath() const;

	/** Extract balance from JSON response (provider-specific) */
	float ExtractBalance(const FString& JsonString) const;

	/** Get local IP address for webhook server */
	FString GetLocalIP() const;

	/** Webhook server port */
	UPROPERTY(EditAnywhere, Category = "Payment")
	int32 WebhookPort = 8080;

	/** Whether webhook server is running */
	bool bIsServerRunning = false;

	/** TCP listen socket for webhook server */
	FSocket* ListenSocket = nullptr;

	/** Background thread for accepting connections */
	FRunnableThread* ServerThread = nullptr;

	/** Process incoming HTTP connections */
	void ProcessWebhookConnections();

	/** Parse HTTP request from raw data */
	bool ParseHTTPRequest(const TArray<uint8>& Data, FString& OutMethod, FString& OutPath, FString& OutBody);

	/** Send HTTP response */
	void SendHTTPResponse(FSocket* ClientSocket, int32 StatusCode, const FString& Body);
};
