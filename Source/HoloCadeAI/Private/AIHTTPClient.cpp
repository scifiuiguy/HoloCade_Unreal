// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "AIHTTPClient.h"
#include "JsonUtilities.h"
#include "Misc/DefaultValueHelper.h"

UAIHTTPClient::UAIHTTPClient()
{
}

UAIHTTPClient::~UAIHTTPClient()
{
}

void UAIHTTPClient::Get(const FString& URL, const TMap<FString, FString>& Headers, TFunction<void(const FAIHTTPResult&)> Callback)
{
	TSharedRef<IHttpRequest> Request = CreateRequest(URL, TEXT("GET"), Headers);
	ExecuteRequest(Request, Callback);
}

void UAIHTTPClient::PostJSON(const FString& URL, const TSharedPtr<FJsonObject>& JsonBody, const TMap<FString, FString>& Headers, TFunction<void(const FAIHTTPResult&)> Callback)
{
	FString JsonString;
	if (!SerializeJSONObject(JsonBody, JsonString))
	{
		FAIHTTPResult ErrorResult(false, 0, TEXT(""), TEXT("Failed to serialize JSON object"));
		if (Callback)
		{
			Callback(ErrorResult);
		}
		return;
	}

	PostString(URL, JsonString, TEXT("application/json"), Headers, Callback);
}

void UAIHTTPClient::PostString(const FString& URL, const FString& Body, const FString& ContentType, const TMap<FString, FString>& Headers, TFunction<void(const FAIHTTPResult&)> Callback)
{
	TSharedRef<IHttpRequest> Request = CreateRequest(URL, TEXT("POST"), Headers);
	
	Request->SetContentAsString(Body);
	Request->SetHeader(TEXT("Content-Type"), ContentType);
	
	ExecuteRequest(Request, Callback);
}

void UAIHTTPClient::PutJSON(const FString& URL, const TSharedPtr<FJsonObject>& JsonBody, const TMap<FString, FString>& Headers, TFunction<void(const FAIHTTPResult&)> Callback)
{
	FString JsonString;
	if (!SerializeJSONObject(JsonBody, JsonString))
	{
		FAIHTTPResult ErrorResult(false, 0, TEXT(""), TEXT("Failed to serialize JSON object"));
		if (Callback)
		{
			Callback(ErrorResult);
		}
		return;
	}

	TSharedRef<IHttpRequest> Request = CreateRequest(URL, TEXT("PUT"), Headers);
	
	Request->SetContentAsString(JsonString);
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	
	ExecuteRequest(Request, Callback);
}

void UAIHTTPClient::Delete(const FString& URL, const TMap<FString, FString>& Headers, TFunction<void(const FAIHTTPResult&)> Callback)
{
	TSharedRef<IHttpRequest> Request = CreateRequest(URL, TEXT("DELETE"), Headers);
	ExecuteRequest(Request, Callback);
}

bool UAIHTTPClient::ParseJSONResponse(const FString& ResponseBody, TSharedPtr<FJsonObject>& OutJsonObject)
{
	if (ResponseBody.IsEmpty())
	{
		OutJsonObject = nullptr;
		return false;
	}

	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseBody);
	
	if (!FJsonSerializer::Deserialize(Reader, OutJsonObject) || !OutJsonObject.IsValid())
	{
		OutJsonObject = nullptr;
		return false;
	}

	return true;
}

bool UAIHTTPClient::SerializeJSONObject(const TSharedPtr<FJsonObject>& JsonObject, FString& OutJsonString)
{
	if (!JsonObject.IsValid())
	{
		OutJsonString.Empty();
		return false;
	}

	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutJsonString);
	
	if (!FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer))
	{
		OutJsonString.Empty();
		return false;
	}

	return true;
}

FString UAIHTTPClient::BuildURLWithQuery(const FString& BaseURL, const TMap<FString, FString>& QueryParams)
{
	if (QueryParams.Num() == 0)
	{
		return BaseURL;
	}

	FString URL = BaseURL;
	bool bFirstParam = true;

	for (const auto& Param : QueryParams)
	{
		if (bFirstParam)
		{
			URL += TEXT("?");
			bFirstParam = false;
		}
		else
		{
			URL += TEXT("&");
		}

		// URL encode the parameter value
		FString EncodedValue = Param.Value;
		EncodedValue.ReplaceInline(TEXT(" "), TEXT("%20"));
		EncodedValue.ReplaceInline(TEXT("&"), TEXT("%26"));
		EncodedValue.ReplaceInline(TEXT("="), TEXT("%3D"));
		EncodedValue.ReplaceInline(TEXT("?"), TEXT("%3F"));
		EncodedValue.ReplaceInline(TEXT("#"), TEXT("%23"));
		EncodedValue.ReplaceInline(TEXT("/"), TEXT("%2F"));
		EncodedValue.ReplaceInline(TEXT("\\"), TEXT("%5C"));
		EncodedValue.ReplaceInline(TEXT("+"), TEXT("%2B"));
		EncodedValue.ReplaceInline(TEXT("\""), TEXT("%22"));
		EncodedValue.ReplaceInline(TEXT("'"), TEXT("%27"));
		
		URL += FString::Printf(TEXT("%s=%s"), *Param.Key, *EncodedValue);
	}

	return URL;
}

void UAIHTTPClient::ExecuteRequest(TSharedRef<IHttpRequest> Request, TFunction<void(const FAIHTTPResult&)> Callback)
{
	Request->SetTimeout(RequestTimeout);

	Request->OnProcessRequestComplete().BindLambda([Callback](FHttpRequestPtr RequestPtr, FHttpResponsePtr ResponsePtr, bool bWasSuccessful)
	{
		FAIHTTPResult Result;

		if (!bWasSuccessful || !ResponsePtr.IsValid())
		{
			Result.bSuccess = false;
			Result.ResponseCode = ResponsePtr.IsValid() ? ResponsePtr->GetResponseCode() : 0;
			Result.ErrorMessage = FString::Printf(TEXT("HTTP request failed: %s"), 
				RequestPtr.IsValid() ? *RequestPtr->GetURL() : TEXT("Invalid request"));
		}
		else
		{
			Result.bSuccess = (ResponsePtr->GetResponseCode() >= 200 && ResponsePtr->GetResponseCode() < 300);
			Result.ResponseCode = ResponsePtr->GetResponseCode();
			Result.ResponseBody = ResponsePtr->GetContentAsString();

			if (!Result.bSuccess)
			{
				Result.ErrorMessage = FString::Printf(TEXT("HTTP error %d: %s"), 
					Result.ResponseCode, *Result.ResponseBody);
			}
		}

		if (Callback)
		{
			Callback(Result);
		}
	});

	Request->ProcessRequest();
}

TSharedRef<IHttpRequest> UAIHTTPClient::CreateRequest(const FString& URL, const FString& Verb, const TMap<FString, FString>& Headers) const
{
	TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
	
	Request->SetURL(URL);
	Request->SetVerb(Verb);

	// Set custom headers
	for (const auto& Header : Headers)
	{
		Request->SetHeader(Header.Key, Header.Value);
	}

	// Set default headers if not already set
	if (!Headers.Contains(TEXT("Accept")))
	{
		Request->SetHeader(TEXT("Accept"), TEXT("application/json"));
	}

	return Request;
}

