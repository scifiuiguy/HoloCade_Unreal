// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "ArcadePaymentManager.h"
#include "Retail.h"
#include "JsonUtilities.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "HAL/PlatformProcess.h"
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "IPAddress.h"
#include "Common/TcpSocketBuilder.h"
#include "Serialization/ArrayReader.h"
#include "Serialization/ArrayWriter.h"

AArcadePaymentManager::AArcadePaymentManager(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;
}

void AArcadePaymentManager::BeginPlay()
{
	Super::BeginPlay();
	
	// Start webhook server
	StartWebhookServer();
	
	// Poll balance after 2 seconds
	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]()
	{
		CheckBalance(Config.CardId, [](float Balance)
		{
			UE_LOG(LogRetail, Log, TEXT("Initial Balance: %.2f"), Balance);
		});
	}, 2.0f, false);
}

void AArcadePaymentManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	bIsServerRunning = false;
	
	// Stop server thread
	if (ServerThread)
	{
		ServerThread->Kill(true);
		delete ServerThread;
		ServerThread = nullptr;
	}
	
	// Close listen socket
	if (ListenSocket)
	{
		ListenSocket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ListenSocket);
		ListenSocket = nullptr;
	}
	
	Super::EndPlay(EndPlayReason);
}

void AArcadePaymentManager::StartWebhookServer()
{
	if (bIsServerRunning)
	{
		return;
	}

	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (!SocketSubsystem)
	{
		UE_LOG(LogRetail, Error, TEXT("[Payment] Failed to get socket subsystem"));
		return;
	}

	// Create TCP listen socket
	FString LocalIPStr = GetLocalIP();
	FIPv4Address BindAddress;
	if (!FIPv4Address::Parse(LocalIPStr, BindAddress))
	{
		UE_LOG(LogRetail, Error, TEXT("[Payment] Invalid local IP address: %s"), *LocalIPStr);
		return;
	}

	ListenSocket = FTcpSocketBuilder(TEXT("HoloCade_PaymentWebhook"))
		.AsReusable()
		.BoundToAddress(BindAddress)
		.BoundToPort(WebhookPort)
		.Listening(8) // Max pending connections
		.Build();

	if (!ListenSocket)
	{
		UE_LOG(LogRetail, Error, TEXT("[Payment] Failed to create TCP listen socket on port %d"), WebhookPort);
		return;
	}

	bIsServerRunning = true;

	// Start background thread to accept connections
	// Create a simple runnable that processes webhook connections
	class FWebhookServerRunnable : public FRunnable
	{
	public:
		FWebhookServerRunnable(AArcadePaymentManager* InManager) : Manager(InManager) {}
		
		virtual uint32 Run() override
		{
			if (Manager)
			{
				Manager->ProcessWebhookConnections();
			}
			return 0;
		}
		
		virtual void Stop() override {}
		
	private:
		AArcadePaymentManager* Manager;
	};

	ServerThread = FRunnableThread::Create(
		new FWebhookServerRunnable(this),
		TEXT("PaymentWebhookServer"),
		0,
		TPri_Normal
	);

	if (!ServerThread)
	{
		UE_LOG(LogRetail, Error, TEXT("[Payment] Failed to create server thread"));
		bIsServerRunning = false;
		if (ListenSocket)
		{
			ListenSocket->Close();
			SocketSubsystem->DestroySocket(ListenSocket);
			ListenSocket = nullptr;
		}
		return;
	}

	UE_LOG(LogRetail, Log, TEXT("[Payment] Webhook server started on %s:%d/%s"), 
		*GetLocalIP(), WebhookPort, *GetWebhookPath());
}

FString AArcadePaymentManager::BuildEndpoint(const FString& Action, const TArray<FString>& Parts) const
{
	if (Parts.Num() == 0)
	{
		return Config.BaseUrl;
	}

	switch (Config.Provider)
	{
	case EPaymentProvider::Embed:
		if (Action == TEXT("balance"))
		{
			return FString::Printf(TEXT("%s/balance/%s"), *Config.BaseUrl, *Parts[0]);
		}
		if (Action == TEXT("allocate"))
		{
			return FString::Printf(TEXT("%s/allocate/%s/%s"), *Config.BaseUrl, *Parts[0], *Parts[1]);
		}
		break;

	case EPaymentProvider::Nayax:
		if (Action == TEXT("balance"))
		{
			return FString::Printf(TEXT("%s/v1/card/balance?card_id=%s"), *Config.BaseUrl, *Parts[0]);
		}
		if (Action == TEXT("allocate"))
		{
			return FString::Printf(TEXT("%s/credit/allocate"), *Config.BaseUrl);
		}
		break;

	case EPaymentProvider::Intercard:
		if (Action == TEXT("balance"))
		{
			return FString::Printf(TEXT("%s/api/player/balance?card=%s"), *Config.BaseUrl, *Parts[0]);
		}
		if (Action == TEXT("allocate"))
		{
			return FString::Printf(TEXT("%s/game/play"), *Config.BaseUrl);
		}
		break;

	case EPaymentProvider::CoreCashless:
		if (Action == TEXT("balance"))
		{
			return FString::Printf(TEXT("%s/balances/%s"), *Config.BaseUrl, *Parts[0]);
		}
		if (Action == TEXT("allocate"))
		{
			return FString::Printf(TEXT("%s/allocate/tokens"), *Config.BaseUrl);
		}
		break;

	case EPaymentProvider::Cantaloupe:
		if (Action == TEXT("balance"))
		{
			return FString::Printf(TEXT("%s/device/balance?device_id=%s"), *Config.BaseUrl, *Parts[0]);
		}
		if (Action == TEXT("allocate"))
		{
			return FString::Printf(TEXT("%s/play/allocate"), *Config.BaseUrl);
		}
		break;
	}

	return Config.BaseUrl;
}

FString AArcadePaymentManager::GetWebhookPath() const
{
	switch (Config.Provider)
	{
	case EPaymentProvider::Embed:
		return TEXT("embed");
	case EPaymentProvider::Nayax:
		return TEXT("nayax");
	case EPaymentProvider::Intercard:
		return TEXT("intercard");
	case EPaymentProvider::CoreCashless:
		return TEXT("core");
	case EPaymentProvider::Cantaloupe:
		return TEXT("cantaloupe");
	default:
		return TEXT("unknown");
	}
}

float AArcadePaymentManager::ExtractBalance(const FString& JsonString) const
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
	
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		return 0.0f;
	}

	// Provider-specific balance field extraction
	switch (Config.Provider)
	{
	case EPaymentProvider::Embed:
		return JsonObject->GetNumberField(TEXT("balance"));
	case EPaymentProvider::Nayax:
		return JsonObject->GetNumberField(TEXT("credits"));
	case EPaymentProvider::Intercard:
		return JsonObject->GetNumberField(TEXT("tokens"));
	case EPaymentProvider::CoreCashless:
		return JsonObject->GetNumberField(TEXT("balance"));
	case EPaymentProvider::Cantaloupe:
		return JsonObject->GetNumberField(TEXT("balance"));
	default:
		return 0.0f;
	}
}

FString AArcadePaymentManager::GetLocalIP() const
{
	bool bCanBindAll = false;
	TSharedPtr<FInternetAddr> Addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLocalHostAddr(*GLog, bCanBindAll);
	
	if (Addr.IsValid())
	{
		return Addr->ToString(false);
	}
	
	return TEXT("127.0.0.1");
}

void AArcadePaymentManager::CheckBalance(const FString& CardId, TFunction<void(float)> Callback)
{
	if (CardId.IsEmpty())
	{
		UE_LOG(LogRetail, Warning, TEXT("CheckBalance called with empty CardId"));
		if (Callback)
		{
			Callback(0.0f);
		}
		return;
	}

	TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(BuildEndpoint(TEXT("balance"), { CardId }));
	Request->SetVerb(TEXT("GET"));
	Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *Config.ApiKey));

	Request->OnProcessRequestComplete().BindLambda([this, Callback](FHttpRequestPtr Req, FHttpResponsePtr Resp, bool bSuccess)
	{
		if (bSuccess && Resp.IsValid() && Resp->GetResponseCode() == 200)
		{
			FString Content = Resp->GetContentAsString();
			float Balance = ExtractBalance(Content);
			
			if (Callback)
			{
				Callback(Balance);
			}
		}
		else
		{
			UE_LOG(LogRetail, Warning, TEXT("CheckBalance request failed: %d"), Resp.IsValid() ? Resp->GetResponseCode() : 0);
			if (Callback)
			{
				Callback(0.0f);
			}
		}
	});

	Request->ProcessRequest();
}

void AArcadePaymentManager::AllocateTokens(const FString& StationId, float Amount, TFunction<void(bool)> Callback)
{
	if (StationId.IsEmpty())
	{
		UE_LOG(LogRetail, Warning, TEXT("AllocateTokens called with empty StationId"));
		if (Callback)
		{
			Callback(false);
		}
		return;
	}

	TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(BuildEndpoint(TEXT("allocate"), { StationId, FString::SanitizeFloat(Amount) }));
	Request->SetVerb(TEXT("POST"));

	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetStringField(TEXT("cardId"), Config.CardId);
	JsonObject->SetNumberField(TEXT("amount"), Amount);
	JsonObject->SetStringField(TEXT("stationId"), StationId);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	Request->SetContentAsString(OutputString);
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *Config.ApiKey));

	Request->OnProcessRequestComplete().BindLambda([Callback](FHttpRequestPtr, FHttpResponsePtr Resp, bool bSuccess)
	{
		bool bResult = bSuccess && Resp.IsValid() && Resp->GetResponseCode() == 200;
		if (Callback)
		{
			Callback(bResult);
		}
	});

	Request->ProcessRequest();
}

void AArcadePaymentManager::ProcessWebhookConnections()
{
	if (!ListenSocket || !bIsServerRunning)
	{
		return;
	}

	while (bIsServerRunning && ListenSocket)
	{
		// Check for incoming connections (non-blocking)
		bool bHasPendingConnection = false;
		if (ListenSocket->HasPendingConnection(bHasPendingConnection) && bHasPendingConnection)
		{
			// Accept the connection
			TSharedRef<FInternetAddr> ClientAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
			FSocket* ClientSocket = ListenSocket->Accept(*ClientAddr, TEXT("PaymentWebhookClient"));
			
			if (ClientSocket)
			{
				// Set socket to blocking for reading
				ClientSocket->SetNonBlocking(false);
				
				// Read request data
				TArray<uint8> RequestData;
				RequestData.SetNumUninitialized(8192);
				int32 DataSize = 0;
				
				// Wait for data with timeout
				if (ClientSocket->Wait(ESocketWaitConditions::WaitForRead, FTimespan::FromSeconds(5)))
				{
					if (ClientSocket->Recv(RequestData.GetData(), RequestData.Num(), DataSize))
					{
						RequestData.SetNum(DataSize);
					}
					else
					{
						DataSize = 0;
					}
					
					if (DataSize > 0)
					{
						FString Method, Path, Body;
						if (ParseHTTPRequest(RequestData, Method, Path, Body))
						{
							// Check if this is a POST to our webhook path
							if (Method == TEXT("POST") && Path.Contains(GetWebhookPath()))
							{
								// Parse JSON payload
								TSharedPtr<FJsonObject> JsonObject;
								TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Body);
								
								if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
								{
									FString CardId = JsonObject->GetStringField(TEXT("cardId"));
									float Amount = JsonObject->GetNumberField(TEXT("amount"));
									float NewBalance = JsonObject->GetNumberField(TEXT("newBalance"));

									// Process on game thread
									AsyncTask(ENamedThreads::GameThread, [this, CardId, NewBalance]()
									{
										StartSession(CardId, NewBalance);
									});
								}
							}
							
							// Send HTTP 200 OK response
							SendHTTPResponse(ClientSocket, 200, TEXT("OK"));
						}
						else
						{
							// Send HTTP 400 Bad Request
							SendHTTPResponse(ClientSocket, 400, TEXT("Bad Request"));
						}
					}
				}
				
				// Close client connection
				ClientSocket->Close();
				ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ClientSocket);
			}
		}
		
		// Small delay to prevent busy-waiting
		FPlatformProcess::Sleep(0.01f);
	}
}

bool AArcadePaymentManager::ParseHTTPRequest(const TArray<uint8>& Data, FString& OutMethod, FString& OutPath, FString& OutBody)
{
	if (Data.Num() == 0)
	{
		return false;
	}

	// Convert UTF-8 bytes to FString
	FUTF8ToTCHAR UTF8String((const ANSICHAR*)Data.GetData(), Data.Num());
	FString RequestString(UTF8String.Length(), UTF8String.Get());

	// Parse HTTP request line
	TArray<FString> Lines;
	RequestString.ParseIntoArrayLines(Lines);

	if (Lines.Num() == 0)
	{
		return false;
	}

	// Parse first line: "METHOD /path HTTP/1.1"
	TArray<FString> RequestLineParts;
	Lines[0].ParseIntoArray(RequestLineParts, TEXT(" "), true);
	
	if (RequestLineParts.Num() < 2)
	{
		return false;
	}

	OutMethod = RequestLineParts[0];
	OutPath = RequestLineParts[1];

	// Find body (after double CRLF)
	int32 BodyStart = RequestString.Find(TEXT("\r\n\r\n"));
	if (BodyStart >= 0)
	{
		BodyStart += 4; // Skip "\r\n\r\n"
		OutBody = RequestString.Mid(BodyStart);
	}

	return true;
}

void AArcadePaymentManager::SendHTTPResponse(FSocket* ClientSocket, int32 StatusCode, const FString& Body)
{
	if (!ClientSocket)
	{
		return;
	}

	FString StatusText = (StatusCode == 200) ? TEXT("OK") : TEXT("Bad Request");
	FString Response = FString::Printf(
		TEXT("HTTP/1.1 %d %s\r\n")
		TEXT("Content-Type: text/plain\r\n")
		TEXT("Content-Length: %d\r\n")
		TEXT("Connection: close\r\n")
		TEXT("\r\n")
		TEXT("%s"),
		StatusCode, *StatusText, Body.Len(), *Body
	);

	TArray<uint8> ResponseData;
	FTCHARToUTF8 UTF8String(*Response);
	ResponseData.Append((uint8*)UTF8String.Get(), UTF8String.Length());

	int32 BytesSent = 0;
	ClientSocket->Send(ResponseData.GetData(), ResponseData.Num(), BytesSent);
}

void AArcadePaymentManager::StartSession(const FString& CardId, float Balance)
{
	UE_LOG(LogRetail, Log, TEXT("[Payment:%s] Unlocking VR for %s | Balance: %.2f"), 
		*UEnum::GetValueAsString(Config.Provider), *CardId, Balance);
	
	// TODO: Integrate with VR session manager to start gameplay
	// Example: GetWorld()->GetGameInstance<UHoloCadeGameInstance>()->StartVRSession(CardId, Balance);
}
