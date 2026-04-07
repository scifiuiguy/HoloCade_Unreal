// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "Networking/HoloCadeUDPTransport.h"
#include "IPAddress.h"

UHoloCadeUDPTransport::UHoloCadeUDPTransport()
{
	PrimaryComponentTick.bCanEverTick = true;
}

UHoloCadeUDPTransport::~UHoloCadeUDPTransport()
{
	ShutdownUDPConnection();
}

void UHoloCadeUDPTransport::BeginPlay()
{
	Super::BeginPlay();
}

void UHoloCadeUDPTransport::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ShutdownUDPConnection();
	Super::EndPlay(EndPlayReason);
}

void UHoloCadeUDPTransport::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (IsUDPConnected())
	{
		ProcessIncomingUDPData();
	}
}

bool UHoloCadeUDPTransport::InitializeUDPConnection(const FString& RemoteIP, int32 RemotePort, const FString& SocketName)
{
	UE_LOG(LogTemp, Log, TEXT("HoloCadeUDPTransport: Initializing UDP connection to %s:%d"), *RemoteIP, RemotePort);

	// Use base transport for socket management
	return UDPTransport.InitializeUDPConnection(RemoteIP, RemotePort, SocketName, false);
}

void UHoloCadeUDPTransport::ShutdownUDPConnection()
{
	UDPTransport.ShutdownUDPConnection();

	ReceivedFloatCache.Empty();
	ReceivedBoolCache.Empty();
	ReceivedInt32Cache.Empty();
	ReceivedBytesCache.Empty();
}

// =====================================
// Channel-Based Send API Implementation
// =====================================

void UHoloCadeUDPTransport::SendFloat(int32 Channel, float Value)
{
	if (!IsUDPConnected())
	{
		UE_LOG(LogTemp, Warning, TEXT("HoloCadeUDPTransport: Cannot send float - UDP not connected"));
		return;
	}

	TArray<uint8> Payload;
	Payload.SetNumUninitialized(4);
	// Little-endian float (reinterpret as uint32)
	uint32 IntValue = *reinterpret_cast<uint32*>(&Value);
	Payload[0] = (IntValue) & 0xFF;
	Payload[1] = (IntValue >> 8) & 0xFF;
	Payload[2] = (IntValue >> 16) & 0xFF;
	Payload[3] = (IntValue >> 24) & 0xFF;

	TArray<uint8> Packet = BuildBinaryPacket(EHoloCadeUDPDataType::Float, Channel, Payload);
	SendUDPData(Packet);
}

void UHoloCadeUDPTransport::SendBool(int32 Channel, bool Value)
{
	if (!IsUDPConnected())
	{
		UE_LOG(LogTemp, Warning, TEXT("HoloCadeUDPTransport: Cannot send bool - UDP not connected"));
		return;
	}

	TArray<uint8> Payload;
	Payload.Add(Value ? 1 : 0);

	TArray<uint8> Packet = BuildBinaryPacket(EHoloCadeUDPDataType::Bool, Channel, Payload);
	SendUDPData(Packet);
}

void UHoloCadeUDPTransport::SendInt32(int32 Channel, int32 Value)
{
	if (!IsUDPConnected())
	{
		UE_LOG(LogTemp, Warning, TEXT("HoloCadeUDPTransport: Cannot send int32 - UDP not connected"));
		return;
	}

	TArray<uint8> Payload;
	Payload.SetNumUninitialized(4);
	// Little-endian int32
	Payload[0] = (Value) & 0xFF;
	Payload[1] = (Value >> 8) & 0xFF;
	Payload[2] = (Value >> 16) & 0xFF;
	Payload[3] = (Value >> 24) & 0xFF;

	TArray<uint8> Packet = BuildBinaryPacket(EHoloCadeUDPDataType::Int32, Channel, Payload);
	SendUDPData(Packet);
}

void UHoloCadeUDPTransport::SendString(int32 Channel, const FString& Value)
{
	if (!IsUDPConnected())
	{
		UE_LOG(LogTemp, Warning, TEXT("HoloCadeUDPTransport: Cannot send string - UDP not connected"));
		return;
	}

	// Convert string to UTF-8 bytes
	FTCHARToUTF8 Converter(*Value);
	int32 StrLength = FMath::Min(Converter.Length(), 255); // Max 255 bytes

	TArray<uint8> Payload;
	Payload.Add((uint8)StrLength);
	Payload.Append((uint8*)Converter.Get(), StrLength);

	TArray<uint8> Packet = BuildBinaryPacket(EHoloCadeUDPDataType::String, Channel, Payload);
	SendUDPData(Packet);
}

void UHoloCadeUDPTransport::SendBytes(int32 Channel, const TArray<uint8>& Data)
{
	if (!IsUDPConnected())
	{
		UE_LOG(LogTemp, Warning, TEXT("HoloCadeUDPTransport: Cannot send bytes - UDP not connected"));
		return;
	}

	int32 DataLength = FMath::Min(Data.Num(), 255); // Max 255 bytes

	TArray<uint8> Payload;
	Payload.Add((uint8)DataLength);
	Payload.Append(Data.GetData(), DataLength);

	TArray<uint8> Packet = BuildBinaryPacket(EHoloCadeUDPDataType::Bytes, Channel, Payload);
	SendUDPData(Packet);
}

// =====================================
// Channel-Based Receive API Implementation
// =====================================

float UHoloCadeUDPTransport::GetReceivedFloat(int32 Channel) const
{
	const float* Value = ReceivedFloatCache.Find(Channel);
	return Value ? *Value : 0.0f;
}

bool UHoloCadeUDPTransport::GetReceivedBool(int32 Channel) const
{
	const bool* Value = ReceivedBoolCache.Find(Channel);
	return Value ? *Value : false;
}

int32 UHoloCadeUDPTransport::GetReceivedInt32(int32 Channel) const
{
	const int32* Value = ReceivedInt32Cache.Find(Channel);
	return Value ? *Value : 0;
}

TArray<uint8> UHoloCadeUDPTransport::GetReceivedBytes(int32 Channel) const
{
	const TArray<uint8>* Value = ReceivedBytesCache.Find(Channel);
	return Value ? *Value : TArray<uint8>();
}

// =====================================
// UDP Communication Implementation
// =====================================

void UHoloCadeUDPTransport::SendUDPData(const TArray<uint8>& Data)
{
	UDPTransport.SendUDPData(Data);
}

void UHoloCadeUDPTransport::ReceiveUDPData()
{
	TArray<uint8> ReceivedData;
	int32 BytesRead = 0;
	TSharedPtr<FInternetAddr> SenderAddr;

	if (UDPTransport.ReceiveUDPData(ReceivedData, BytesRead, &SenderAddr))
	{
		UE_LOG(LogTemp, VeryVerbose, TEXT("HoloCadeUDPTransport: Received %d bytes"), BytesRead);
		ParseBinaryPacket(ReceivedData, BytesRead);
	}
}

void UHoloCadeUDPTransport::ProcessIncomingUDPData()
{
	ReceiveUDPData();
}

// =====================================
// HoloCade Binary Protocol Implementation
// =====================================

TArray<uint8> UHoloCadeUDPTransport::BuildBinaryPacket(EHoloCadeUDPDataType DataType, int32 Channel, const TArray<uint8>& Payload)
{
	TArray<uint8> Packet;

	// Simple HoloCade format: [0xAA][Type][Ch][Payload][CRC:1]
	// No security for now (can add HMAC/encryption later if needed)
	
	Packet.Add(PACKET_START_MARKER);
	Packet.Add((uint8)DataType);
	Packet.Add((uint8)Channel);
	Packet.Append(Payload);
	
	// Calculate and append CRC
	uint8 CRC = CalculateCRC(Packet, Packet.Num());
	Packet.Add(CRC);

	return Packet;
}

void UHoloCadeUDPTransport::ParseBinaryPacket(const TArray<uint8>& Data, int32 Length)
{
	// Validate start marker
	if (Length < 1 || Data[0] != PACKET_START_MARKER)
	{
		UE_LOG(LogTemp, Warning, TEXT("HoloCadeUDPTransport: Invalid start marker"));
		return;
	}

	// Simple format: [0xAA][Type][Ch][Payload][CRC:1]
	// Minimum: Marker(1) + Type(1) + Channel(1) + Payload(1) + CRC(1) = 5 bytes
	if (Length < 5)
	{
		UE_LOG(LogTemp, Warning, TEXT("HoloCadeUDPTransport: Packet too small (%d bytes)"), Length);
		return;
	}

	// Extract CRC and validate
	uint8 ReceivedCRC = Data[Length - 1];
	if (!ValidateCRC(Data, Length - 1, ReceivedCRC))
	{
		UE_LOG(LogTemp, Warning, TEXT("HoloCadeUDPTransport: CRC validation failed"));
		return;
	}

	// Parse packet
	uint8 DataType = Data[1];
	int32 Channel = Data[2];
	
	// Extract payload (bytes 3 to Length-2)
	TArray<uint8> PayloadData;
	PayloadData.Append(&Data[3], Length - 4);

	// Parse payload based on type
	switch (DataType)
	{
	case 0: // Bool
	{
		if (PayloadData.Num() < 1) return;
		bool Value = (PayloadData[0] != 0);
		ReceivedBoolCache.Add(Channel, Value);
		OnBoolReceived.Broadcast(Channel, Value);
		UE_LOG(LogTemp, Verbose, TEXT("HoloCadeUDPTransport: Bool received - Ch:%d Val:%s"), 
			Channel, Value ? TEXT("true") : TEXT("false"));
		break;
	}

	case 1: // Int32
	{
		if (PayloadData.Num() < 4) return;
		// Little-endian int32
		int32 Value = PayloadData[0] | (PayloadData[1] << 8) | (PayloadData[2] << 16) | (PayloadData[3] << 24);
		ReceivedInt32Cache.Add(Channel, Value);
		OnInt32Received.Broadcast(Channel, Value);
		UE_LOG(LogTemp, Verbose, TEXT("HoloCadeUDPTransport: Int32 received - Ch:%d Val:%d"), 
			Channel, Value);
		break;
	}

	case 2: // Float
	{
		if (PayloadData.Num() < 4) return;
		// Little-endian float (reinterpret uint32 as float)
		uint32 IntValue = PayloadData[0] | (PayloadData[1] << 8) | (PayloadData[2] << 16) | (PayloadData[3] << 24);
		float Value = *reinterpret_cast<float*>(&IntValue);
		ReceivedFloatCache.Add(Channel, Value);
		OnFloatReceived.Broadcast(Channel, Value);
		UE_LOG(LogTemp, Verbose, TEXT("HoloCadeUDPTransport: Float received - Ch:%d Val:%.3f"), 
			Channel, Value);
		break;
	}

	case 3: // String
	{
		if (PayloadData.Num() < 1) return;
		uint8 StrLength = PayloadData[0];
		if (PayloadData.Num() < 1 + StrLength) return;
		
		// Convert UTF-8 bytes to FString
		FUTF8ToTCHAR Converter((const ANSICHAR*)&PayloadData[1], StrLength);
		FString Value(Converter.Length(), Converter.Get());
		
		OnStringReceived.Broadcast(Channel, Value);
		UE_LOG(LogTemp, Verbose, TEXT("HoloCadeUDPTransport: String received - Ch:%d Val:%s"), 
			Channel, *Value);
		break;
	}

	case 4: // Bytes
	{
		if (PayloadData.Num() < 1) return;
		uint8 ByteLength = PayloadData[0];
		if (PayloadData.Num() < 1 + ByteLength) return;
		
		TArray<uint8> Bytes;
		Bytes.Append(&PayloadData[1], ByteLength);
		
		// Cache bytes for struct packet parsing
		ReceivedBytesCache.Add(Channel, Bytes);
		OnBytesReceived.Broadcast(Channel, Bytes);
		UE_LOG(LogTemp, Verbose, TEXT("HoloCadeUDPTransport: Bytes received - Ch:%d Len:%d"), 
			Channel, ByteLength);
		break;
	}

	default:
		UE_LOG(LogTemp, Warning, TEXT("HoloCadeUDPTransport: Unknown data type (%d)"), DataType);
		break;
	}
}

uint8 UHoloCadeUDPTransport::CalculateCRC(const TArray<uint8>& Data, int32 Length) const
{
	uint8 CRC = 0;
	for (int32 i = 0; i < Length; i++)
	{
		CRC ^= Data[i];
	}
	return CRC;
}

bool UHoloCadeUDPTransport::ValidateCRC(const TArray<uint8>& Data, int32 Length, uint8 ExpectedCRC) const
{
	uint8 CalculatedCRC = CalculateCRC(Data, Length);
	return (CalculatedCRC == ExpectedCRC);
}

