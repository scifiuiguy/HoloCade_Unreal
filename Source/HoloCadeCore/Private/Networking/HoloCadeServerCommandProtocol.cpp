// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "Networking/HoloCadeServerCommandProtocol.h"
#include "Common/UdpSocketBuilder.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonReader.h"
#include "Dom/JsonObject.h"
#include "HAL/PlatformProcess.h"

UHoloCadeServerCommandProtocol::UHoloCadeServerCommandProtocol()
{
	CommandPort = 7779;
	NextSequenceNumber = 0;
	bIsActive = false;
	bIsListening = false;
}

UHoloCadeServerCommandProtocol::~UHoloCadeServerCommandProtocol()
{
	ShutdownClient();
	StopListening();
}

bool UHoloCadeServerCommandProtocol::InitializeClient(const FString& ServerIP, int32 ServerPort)
{
	if (bIsActive)
	{
		UE_LOG(LogTemp, Warning, TEXT("HoloCadeServerCommandProtocol: Client already initialized"));
		return false;
	}

	TargetServerIP = ServerIP;
	TargetServerPort = ServerPort > 0 ? ServerPort : CommandPort;

	if (!CreateClientSocket())
	{
		OnClientError.Broadcast(TEXT("Failed to create client socket"));
		return false;
	}

	bIsActive = true;
	NextSequenceNumber = 0;

	UE_LOG(LogTemp, Log, TEXT("HoloCadeServerCommandProtocol: Client initialized (target: %s:%d)"), 
		*TargetServerIP, TargetServerPort);
	
	OnClientInitialized.Broadcast(FString::Printf(TEXT("Connected to %s:%d"), *TargetServerIP, TargetServerPort));
	return true;
}

void UHoloCadeServerCommandProtocol::ShutdownClient()
{
	if (!bIsActive)
	{
		return;
	}

	CleanupSocket(CommandSocket);
	RemoteServerAddr.Reset();
	bIsActive = false;

	UE_LOG(LogTemp, Log, TEXT("HoloCadeServerCommandProtocol: Client shutdown"));
	OnClientShutdown.Broadcast(TEXT("Client shutdown"));
}

FHoloCadeServerResponseMessage UHoloCadeServerCommandProtocol::SendCommand(EHoloCadeServerCommand Command, const FString& Parameter)
{
	if (!bIsActive || !CommandSocket || !RemoteServerAddr.IsValid())
	{
		return FHoloCadeServerResponseMessage(false, TEXT("Not connected to server"));
	}

	// Create command message with sequence number
	FHoloCadeServerCommandMessage CommandMsg(Command, Parameter, NextSequenceNumber++);
	
	// Generate auth token if enabled
	if (bEnableAuthentication && !SharedSecret.IsEmpty())
	{
		CommandMsg.AuthToken = GenerateAuthToken(CommandMsg);
	}
	
	// Serialize to JSON
	FString JsonString = SerializeCommand(CommandMsg);
	
	// Convert to bytes
	TArray<uint8> Data;
	FTCHARToUTF8 UTF8String(*JsonString);
	Data.Append((uint8*)UTF8String.Get(), UTF8String.Length());

	// Send via UDP
	bool bSuccess = SendUDPData(CommandSocket, Data, RemoteServerAddr.ToSharedRef());
	
	if (bSuccess)
	{
		UE_LOG(LogTemp, Verbose, TEXT("HoloCadeServerCommandProtocol: Sent command %d (seq: %d)"), 
			(uint8)Command, CommandMsg.SequenceNumber);
		
		// For UDP, we return a generic success - actual response would come via TickClient
		return FHoloCadeServerResponseMessage(true, TEXT("Command sent"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("HoloCadeServerCommandProtocol: Failed to send command %d"), (uint8)Command);
		return FHoloCadeServerResponseMessage(false, TEXT("Failed to send command"));
	}
}

bool UHoloCadeServerCommandProtocol::StartListening()
{
	if (bIsListening)
	{
		UE_LOG(LogTemp, Warning, TEXT("HoloCadeServerCommandProtocol: Already listening"));
		return false;
	}

	if (!CreateListenSocket())
	{
		OnServerStopped.Broadcast(TEXT("Failed to create listen socket"));
		return false;
	}

	bIsListening = true;

	UE_LOG(LogTemp, Log, TEXT("HoloCadeServerCommandProtocol: Started listening on port %d"), CommandPort);
	OnServerStarted.Broadcast(FString::Printf(TEXT("Listening on port %d"), CommandPort));
	return true;
}

void UHoloCadeServerCommandProtocol::StopListening()
{
	if (!bIsListening)
	{
		return;
	}

	CleanupSocket(ListenSocket);
	bIsListening = false;

	UE_LOG(LogTemp, Log, TEXT("HoloCadeServerCommandProtocol: Stopped listening"));
	OnServerStopped.Broadcast(TEXT("Stopped listening"));
}

void UHoloCadeServerCommandProtocol::Tick(float DeltaTime)
{
	if (!bIsListening || !ListenSocket)
	{
		return;
	}

	ProcessIncomingCommands();
}

void UHoloCadeServerCommandProtocol::TickClient(float DeltaTime)
{
	if (!bIsActive || !CommandSocket)
	{
		return;
	}

	// Process incoming responses (optional - for request-response mode)
	TArray<uint8> ReceivedData;
	TSharedPtr<FInternetAddr> Sender;
	if (ReceiveUDPData(CommandSocket, ReceivedData, Sender))
	{
		// Deserialize response
		FString JsonString;
		JsonString.AppendChars((TCHAR*)ReceivedData.GetData(), ReceivedData.Num());
		
		FHoloCadeServerResponseMessage Response;
		if (DeserializeResponse(JsonString, Response))
		{
			UE_LOG(LogTemp, Verbose, TEXT("HoloCadeServerCommandProtocol: Received response: %s"), *Response.Message);
			// Could broadcast response via delegate if needed
		}
	}
}

bool UHoloCadeServerCommandProtocol::CreateClientSocket()
{
	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (!SocketSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("HoloCadeServerCommandProtocol: Failed to get socket subsystem"));
		return false;
	}

	// Create UDP socket
	CommandSocket = SocketSubsystem->CreateSocket(NAME_DGram, TEXT("HoloCade_CommandClient"), false);
	if (!CommandSocket)
	{
		UE_LOG(LogTemp, Error, TEXT("HoloCadeServerCommandProtocol: Failed to create client socket"));
		return false;
	}

	// Make non-blocking
	CommandSocket->SetNonBlocking(true);

	// Set send buffer size
	int32 NewSize = 8192;
	CommandSocket->SetSendBufferSize(NewSize, NewSize);

	// Create remote address
	RemoteServerAddr = SocketSubsystem->CreateInternetAddr();
	bool bIsValid = false;
	RemoteServerAddr->SetIp(*TargetServerIP, bIsValid);
	RemoteServerAddr->SetPort(TargetServerPort);

	if (!bIsValid)
	{
		UE_LOG(LogTemp, Error, TEXT("HoloCadeServerCommandProtocol: Invalid server IP: %s"), *TargetServerIP);
		SocketSubsystem->DestroySocket(CommandSocket);
		CommandSocket = nullptr;
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("HoloCadeServerCommandProtocol: Client socket created (target: %s:%d)"), 
		*TargetServerIP, TargetServerPort);
	return true;
}

bool UHoloCadeServerCommandProtocol::CreateListenSocket()
{
	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (!SocketSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("HoloCadeServerCommandProtocol: Failed to get socket subsystem"));
		return false;
	}

	// Create UDP listen socket
	ListenSocket = FUdpSocketBuilder(TEXT("HoloCade_CommandServer"))
		.AsNonBlocking()
		.WithReceiveBufferSize(8192)
		.BoundToPort(CommandPort)
		.Build();

	if (!ListenSocket)
	{
		UE_LOG(LogTemp, Error, TEXT("HoloCadeServerCommandProtocol: Failed to create listen socket on port %d"), CommandPort);
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("HoloCadeServerCommandProtocol: Listen socket created on port %d"), CommandPort);
	return true;
}

void UHoloCadeServerCommandProtocol::ProcessIncomingCommands()
{
	if (!ListenSocket)
	{
		return;
	}

	// Process incoming packets
	TArray<uint8> ReceivedData;
	TSharedPtr<FInternetAddr> Sender;
	
	while (ReceiveUDPData(ListenSocket, ReceivedData, Sender))
	{
		if (ReceivedData.Num() == 0)
		{
			continue;
		}

		// Convert to string
		FString JsonString;
		JsonString.AppendChars((TCHAR*)ReceivedData.GetData(), ReceivedData.Num());

		// Deserialize command
		FHoloCadeServerCommandMessage Command;
		if (DeserializeCommand(JsonString, Command))
		{
			// Validate authentication if enabled
			if (bEnableAuthentication)
			{
				if (!ValidateAuthToken(Command))
				{
					UE_LOG(LogTemp, Warning, TEXT("HoloCadeServerCommandProtocol: Authentication failed for command %d from %s"), 
						(uint8)Command.Command, 
						Sender.IsValid() ? *Sender->ToString(false) : TEXT("unknown"));
					
					// Send authentication failure response
					if (Sender.IsValid())
					{
						FHoloCadeServerResponseMessage AuthFailureResponse(false, TEXT("Authentication failed"));
						SendResponse(AuthFailureResponse, Sender.ToSharedRef());
					}
					continue;
				}
			}

			UE_LOG(LogTemp, Verbose, TEXT("HoloCadeServerCommandProtocol: Received command %d (seq: %d) from %s"), 
				(uint8)Command.Command, Command.SequenceNumber, 
				Sender.IsValid() ? *Sender->ToString(false) : TEXT("unknown"));

			// Store sender address for response
			LastSenderAddress = Sender;

			// Broadcast command to handlers - they can request response via SendResponse using GetLastSenderAddress()
			OnCommandReceived.Broadcast(Command, this);
			
			// Clear sender address after delegate (handlers should use GetLastSenderAddress() during delegate execution)
			// We'll keep it for now in case handler needs it later

			// Optionally send response back (fire-and-forget mode doesn't need this)
			// For request-response mode, we could send a response here
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("HoloCadeServerCommandProtocol: Failed to deserialize command from %s"), 
				Sender.IsValid() ? *Sender->ToString(false) : TEXT("unknown"));
		}

		ReceivedData.Reset();
	}
}

FString UHoloCadeServerCommandProtocol::SerializeCommand(const FHoloCadeServerCommandMessage& Command) const
{
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetNumberField(TEXT("Command"), (uint8)Command.Command);
	JsonObject->SetStringField(TEXT("Parameter"), Command.Parameter);
	JsonObject->SetNumberField(TEXT("Timestamp"), Command.Timestamp);
	JsonObject->SetNumberField(TEXT("SequenceNumber"), (double)Command.SequenceNumber);
	
	if (!Command.AuthToken.IsEmpty())
	{
		JsonObject->SetStringField(TEXT("AuthToken"), Command.AuthToken);
	}

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	return OutputString;
}

bool UHoloCadeServerCommandProtocol::DeserializeCommand(const FString& JsonString, FHoloCadeServerCommandMessage& OutCommand) const
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		return false;
	}

	uint8 CommandValue = 0;
	if (JsonObject->TryGetNumberField(TEXT("Command"), CommandValue))
	{
		OutCommand.Command = (EHoloCadeServerCommand)CommandValue;
	}

	JsonObject->TryGetStringField(TEXT("Parameter"), OutCommand.Parameter);
	JsonObject->TryGetNumberField(TEXT("Timestamp"), OutCommand.Timestamp);
	
	double SeqNum = 0;
	if (JsonObject->TryGetNumberField(TEXT("SequenceNumber"), SeqNum))
	{
		OutCommand.SequenceNumber = (uint32)SeqNum;
	}
	
	JsonObject->TryGetStringField(TEXT("AuthToken"), OutCommand.AuthToken);

	return true;
}

FString UHoloCadeServerCommandProtocol::SerializeResponse(const FHoloCadeServerResponseMessage& Response) const
{
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetBoolField(TEXT("Success"), Response.bSuccess);
	JsonObject->SetStringField(TEXT("Message"), Response.Message);
	JsonObject->SetStringField(TEXT("Data"), Response.Data);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	return OutputString;
}

bool UHoloCadeServerCommandProtocol::DeserializeResponse(const FString& JsonString, FHoloCadeServerResponseMessage& OutResponse) const
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		return false;
	}

	JsonObject->TryGetBoolField(TEXT("Success"), OutResponse.bSuccess);
	JsonObject->TryGetStringField(TEXT("Message"), OutResponse.Message);
	JsonObject->TryGetStringField(TEXT("Data"), OutResponse.Data);

	return true;
}

bool UHoloCadeServerCommandProtocol::SendUDPData(FSocket* Socket, const TArray<uint8>& Data, TSharedRef<FInternetAddr> Address)
{
	if (!Socket || Data.Num() == 0)
	{
		return false;
	}

	int32 BytesSent = 0;
	bool bSuccess = Socket->SendTo(Data.GetData(), Data.Num(), BytesSent, *Address);

	if (!bSuccess || BytesSent != Data.Num())
	{
		UE_LOG(LogTemp, Warning, TEXT("HoloCadeServerCommandProtocol: Failed to send %d bytes (sent: %d)"), 
			Data.Num(), BytesSent);
		return false;
	}

	return true;
}

bool UHoloCadeServerCommandProtocol::ReceiveUDPData(FSocket* Socket, TArray<uint8>& OutData, TSharedPtr<FInternetAddr>& OutSender)
{
	if (!Socket)
	{
		return false;
	}

	// Check if data is available
	uint32 DataSize = 0;
	if (!Socket->HasPendingData(DataSize) || DataSize == 0)
	{
		return false;
	}

	// Allocate buffer
	OutData.SetNumUninitialized(DataSize);
	
	// Create sender address
	OutSender = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	
	// Receive data
	int32 BytesRead = 0;
	if (Socket->RecvFrom(OutData.GetData(), OutData.Num(), BytesRead, *OutSender))
	{
		if (BytesRead > 0 && BytesRead <= (int32)DataSize)
		{
			OutData.SetNum(BytesRead);
			return true;
		}
	}

	OutData.Reset();
	return false;
}

void UHoloCadeServerCommandProtocol::CleanupSockets()
{
	CleanupSocket(CommandSocket);
	CleanupSocket(ListenSocket);
	RemoteServerAddr.Reset();
}

void UHoloCadeServerCommandProtocol::CleanupSocket(FSocket*& Socket)
{
	if (Socket)
	{
		Socket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);
		Socket = nullptr;
	}
}

FString UHoloCadeServerCommandProtocol::GenerateAuthToken(const FHoloCadeServerCommandMessage& Command) const
{
	// Simple HMAC-like approach: hash(command + timestamp + sequence + secret)
	// For production, consider using proper HMAC-SHA256
	FString DataToHash = FString::Printf(TEXT("%d_%f_%u_%s"), 
		(uint8)Command.Command, 
		Command.Timestamp, 
		Command.SequenceNumber,
		*SharedSecret);
	
	// Simple hash (for production, use proper crypto library)
	uint32 Hash = GetTypeHash(DataToHash);
	return FString::Printf(TEXT("%08X"), Hash);
}

bool UHoloCadeServerCommandProtocol::ValidateAuthToken(const FHoloCadeServerCommandMessage& Command) const
{
	if (Command.AuthToken.IsEmpty())
	{
		return false;
	}
	
	// Generate expected token
	FString ExpectedToken = GenerateAuthToken(Command);
	
	// Compare (constant-time comparison would be better, but this is simpler)
	return ExpectedToken == Command.AuthToken;
}

void UHoloCadeServerCommandProtocol::SendResponse(const FHoloCadeServerResponseMessage& Response, TSharedRef<FInternetAddr> ClientAddress)
{
	if (!ListenSocket)
	{
		return;
	}
	
	// Serialize response
	FString JsonString = SerializeResponse(Response);
	
	// Convert to bytes
	TArray<uint8> Data;
	FTCHARToUTF8 UTF8String(*JsonString);
	Data.Append((uint8*)UTF8String.Get(), UTF8String.Length());
	
	// Send via UDP
	bool bSuccess = SendUDPData(ListenSocket, Data, ClientAddress);
	
	if (bSuccess)
	{
		UE_LOG(LogTemp, Verbose, TEXT("HoloCadeServerCommandProtocol: Sent response to %s"), 
			*ClientAddress->ToString(false));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("HoloCadeServerCommandProtocol: Failed to send response to %s"), 
			*ClientAddress->ToString(false));
	}
}

