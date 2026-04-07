// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "AIAPI.h"  // For HOLOCADEAI_API macro
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Json.h"
#include "AIHTTPClient.generated.h"

/**
 * HTTP Request Result
 */
USTRUCT(BlueprintType)
struct HOLOCADEAI_API FAIHTTPResult
{
	GENERATED_BODY()

	/** Whether the request was successful */
	UPROPERTY(BlueprintReadOnly, Category = "HTTP")
	bool bSuccess = false;

	/** HTTP response code (200 = success, etc.) */
	UPROPERTY(BlueprintReadOnly, Category = "HTTP")
	int32 ResponseCode = 0;

	/** Response body as string */
	UPROPERTY(BlueprintReadOnly, Category = "HTTP")
	FString ResponseBody;

	/** Error message if request failed */
	UPROPERTY(BlueprintReadOnly, Category = "HTTP")
	FString ErrorMessage;

	FAIHTTPResult()
		: bSuccess(false)
		, ResponseCode(0)
	{
	}

	FAIHTTPResult(bool bInSuccess, int32 InResponseCode, const FString& InResponseBody, const FString& InErrorMessage = TEXT(""))
		: bSuccess(bInSuccess)
		, ResponseCode(InResponseCode)
		, ResponseBody(InResponseBody)
		, ErrorMessage(InErrorMessage)
	{
	}
};

/**
 * Generic HTTP Client for AI Service Integration
 * 
 * Provides async HTTP request/response handling with JSON support.
 * Used by all AI service managers (LLM, ASR, TTS, Audio2Face, etc.)
 * to communicate with AI service endpoints.
 * 
 * Features:
 * - Async request/response handling with callbacks
 * - JSON serialization/deserialization
 * - Error handling and retry logic
 * - Support for POST, GET, PUT, DELETE methods
 * - Custom headers and authentication
 */
UCLASS()
class HOLOCADEAI_API UAIHTTPClient : public UObject
{
	GENERATED_BODY()

public:
	UAIHTTPClient();
	virtual ~UAIHTTPClient();

	/**
	 * Make an async HTTP GET request
	 * @param URL - Full URL to request
	 * @param Headers - Optional custom headers (key-value pairs)
	 * @param Callback - Callback function called when request completes
	 */
	void Get(const FString& URL, const TMap<FString, FString>& Headers, TFunction<void(const FAIHTTPResult&)> Callback);

	/**
	 * Make an async HTTP POST request with JSON body
	 * @param URL - Full URL to request
	 * @param JsonBody - JSON object to send as request body
	 * @param Headers - Optional custom headers (key-value pairs)
	 * @param Callback - Callback function called when request completes
	 */
	void PostJSON(const FString& URL, const TSharedPtr<FJsonObject>& JsonBody, const TMap<FString, FString>& Headers, TFunction<void(const FAIHTTPResult&)> Callback);

	/**
	 * Make an async HTTP POST request with string body
	 * @param URL - Full URL to request
	 * @param Body - String body to send
	 * @param ContentType - Content-Type header (default: "application/json")
	 * @param Headers - Optional custom headers (key-value pairs)
	 * @param Callback - Callback function called when request completes
	 */
	void PostString(const FString& URL, const FString& Body, const FString& ContentType, const TMap<FString, FString>& Headers, TFunction<void(const FAIHTTPResult&)> Callback);

	/**
	 * Make an async HTTP PUT request with JSON body
	 * @param URL - Full URL to request
	 * @param JsonBody - JSON object to send as request body
	 * @param Headers - Optional custom headers (key-value pairs)
	 * @param Callback - Callback function called when request completes
	 */
	void PutJSON(const FString& URL, const TSharedPtr<FJsonObject>& JsonBody, const TMap<FString, FString>& Headers, TFunction<void(const FAIHTTPResult&)> Callback);

	/**
	 * Make an async HTTP DELETE request
	 * @param URL - Full URL to request
	 * @param Headers - Optional custom headers (key-value pairs)
	 * @param Callback - Callback function called when request completes
	 */
	void Delete(const FString& URL, const TMap<FString, FString>& Headers, TFunction<void(const FAIHTTPResult&)> Callback);

	/**
	 * Parse JSON response body into FJsonObject
	 * @param ResponseBody - JSON string to parse
	 * @param OutJsonObject - Output JSON object (nullptr if parsing failed)
	 * @return True if parsing succeeded
	 */
	static bool ParseJSONResponse(const FString& ResponseBody, TSharedPtr<FJsonObject>& OutJsonObject);

	/**
	 * Serialize FJsonObject to JSON string
	 * @param JsonObject - JSON object to serialize
	 * @param OutJsonString - Output JSON string
	 * @return True if serialization succeeded
	 */
	static bool SerializeJSONObject(const TSharedPtr<FJsonObject>& JsonObject, FString& OutJsonString);

	/**
	 * Build URL with query parameters
	 * @param BaseURL - Base URL
	 * @param QueryParams - Query parameters (key-value pairs)
	 * @return URL with query string appended
	 */
	static FString BuildURLWithQuery(const FString& BaseURL, const TMap<FString, FString>& QueryParams);

private:
	/**
	 * Internal: Execute HTTP request
	 * @param Request - HTTP request to execute
	 * @param Callback - Callback function
	 */
	void ExecuteRequest(TSharedRef<IHttpRequest> Request, TFunction<void(const FAIHTTPResult&)> Callback);

	/**
	 * Internal: Create HTTP request with common settings
	 * @param URL - Request URL
	 * @param Verb - HTTP verb (GET, POST, etc.)
	 * @param Headers - Custom headers
	 * @return HTTP request
	 */
	TSharedRef<IHttpRequest> CreateRequest(const FString& URL, const FString& Verb, const TMap<FString, FString>& Headers) const;

	/** Default timeout for HTTP requests (seconds) */
	UPROPERTY(EditAnywhere, Category = "HTTP")
	float RequestTimeout = 30.0f;

	/** Maximum number of retries for failed requests */
	UPROPERTY(EditAnywhere, Category = "HTTP")
	int32 MaxRetries = 3;
};

