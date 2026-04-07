// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "HoloCadeServerCommandProtocol.generated.h"

/**
 * Server command types that can be sent from Command Console to Server Manager
 */
UENUM(BlueprintType)
enum class EHoloCadeServerCommand : uint8
{
	None,
	StartServer,
	StopServer,
	AdvanceState,
	RetreatState,
	SetMaxPlayers,
	SetPort,
	RequestStatus,
	Shutdown
};

/**
 * Server command message structure
 */
USTRUCT(BlueprintType)
struct FHoloCadeServerCommandMessage
{
	GENERATED_BODY()

	/** Command type */
	UPROPERTY()
	EHoloCadeServerCommand Command = EHoloCadeServerCommand::None;

	/** Command parameter (JSON string for complex data) */
	UPROPERTY()
	FString Parameter;

	/** Command timestamp */
	UPROPERTY()
	float Timestamp = 0.0f;

	/** Sequence number for reliability tracking (UDP) */
	UPROPERTY()
	uint32 SequenceNumber = 0;

	/** Authentication token (HMAC or shared secret, only used if authentication enabled) */
	UPROPERTY()
	FString AuthToken;

	FHoloCadeServerCommandMessage()
		: Command(EHoloCadeServerCommand::None)
		, Timestamp(0.0f)
		, SequenceNumber(0)
	{}

	FHoloCadeServerCommandMessage(EHoloCadeServerCommand InCommand, const FString& InParameter = TEXT(""), uint32 InSequenceNumber = 0)
		: Command(InCommand)
		, Parameter(InParameter)
		, Timestamp(FPlatformTime::Seconds())
		, SequenceNumber(InSequenceNumber)
	{}
};

/**
 * Server response message structure
 */
USTRUCT(BlueprintType)
struct FHoloCadeServerResponseMessage
{
	GENERATED_BODY()

	/** Success flag */
	UPROPERTY()
	bool bSuccess = false;

	/** Response message */
	UPROPERTY()
	FString Message;

	/** Response data (JSON string for complex data) */
	UPROPERTY()
	FString Data;

	FHoloCadeServerResponseMessage()
		: bSuccess(false)
	{}

	FHoloCadeServerResponseMessage(bool bInSuccess, const FString& InMessage, const FString& InData = TEXT(""))
		: bSuccess(bInSuccess)
		, Message(InMessage)
		, Data(InData)
	{}
};

/**
 * HoloCade Server Command Protocol
 * 
 * UDP-based command protocol for remote server control.
 * Allows Command Console to send commands to Server Manager over network.
 * 
 * CLIENT MODE (Command Console):
 * - Sends commands to Server Manager via UDP
 * - Sends commands (start/stop, state changes, etc.)
 * - Optionally receives responses
 * 
 * SERVER MODE (Server Manager):
 * - Listens for incoming command packets on UDP
 * - Receives and processes commands
 * - Sends responses back
 * 
 * Protocol:
 * - UDP packets on port 7779 (default)
 * - Messages are JSON-serialized
 * - Commands include sequence numbers for reliability
 * - Responses are optional (fire-and-forget or request-response)
 * 
 * Note: Consistent with HoloCade architecture (all networking is UDP-based:
 *       Server Beacon on 7778, Embedded Systems on 8888, Commands on 7779)
 */
UCLASS(BlueprintType)
class HOLOCADECORE_API UHoloCadeServerCommandProtocol : public UObject
{
	GENERATED_BODY()

public:
	UHoloCadeServerCommandProtocol();
	virtual ~UHoloCadeServerCommandProtocol();

	/** Command port (default: 7779, separate from game port 7777 and beacon port 7778) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Networking")
	int32 CommandPort = 7779;

	/** Enable authentication for remote connections (not needed for local same-desk setups) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Networking|Security")
	bool bEnableAuthentication = false;

	/** Shared secret for authentication (must match between client and server) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Networking|Security", meta = (EditCondition = "bEnableAuthentication", PasswordField = true))
	FString SharedSecret = TEXT("CHANGE_ME_IN_PRODUCTION");

	/** Is currently sending commands? (client mode) */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Networking")
	bool IsActive() const { return bIsActive && CommandSocket != nullptr; }

	/** Is currently listening (server mode)? */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Networking")
	bool IsListening() const { return bIsListening && ListenSocket != nullptr; }

	/**
	 * CLIENT MODE: Initialize connection to remote Server Manager
	 * @param ServerIP - IP address of the server
	 * @param ServerPort - Port of the server (default: CommandPort)
	 * @return True if initialization successful
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Networking")
	bool InitializeClient(const FString& ServerIP, int32 ServerPort = 7779);

	/**
	 * CLIENT MODE: Shutdown client mode
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Networking")
	void ShutdownClient();

	/**
	 * CLIENT MODE: Send a command to the server
	 * @param Command - Command to send
	 * @param Parameter - Optional parameter (JSON string)
	 * @return Response from server
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Networking")
	FHoloCadeServerResponseMessage SendCommand(EHoloCadeServerCommand Command, const FString& Parameter = TEXT(""));

	/**
	 * SERVER MODE: Send a response back to the client
	 * @param Response - Response message to send
	 * @param ClientAddress - Address to send response to
	 */
	void SendResponse(const FHoloCadeServerResponseMessage& Response, TSharedRef<FInternetAddr> ClientAddress);

	/**
	 * SERVER MODE: Start listening for incoming command connections
	 * @return True if listening started successfully
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Networking")
	bool StartListening();

	/**
	 * SERVER MODE: Stop listening for connections
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Networking")
	void StopListening();

	/**
	 * SERVER MODE: Process incoming command packets
	 * Call this from Tick() in server mode
	 */
	void Tick(float DeltaTime);

	/**
	 * CLIENT MODE: Process incoming response packets
	 * Call this from Tick() in client mode
	 */
	void TickClient(float DeltaTime);

	/** Delegate for received commands (server mode) */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCommandReceived, const FHoloCadeServerCommandMessage&, Command, class UHoloCadeServerCommandProtocol*, Protocol);
	UPROPERTY(BlueprintAssignable, Category = "HoloCade|Networking")
	FOnCommandReceived OnCommandReceived;

	/**
	 * Get the last sender address (for sending responses)
	 * Only valid immediately after OnCommandReceived fires
	 */
	TSharedPtr<FInternetAddr> GetLastSenderAddress() const { return LastSenderAddress; }

	/** Delegate for client events */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnClientEvent, const FString&, Message);
	UPROPERTY(BlueprintAssignable, Category = "HoloCade|Networking")
	FOnClientEvent OnClientInitialized;

	UPROPERTY(BlueprintAssignable, Category = "HoloCade|Networking")
	FOnClientEvent OnClientShutdown;

	UPROPERTY(BlueprintAssignable, Category = "HoloCade|Networking")
	FOnClientEvent OnClientError;

	/** Delegate for server events */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnServerEvent, const FString&, Message);
	UPROPERTY(BlueprintAssignable, Category = "HoloCade|Networking")
	FOnServerEvent OnServerStarted;

	UPROPERTY(BlueprintAssignable, Category = "HoloCade|Networking")
	FOnServerEvent OnServerStopped;

protected:
	/** Client socket (for sending commands to server) */
	FSocket* CommandSocket = nullptr;

	/** Server listen socket (for receiving commands) */
	FSocket* ListenSocket = nullptr;

	/** Remote server address (client mode) */
	TSharedPtr<FInternetAddr> RemoteServerAddr;

	/** Is currently active (client mode) */
	bool bIsActive = false;

	/** Is currently listening (server mode) */
	bool bIsListening = false;

	/** Server IP address (when active as client) */
	FString TargetServerIP;

	/** Server port (when active as client) */
	int32 TargetServerPort = 0;

	/** Sequence number for command reliability (client mode) */
	uint32 NextSequenceNumber = 0;

	/** Last sender address (for sending responses to commands) */
	TSharedPtr<FInternetAddr> LastSenderAddress;

	/** Create UDP socket for sending commands (client mode) */
	bool CreateClientSocket();

	/** Create UDP socket for receiving commands (server mode) */
	bool CreateListenSocket();

	/** Process incoming command packets (server mode) */
	void ProcessIncomingCommands();

	/** Serialize command message to JSON */
	FString SerializeCommand(const FHoloCadeServerCommandMessage& Command) const;

	/** Deserialize command message from JSON */
	bool DeserializeCommand(const FString& JsonString, FHoloCadeServerCommandMessage& OutCommand) const;

	/** Serialize response message to JSON */
	FString SerializeResponse(const FHoloCadeServerResponseMessage& Response) const;

	/** Deserialize response message from JSON */
	bool DeserializeResponse(const FString& JsonString, FHoloCadeServerResponseMessage& OutResponse) const;

	/** Generate authentication token from command data */
	FString GenerateAuthToken(const FHoloCadeServerCommandMessage& Command) const;

	/** Validate authentication token */
	bool ValidateAuthToken(const FHoloCadeServerCommandMessage& Command) const;

	/** Send data via UDP socket */
	bool SendUDPData(FSocket* Socket, const TArray<uint8>& Data, TSharedRef<FInternetAddr> Address);

	/** Receive data from UDP socket (non-blocking) */
	bool ReceiveUDPData(FSocket* Socket, TArray<uint8>& OutData, TSharedPtr<FInternetAddr>& OutSender);

	/** Cleanup sockets */
	void CleanupSockets();

	/** Cleanup a specific socket */
	void CleanupSocket(FSocket*& Socket);
};

