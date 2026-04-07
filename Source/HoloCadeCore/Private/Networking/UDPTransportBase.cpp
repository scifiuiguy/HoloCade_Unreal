// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "Networking/UDPTransportBase.h"
#include "IPAddress.h"

FUDPTransportBase::FUDPTransportBase()
{
}

FUDPTransportBase::~FUDPTransportBase()
{
	ShutdownUDPConnection();
}

bool FUDPTransportBase::InitializeUDPConnection(const FString& RemoteIP, int32 RemotePort, const FString& SocketName, bool bEnableBroadcast)
{
	// Get socket subsystem
	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (!SocketSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("UDPTransportBase: Failed to get socket subsystem"));
		return false;
	}

	// Create UDP socket
	UDPSocket = SocketSubsystem->CreateSocket(NAME_DGram, *SocketName, false);
	if (!UDPSocket)
	{
		UE_LOG(LogTemp, Error, TEXT("UDPTransportBase: Failed to create UDP socket"));
		return false;
	}

	// Make socket non-blocking
	UDPSocket->SetNonBlocking(true);

	// Set receive buffer size (8KB should be plenty for most hardware data)
	int32 NewSize = 8192;
	UDPSocket->SetReceiveBufferSize(NewSize, NewSize);

	// Enable broadcast if requested (for Art-Net, etc.)
	if (bEnableBroadcast)
	{
		UDPSocket->SetBroadcast(true);
	}

	// Parse and store remote address
	RemoteAddress = SocketSubsystem->CreateInternetAddr();
	bool bIsValid = false;
	RemoteAddress->SetIp(*RemoteIP, bIsValid);
	RemoteAddress->SetPort(RemotePort);

	if (!bIsValid)
	{
		UE_LOG(LogTemp, Error, TEXT("UDPTransportBase: Invalid IP address: %s"), *RemoteIP);
		SocketSubsystem->DestroySocket(UDPSocket);
		UDPSocket = nullptr;
		RemoteAddress.Reset();
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("UDPTransportBase: UDP socket created successfully (%s:%d)"), *RemoteIP, RemotePort);
	return true;
}

void FUDPTransportBase::ShutdownUDPConnection()
{
	if (UDPSocket)
	{
		UDPSocket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(UDPSocket);
		UDPSocket = nullptr;
		RemoteAddress.Reset();
		UE_LOG(LogTemp, Log, TEXT("UDPTransportBase: UDP connection closed"));
	}
}

bool FUDPTransportBase::SendUDPData(const TArray<uint8>& Data)
{
	if (!UDPSocket || !RemoteAddress.IsValid())
	{
		return false;
	}

	int32 BytesSent = 0;
	bool bSuccess = UDPSocket->SendTo(Data.GetData(), Data.Num(), BytesSent, *RemoteAddress);

	if (!bSuccess || BytesSent != Data.Num())
	{
		UE_LOG(LogTemp, Warning, TEXT("UDPTransportBase: Failed to send %d bytes (sent: %d)"), 
			Data.Num(), BytesSent);
		return false;
	}

	return true;
}

bool FUDPTransportBase::ReceiveUDPData(TArray<uint8>& OutData, int32& OutBytesRead, TSharedPtr<FInternetAddr>* OutSenderAddr)
{
	if (!UDPSocket)
	{
		return false;
	}

	// Check if data is available
	uint32 PendingDataSize = 0;
	if (!UDPSocket->HasPendingData(PendingDataSize) || PendingDataSize == 0)
	{
		return false;
	}

	// Read data
	OutData.SetNumUninitialized(PendingDataSize);
	OutBytesRead = 0;

	TSharedRef<FInternetAddr> SenderAddr = 
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();

	bool bSuccess = UDPSocket->RecvFrom(OutData.GetData(), OutData.Num(), OutBytesRead, *SenderAddr);

	if (bSuccess && OutBytesRead > 0)
	{
		// Resize output array to actual bytes read
		OutData.SetNum(OutBytesRead);

		// Optionally return sender address
		if (OutSenderAddr)
		{
			*OutSenderAddr = SenderAddr;
		}

		return true;
	}

	return false;
}

bool FUDPTransportBase::HasPendingData(uint32& OutPendingSize) const
{
	if (!UDPSocket)
	{
		return false;
	}

	return UDPSocket->HasPendingData(OutPendingSize);
}



