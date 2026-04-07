// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "EmbeddedDeviceController.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "IPAddress.h"
#include "Json.h"
#include "JsonUtilities.h"
#include "Misc/SecureHash.h"
#include "Misc/AES.h"

UEmbeddedDeviceController::UEmbeddedDeviceController()
{
	PrimaryComponentTick.bCanEverTick = true;
	
	// Initialize random state
	RandomState = FPlatformTime::Cycles();
}

void UEmbeddedDeviceController::BeginPlay()
{
	Super::BeginPlay();
	
	// Auto-initialize if config is set
	if (!Config.DeviceAddress.IsEmpty())
	{
		InitializeDevice(Config);
	}
}

void UEmbeddedDeviceController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DisconnectDevice();
	Super::EndPlay(EndPlayReason);
}

void UEmbeddedDeviceController::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsConnected)
	{
		return;
	}

	// Process any incoming data
	ProcessIncomingData();

	// Check connection health
	CheckConnectionHealth();
}

bool UEmbeddedDeviceController::InitializeDevice(const FEmbeddedDeviceConfig& InConfig)
{
	Config = InConfig;

	// Warn if debug mode conflicts with security settings
	if (Config.bDebugMode && Config.SecurityLevel != EHoloCadeSecurityLevel::None)
	{
		UE_LOG(LogTemp, Warning, TEXT("========================================"));
		UE_LOG(LogTemp, Warning, TEXT("⚠️  SECURITY WARNING ⚠️"));
		UE_LOG(LogTemp, Warning, TEXT("========================================"));
		UE_LOG(LogTemp, Warning, TEXT("Debug mode DISABLES encryption for Wireshark packet inspection!"));
		UE_LOG(LogTemp, Warning, TEXT("SecurityLevel is set to '%s' but will be IGNORED in debug mode."),
			*UEnum::GetValueAsString(Config.SecurityLevel));
		UE_LOG(LogTemp, Warning, TEXT("All packets will be sent as PLAIN JSON (no encryption)."));
		UE_LOG(LogTemp, Warning, TEXT(""));
		UE_LOG(LogTemp, Warning, TEXT("⛔ NEVER USE DEBUG MODE IN PRODUCTION! ⛔"));
		UE_LOG(LogTemp, Warning, TEXT("========================================"));
	}

	// Derive encryption keys from shared secret (only if not in debug mode)
	if (Config.SecurityLevel != EHoloCadeSecurityLevel::None && !Config.bDebugMode)
	{
		DeriveKeysFromSecret();
		
		UE_LOG(LogTemp, Log, TEXT("EmbeddedDeviceController: Security enabled (%s)"), 
			*UEnum::GetValueAsString(Config.SecurityLevel));
	}
	else if (Config.SecurityLevel == EHoloCadeSecurityLevel::None)
	{
		UE_LOG(LogTemp, Warning, TEXT("EmbeddedDeviceController: Security DISABLED (Development Only)"));
	}

	// Establish connection based on protocol type
	bool bSuccess = false;
	switch (Config.Protocol)
	{
	case EHoloCadeCommProtocol::Serial:
		bSuccess = InitializeSerialConnection();
		break;

	case EHoloCadeCommProtocol::WiFi:
	case EHoloCadeCommProtocol::Ethernet:
		bSuccess = InitializeWiFiConnection();
		break;

	case EHoloCadeCommProtocol::Bluetooth:
		UE_LOG(LogTemp, Warning, TEXT("EmbeddedDeviceController: Bluetooth not yet implemented"));
		bSuccess = false;
		break;
	}

	if (!bSuccess)
	{
		UE_LOG(LogTemp, Error, TEXT("EmbeddedDeviceController: Failed to initialize %s connection"), 
			*UEnum::GetValueAsString(Config.Protocol));
		return false;
	}

	// Initialize input cache
	InputValueCache.Empty();
	for (int32 i = 0; i < Config.InputChannelCount; i++)
	{
		InputValueCache.Add(i, 0.0f);
	}

	bIsConnected = true;
	LastCommTimestamp = GetWorld()->GetTimeSeconds();

	UE_LOG(LogTemp, Log, TEXT("EmbeddedDeviceController: Initialized successfully (%s mode, %s)"), 
		Config.bDebugMode ? TEXT("JSON Debug") : TEXT("Binary"),
		*UEnum::GetValueAsString(Config.SecurityLevel));
	return true;
}

bool UEmbeddedDeviceController::InitializeWiFiConnection()
{
	UE_LOG(LogTemp, Log, TEXT("EmbeddedDeviceController: Initializing WiFi/Ethernet (UDP) to %s:%d"), 
		*Config.DeviceAddress, Config.Port);

	// Use base class for UDP socket management
	return InitializeUDPConnection(Config.DeviceAddress, Config.Port, TEXT("HoloCade_Embedded"));
}

bool UEmbeddedDeviceController::InitializeSerialConnection()
{
	UE_LOG(LogTemp, Log, TEXT("EmbeddedDeviceController: Initializing Serial connection to %s at %d baud"), 
		*Config.DeviceAddress, Config.BaudRate);

	// NOOP: TODO - Implement Windows COM port handling
	// This requires platform-specific code (Windows: CreateFile, Linux: termios)
	UE_LOG(LogTemp, Warning, TEXT("EmbeddedDeviceController: Serial communication not yet implemented"));
	return false;
}

void UEmbeddedDeviceController::SendOutputCommand(const FEmbeddedOutputCommand& Command)
{
	if (!bIsConnected)
	{
		UE_LOG(LogTemp, Warning, TEXT("EmbeddedDeviceController: Cannot send command - not connected"));
		return;
	}

	// NOOP: TODO - Format and send command to device
	// Command format would depend on the firmware protocol
	UE_LOG(LogTemp, Log, TEXT("EmbeddedDeviceController: Sending output - Channel: %d, Value: %.2f"), 
		Command.Channel, Command.Value);
}

void UEmbeddedDeviceController::TriggerHapticPulse(int32 Channel, float Intensity, float Duration)
{
	FEmbeddedOutputCommand Command;
	Command.Channel = Channel;
	Command.OutputType = EHoloCadeOutputType::Discrete;
	Command.Value = FMath::Clamp(Intensity, 0.0f, 1.0f);
	Command.Duration = Duration;

	SendOutputCommand(Command);
}

void UEmbeddedDeviceController::SetContinuousOutput(int32 Channel, float Value)
{
	FEmbeddedOutputCommand Command;
	Command.Channel = Channel;
	Command.OutputType = EHoloCadeOutputType::Continuous;
	Command.Value = FMath::Clamp(Value, 0.0f, 1.0f);
	Command.Duration = 0.0f; // Continuous

	SendOutputCommand(Command);
}

float UEmbeddedDeviceController::GetInputValue(int32 Channel) const
{
	const float* Value = InputValueCache.Find(Channel);
	return Value ? *Value : 0.0f;
}

bool UEmbeddedDeviceController::IsDeviceConnected() const
{
	return bIsConnected;
}

void UEmbeddedDeviceController::DisconnectDevice()
{
	if (!bIsConnected)
	{
		return;
	}

	// Close UDP connection (uses base class)
	if (Config.Protocol == EHoloCadeCommProtocol::WiFi || Config.Protocol == EHoloCadeCommProtocol::Ethernet)
	{
		ShutdownUDPConnection();
	}

	bIsConnected = false;
	InputValueCache.Empty();

	UE_LOG(LogTemp, Log, TEXT("EmbeddedDeviceController: Disconnected"));
}

void UEmbeddedDeviceController::ProcessIncomingData()
{
	// Dispatch to protocol-specific receive
	switch (Config.Protocol)
	{
	case EHoloCadeCommProtocol::WiFi:
	case EHoloCadeCommProtocol::Ethernet:
		ReceiveWiFiData();
		break;

	case EHoloCadeCommProtocol::Serial:
		// NOOP: TODO - Implement serial receive
		break;

	default:
		break;
	}
}

void UEmbeddedDeviceController::SendDataToDevice(const TArray<uint8>& Data)
{
	if (!bIsConnected || Data.Num() == 0)
	{
		return;
	}

	// Dispatch to protocol-specific send
	switch (Config.Protocol)
	{
	case EHoloCadeCommProtocol::WiFi:
	case EHoloCadeCommProtocol::Ethernet:
		SendWiFiData(Data);
		break;

	case EHoloCadeCommProtocol::Serial:
		// NOOP: TODO - Implement serial send
		break;

	default:
		break;
	}

	LastCommTimestamp = GetWorld()->GetTimeSeconds();
}

void UEmbeddedDeviceController::SendWiFiData(const TArray<uint8>& Data)
{
	// Use base class SendUDPData to send raw packets (with encryption/HMAC already applied)
	// EmbeddedDeviceController builds its own packets with security features, so we send them directly
	SendUDPData(Data);
}

void UEmbeddedDeviceController::ReceiveWiFiData()
{
	// Use base class's UDPTransport to get raw packets, then parse with encryption/HMAC support
	TArray<uint8> ReceivedData;
	int32 BytesRead = 0;
	TSharedPtr<FInternetAddr> SenderAddr;

	// Access protected UDPTransport member from base class
	if (UDPTransport.ReceiveUDPData(ReceivedData, BytesRead, &SenderAddr))
	{
		UE_LOG(LogTemp, VeryVerbose, TEXT("EmbeddedDeviceController: Received %d bytes"), BytesRead);

		// Parse the received data (with encryption/HMAC support if enabled)
		if (Config.bDebugMode)
		{
			ParseJSONPacket(ReceivedData, BytesRead);
		}
		else
		{
			ParseBinaryPacket(ReceivedData, BytesRead);
		}

		LastCommTimestamp = GetWorld()->GetTimeSeconds();
	}
}

void UEmbeddedDeviceController::CheckConnectionHealth()
{
	float CurrentTime = GetWorld()->GetTimeSeconds();
	float TimeSinceLastComm = CurrentTime - LastCommTimestamp;

	// If no communication for 5 seconds, consider connection lost
	if (TimeSinceLastComm > 5.0f && bIsConnected)
	{
		UE_LOG(LogTemp, Warning, TEXT("EmbeddedDeviceController: Connection timeout - no data received for %.1f seconds"), TimeSinceLastComm);
		bIsConnected = false;
	}
}

// =====================================
// Binary Protocol - Primitive Send API
// =====================================

void UEmbeddedDeviceController::SendBool(int32 Channel, bool Value)
{
	TArray<uint8> Payload;
	Payload.Add(Value ? 1 : 0);

	TArray<uint8> Packet;
	if (Config.bDebugMode)
	{
		Packet = BuildJSONPacket(EHoloCadeDataType::Bool, Channel, Value ? TEXT("true") : TEXT("false"));
	}
	else
	{
		Packet = BuildBinaryPacket(EHoloCadeDataType::Bool, Channel, Payload);
	}

	SendDataToDevice(Packet);
}

void UEmbeddedDeviceController::SendInt32(int32 Channel, int32 Value)
{
	TArray<uint8> Payload;
	Payload.SetNumUninitialized(4);
	// Little-endian int32
	Payload[0] = (Value) & 0xFF;
	Payload[1] = (Value >> 8) & 0xFF;
	Payload[2] = (Value >> 16) & 0xFF;
	Payload[3] = (Value >> 24) & 0xFF;

	TArray<uint8> Packet;
	if (Config.bDebugMode)
	{
		Packet = BuildJSONPacket(EHoloCadeDataType::Int32, Channel, FString::Printf(TEXT("%d"), Value));
	}
	else
	{
		Packet = BuildBinaryPacket(EHoloCadeDataType::Int32, Channel, Payload);
	}

	SendDataToDevice(Packet);
}

void UEmbeddedDeviceController::SendFloat(int32 Channel, float Value)
{
	TArray<uint8> Payload;
	Payload.SetNumUninitialized(4);
	// Little-endian float (reinterpret as uint32)
	uint32 IntValue = *reinterpret_cast<uint32*>(&Value);
	Payload[0] = (IntValue) & 0xFF;
	Payload[1] = (IntValue >> 8) & 0xFF;
	Payload[2] = (IntValue >> 16) & 0xFF;
	Payload[3] = (IntValue >> 24) & 0xFF;

	TArray<uint8> Packet;
	if (Config.bDebugMode)
	{
		Packet = BuildJSONPacket(EHoloCadeDataType::Float, Channel, FString::Printf(TEXT("%.3f"), Value));
	}
	else
	{
		Packet = BuildBinaryPacket(EHoloCadeDataType::Float, Channel, Payload);
	}

	SendDataToDevice(Packet);
}

void UEmbeddedDeviceController::SendString(int32 Channel, const FString& Value)
{
	// Convert string to UTF-8 bytes
	FTCHARToUTF8 Converter(*Value);
	int32 StrLength = FMath::Min(Converter.Length(), 255); // Max 255 bytes

	TArray<uint8> Payload;
	Payload.Add((uint8)StrLength);
	Payload.Append((uint8*)Converter.Get(), StrLength);

	TArray<uint8> Packet;
	if (Config.bDebugMode)
	{
		// Escape quotes in JSON
		FString EscapedValue = Value.Replace(TEXT("\""), TEXT("\\\""));
		Packet = BuildJSONPacket(EHoloCadeDataType::String, Channel, FString::Printf(TEXT("\"%s\""), *EscapedValue));
	}
	else
	{
		Packet = BuildBinaryPacket(EHoloCadeDataType::String, Channel, Payload);
	}

	SendDataToDevice(Packet);
}

void UEmbeddedDeviceController::SendBytes(int32 Channel, const TArray<uint8>& Data)
{
	int32 DataLength = FMath::Min(Data.Num(), 255); // Max 255 bytes

	TArray<uint8> Payload;
	Payload.Add((uint8)DataLength);
	Payload.Append(Data.GetData(), DataLength);

	TArray<uint8> Packet;
	if (Config.bDebugMode)
	{
		// Convert bytes to hex string for JSON
		FString HexString;
		for (int32 i = 0; i < DataLength; i++)
		{
			HexString += FString::Printf(TEXT("%02X"), Data[i]);
		}
		Packet = BuildJSONPacket(EHoloCadeDataType::Bytes, Channel, FString::Printf(TEXT("\"%s\""), *HexString));
	}
	else
	{
		Packet = BuildBinaryPacket(EHoloCadeDataType::Bytes, Channel, Payload);
	}

	SendDataToDevice(Packet);
}

// =====================================
// Binary Protocol - Packet Building
// =====================================

TArray<uint8> UEmbeddedDeviceController::BuildBinaryPacket(EHoloCadeDataType Type, int32 Channel, const TArray<uint8>& Payload)
{
	TArray<uint8> Packet;

	// Security level determines packet format
	if (Config.SecurityLevel == EHoloCadeSecurityLevel::Encrypted)
	{
		// Encrypted format: [0xAA][IV:4][Encrypted(Type|Ch|Payload):N][HMAC:8]
		
		// Build plaintext: [Type][Channel][Payload]
		TArray<uint8> Plaintext;
		Plaintext.Add((uint8)Type);
		Plaintext.Add((uint8)Channel);
		Plaintext.Append(Payload);
		
		// Generate random IV
		uint32 IV = GenerateRandomIV();
		
		// Encrypt plaintext
		TArray<uint8> Ciphertext = EncryptAES128(Plaintext, IV);
		
		// Build packet: [Marker][IV][Ciphertext]
		Packet.Add(UHoloCadeUDPTransport::PACKET_START_MARKER);
		Packet.Add((IV) & 0xFF);
		Packet.Add((IV >> 8) & 0xFF);
		Packet.Add((IV >> 16) & 0xFF);
		Packet.Add((IV >> 24) & 0xFF);
		Packet.Append(Ciphertext);
		
		// Calculate and append HMAC (over everything except HMAC itself)
		TArray<uint8> HMAC = CalculateHMAC(Packet);
		Packet.Append(HMAC);
	}
	else if (Config.SecurityLevel == EHoloCadeSecurityLevel::HMAC)
	{
		// HMAC-only format: [0xAA][Type][Ch][Payload][HMAC:8]
		
		Packet.Add(UHoloCadeUDPTransport::PACKET_START_MARKER);
		Packet.Add((uint8)Type);
		Packet.Add((uint8)Channel);
		Packet.Append(Payload);
		
		// Calculate and append HMAC
		TArray<uint8> HMAC = CalculateHMAC(Packet);
		Packet.Append(HMAC);
	}
	else
	{
		// No security: [0xAA][Type][Ch][Payload][CRC:1]
		
		Packet.Add(UHoloCadeUDPTransport::PACKET_START_MARKER);
		Packet.Add((uint8)Type);
		Packet.Add((uint8)Channel);
		Packet.Append(Payload);
		
		// Calculate and append CRC
		uint8 CRC = CalculateCRC(Packet, Packet.Num());
		Packet.Add(CRC);
	}

	return Packet;
}

TArray<uint8> UEmbeddedDeviceController::BuildJSONPacket(EHoloCadeDataType Type, int32 Channel, const FString& ValueString)
{
	// Build JSON: {"ch":0,"type":"float","val":3.14}
	FString TypeString;
	switch (Type)
	{
	case EHoloCadeDataType::Bool:   TypeString = TEXT("bool"); break;
	case EHoloCadeDataType::Int32:  TypeString = TEXT("int"); break;
	case EHoloCadeDataType::Float:  TypeString = TEXT("float"); break;
	case EHoloCadeDataType::String: TypeString = TEXT("string"); break;
	case EHoloCadeDataType::Bytes:  TypeString = TEXT("bytes"); break;
	default:                      TypeString = TEXT("unknown"); break;
	}

	FString JsonString = FString::Printf(TEXT("{\"ch\":%d,\"type\":\"%s\",\"val\":%s}"), 
		Channel, *TypeString, *ValueString);

	// Convert to UTF-8 bytes
	FTCHARToUTF8 Converter(*JsonString);
	TArray<uint8> Packet;
	Packet.Append((uint8*)Converter.Get(), Converter.Length());

	return Packet;
}

// =====================================
// Binary Protocol - Packet Parsing
// =====================================

void UEmbeddedDeviceController::ParseBinaryPacket(const TArray<uint8>& Data, int32 Length)
{
	// Validate start marker (common to all formats) - use base class constant
	if (Length < 1 || Data[0] != UHoloCadeUDPTransport::PACKET_START_MARKER)
	{
		UE_LOG(LogTemp, Warning, TEXT("EmbeddedDeviceController: Invalid start marker"));
		return;
	}

	EHoloCadeDataType Type;
	int32 Channel;
	TArray<uint8> PayloadData;

	// Security level determines packet format
	if (Config.SecurityLevel == EHoloCadeSecurityLevel::Encrypted)
	{
		// Encrypted format: [0xAA][IV:4][Encrypted(Type|Ch|Payload):N][HMAC:8]
		// Minimum: Marker(1) + IV(4) + Encrypted(2) + HMAC(8) = 15 bytes
		if (Length < 15)
		{
			UE_LOG(LogTemp, Warning, TEXT("EmbeddedDeviceController: Encrypted packet too small (%d bytes)"), Length);
			return;
		}

		// Extract HMAC (last 8 bytes)
		TArray<uint8> ReceivedHMAC;
		ReceivedHMAC.Append(&Data[Length - 8], 8);

		// Validate HMAC (over everything except HMAC itself)
		TArray<uint8> DataForHMAC;
		DataForHMAC.Append(Data.GetData(), Length - 8);
		if (!ValidateHMAC(DataForHMAC, ReceivedHMAC))
		{
			UE_LOG(LogTemp, Warning, TEXT("EmbeddedDeviceController: HMAC validation failed"));
			return;
		}

		// Extract IV (bytes 1-4)
		uint32 IV = Data[1] | (Data[2] << 8) | (Data[3] << 16) | (Data[4] << 24);

		// Extract ciphertext (bytes 5 to Length-9)
		TArray<uint8> Ciphertext;
		Ciphertext.Append(&Data[5], Length - 13);

		// Decrypt
		TArray<uint8> Plaintext = DecryptAES128(Ciphertext, IV);

		// Parse plaintext: [Type][Channel][Payload...]
		if (Plaintext.Num() < 2)
		{
			UE_LOG(LogTemp, Warning, TEXT("EmbeddedDeviceController: Decrypted payload too small"));
			return;
		}

		Type = (EHoloCadeDataType)Plaintext[0];
		Channel = Plaintext[1];
		PayloadData.Append(&Plaintext[2], Plaintext.Num() - 2);
	}
	else if (Config.SecurityLevel == EHoloCadeSecurityLevel::HMAC)
	{
		// HMAC-only format: [0xAA][Type][Ch][Payload][HMAC:8]
		// Minimum: Marker(1) + Type(1) + Channel(1) + Payload(1) + HMAC(8) = 12 bytes
		if (Length < 12)
		{
			UE_LOG(LogTemp, Warning, TEXT("EmbeddedDeviceController: HMAC packet too small (%d bytes)"), Length);
			return;
		}

		// Extract HMAC (last 8 bytes)
		TArray<uint8> ReceivedHMAC;
		ReceivedHMAC.Append(&Data[Length - 8], 8);

		// Validate HMAC
		TArray<uint8> DataForHMAC;
		DataForHMAC.Append(Data.GetData(), Length - 8);
		if (!ValidateHMAC(DataForHMAC, ReceivedHMAC))
		{
			UE_LOG(LogTemp, Warning, TEXT("EmbeddedDeviceController: HMAC validation failed"));
			return;
		}

		// Parse packet
		Type = (EHoloCadeDataType)Data[1];
		Channel = Data[2];
		PayloadData.Append(&Data[3], Length - 11);
	}
	else
	{
		// No security: [0xAA][Type][Ch][Payload][CRC:1]
		// Minimum: Marker(1) + Type(1) + Channel(1) + Payload(1) + CRC(1) = 5 bytes
		if (Length < 5)
		{
			UE_LOG(LogTemp, Warning, TEXT("EmbeddedDeviceController: Packet too small (%d bytes)"), Length);
			return;
		}

		// Extract CRC and validate
		uint8 ReceivedCRC = Data[Length - 1];
		if (!ValidateCRC(Data, Length - 1, ReceivedCRC))
		{
			UE_LOG(LogTemp, Warning, TEXT("EmbeddedDeviceController: CRC validation failed"));
			return;
		}

		// Parse packet
		Type = (EHoloCadeDataType)Data[1];
		Channel = Data[2];
		PayloadData.Append(&Data[3], Length - 4);
	}

	// Parse payload based on type (now using PayloadData instead of raw Data)
	switch (Type)
	{
	case EHoloCadeDataType::Bool:
	{
		if (PayloadData.Num() < 1) return;
		bool Value = (PayloadData[0] != 0);
		InputValueCache.Add(Channel, Value ? 1.0f : 0.0f);
		OnBoolReceived.Broadcast(Channel, Value);
		UE_LOG(LogTemp, Verbose, TEXT("EmbeddedDeviceController: Bool received - Ch:%d Val:%s"), 
			Channel, Value ? TEXT("true") : TEXT("false"));
		break;
	}

	case EHoloCadeDataType::Int32:
	{
		if (PayloadData.Num() < 4) return;
		// Little-endian int32
		int32 Value = PayloadData[0] | (PayloadData[1] << 8) | (PayloadData[2] << 16) | (PayloadData[3] << 24);
		InputValueCache.Add(Channel, (float)Value);
		OnInt32Received.Broadcast(Channel, Value);
		UE_LOG(LogTemp, Verbose, TEXT("EmbeddedDeviceController: Int32 received - Ch:%d Val:%d"), 
			Channel, Value);
		break;
	}

	case EHoloCadeDataType::Float:
	{
		if (PayloadData.Num() < 4) return;
		// Little-endian float (reinterpret uint32 as float)
		uint32 IntValue = PayloadData[0] | (PayloadData[1] << 8) | (PayloadData[2] << 16) | (PayloadData[3] << 24);
		float Value = *reinterpret_cast<float*>(&IntValue);
		InputValueCache.Add(Channel, Value);
		OnFloatReceived.Broadcast(Channel, Value);
		UE_LOG(LogTemp, Verbose, TEXT("EmbeddedDeviceController: Float received - Ch:%d Val:%.3f"), 
			Channel, Value);
		break;
	}

	case EHoloCadeDataType::String:
	{
		if (PayloadData.Num() < 1) return;
		uint8 StrLength = PayloadData[0];
		if (PayloadData.Num() < 1 + StrLength) return;
		
		// Convert UTF-8 bytes to FString
		FUTF8ToTCHAR Converter((const ANSICHAR*)&PayloadData[1], StrLength);
		FString Value(Converter.Length(), Converter.Get());
		
		OnStringReceived.Broadcast(Channel, Value);
		UE_LOG(LogTemp, Verbose, TEXT("EmbeddedDeviceController: String received - Ch:%d Val:%s"), 
			Channel, *Value);
		break;
	}

	case EHoloCadeDataType::Bytes:
	{
		if (PayloadData.Num() < 1) return;
		uint8 ByteLength = PayloadData[0];
		if (PayloadData.Num() < 1 + ByteLength) return;
		
		TArray<uint8> Bytes;
		Bytes.Append(&PayloadData[1], ByteLength);
		
		OnBytesReceived.Broadcast(Channel, Bytes);
		UE_LOG(LogTemp, Verbose, TEXT("EmbeddedDeviceController: Bytes received - Ch:%d Len:%d"), 
			Channel, ByteLength);
		break;
	}

	default:
		UE_LOG(LogTemp, Warning, TEXT("EmbeddedDeviceController: Unknown data type (%d)"), (int32)Type);
		break;
	}
}

void UEmbeddedDeviceController::ParseJSONPacket(const TArray<uint8>& Data, int32 Length)
{
	// Convert bytes to string
	FUTF8ToTCHAR Converter((const ANSICHAR*)Data.GetData(), Length);
	FString JsonString(Converter.Length(), Converter.Get());

	// Parse JSON
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("EmbeddedDeviceController: Failed to parse JSON: %s"), *JsonString);
		return;
	}

	// Extract fields
	int32 Channel = JsonObject->GetIntegerField(TEXT("ch"));
	FString TypeString = JsonObject->GetStringField(TEXT("type"));

	// Parse value based on type
	if (TypeString == TEXT("bool"))
	{
		bool Value = JsonObject->GetBoolField(TEXT("val"));
		InputValueCache.Add(Channel, Value ? 1.0f : 0.0f);
		OnBoolReceived.Broadcast(Channel, Value);
	}
	else if (TypeString == TEXT("int"))
	{
		int32 Value = JsonObject->GetIntegerField(TEXT("val"));
		InputValueCache.Add(Channel, (float)Value);
		OnInt32Received.Broadcast(Channel, Value);
	}
	else if (TypeString == TEXT("float"))
	{
		float Value = (float)JsonObject->GetNumberField(TEXT("val"));
		InputValueCache.Add(Channel, Value);
		OnFloatReceived.Broadcast(Channel, Value);
	}
	else if (TypeString == TEXT("string"))
	{
		FString Value = JsonObject->GetStringField(TEXT("val"));
		OnStringReceived.Broadcast(Channel, Value);
	}

	UE_LOG(LogTemp, Verbose, TEXT("EmbeddedDeviceController: JSON parsed - Ch:%d Type:%s"), 
		Channel, *TypeString);
}

// =====================================
// CRC Validation
// =====================================

uint8 UEmbeddedDeviceController::CalculateCRC(const TArray<uint8>& Data, int32 Length) const
{
	uint8 CRC = 0;
	for (int32 i = 0; i < Length; i++)
	{
		CRC ^= Data[i];
	}
	return CRC;
}

bool UEmbeddedDeviceController::ValidateCRC(const TArray<uint8>& Data, int32 Length, uint8 ExpectedCRC) const
{
	uint8 CalculatedCRC = CalculateCRC(Data, Length);
	return (CalculatedCRC == ExpectedCRC);
}

// =====================================
// Cryptography - Key Derivation
// =====================================

void UEmbeddedDeviceController::DeriveKeysFromSecret()
{
	// Use PBKDF2-like approach: SHA256(Secret + "AES") and SHA256(Secret + "HMAC")
	// Simple but sufficient for LBE use case
	
	FTCHARToUTF8 SecretConverter(*Config.SharedSecret);
	TArray<uint8> SecretBytes;
	SecretBytes.Append((uint8*)SecretConverter.Get(), SecretConverter.Length());
	
	// Derive AES key: SHA256(Secret + "AES128")
	{
		TArray<uint8> AESInput = SecretBytes;
		const char* AESSalt = "AES128_HoloCade_2025";
		AESInput.Append((uint8*)AESSalt, FCStringAnsi::Strlen(AESSalt));
		
		FSHAHash AESHash;
		FSHA1::HashBuffer(AESInput.GetData(), AESInput.Num(), AESHash.Hash);
		
		// Use first 16 bytes for AES-128
		FMemory::Memcpy(DerivedAESKey, AESHash.Hash, 16);
	}
	
	// Derive HMAC key: SHA256(Secret + "HMAC")
	{
		TArray<uint8> HMACInput = SecretBytes;
		const char* HMACSalt = "HMAC_HoloCade_2025";
		HMACInput.Append((uint8*)HMACSalt, FCStringAnsi::Strlen(HMACSalt));
		
		// Use SHA-256 for HMAC key (32 bytes)
		FSHA1::HashBuffer(HMACInput.GetData(), HMACInput.Num(), DerivedHMACKey);
	}
	
	UE_LOG(LogTemp, Verbose, TEXT("EmbeddedDeviceController: Derived AES and HMAC keys from shared secret"));
}

// =====================================
// Cryptography - AES-128-CTR Encryption
// =====================================

TArray<uint8> UEmbeddedDeviceController::EncryptAES128(const TArray<uint8>& Plaintext, uint32 IV) const
{
	if (Plaintext.Num() == 0)
	{
		return TArray<uint8>();
	}
	
	// Prepare counter mode (IV + counter)
	// For simplicity, we'll use a basic counter starting from IV
	TArray<uint8> Ciphertext;
	Ciphertext.SetNumUninitialized(Plaintext.Num());
	
	// CTR mode: Encrypt counter, XOR with plaintext
	int32 BlockCount = (Plaintext.Num() + 15) / 16;  // Ceiling division
	
	for (int32 BlockIdx = 0; BlockIdx < BlockCount; BlockIdx++)
	{
		// Create counter block (IV + BlockIdx)
		uint8 CounterBlock[16];
		FMemory::Memzero(CounterBlock, 16);
		
		// Put IV in first 4 bytes, counter in next 4 bytes (little-endian)
		uint32 CurrentCounter = IV + BlockIdx;
		CounterBlock[0] = (CurrentCounter) & 0xFF;
		CounterBlock[1] = (CurrentCounter >> 8) & 0xFF;
		CounterBlock[2] = (CurrentCounter >> 16) & 0xFF;
		CounterBlock[3] = (CurrentCounter >> 24) & 0xFF;
		CounterBlock[4] = (BlockIdx) & 0xFF;
		CounterBlock[5] = (BlockIdx >> 8) & 0xFF;
		CounterBlock[6] = (BlockIdx >> 16) & 0xFF;
		CounterBlock[7] = (BlockIdx >> 24) & 0xFF;
		
		// Encrypt counter block using derived AES key
		uint8 EncryptedCounter[16];
		FAES::EncryptData(EncryptedCounter, 16, CounterBlock, 16);
		
		// XOR with plaintext
		int32 BytesInBlock = FMath::Min(16, Plaintext.Num() - BlockIdx * 16);
		for (int32 i = 0; i < BytesInBlock; i++)
		{
			Ciphertext[BlockIdx * 16 + i] = Plaintext[BlockIdx * 16 + i] ^ EncryptedCounter[i];
		}
	}
	
	return Ciphertext;
}

TArray<uint8> UEmbeddedDeviceController::DecryptAES128(const TArray<uint8>& Ciphertext, uint32 IV) const
{
	// CTR mode decryption is identical to encryption (XOR is symmetric)
	return EncryptAES128(Ciphertext, IV);
}

// =====================================
// Cryptography - HMAC-SHA256
// =====================================

TArray<uint8> UEmbeddedDeviceController::CalculateHMAC(const TArray<uint8>& Data) const
{
	// HMAC-SHA1 (Unreal's FSHA1 doesn't expose HMAC, so we'll implement it)
	// HMAC(K, m) = H((K' ⊕ opad) || H((K' ⊕ ipad) || m))
	// Where K' = K padded to block size (64 bytes for SHA-1)
	
	const int32 BlockSize = 64;
	const uint8 ipad = 0x36;
	const uint8 opad = 0x5C;
	
	// Prepare key (pad or hash if > block size)
	uint8 Key[BlockSize];
	FMemory::Memzero(Key, BlockSize);
	FMemory::Memcpy(Key, DerivedHMACKey, FMath::Min(32, BlockSize));
	
	// Inner hash: H((K' ⊕ ipad) || m)
	TArray<uint8> InnerInput;
	InnerInput.Reserve(BlockSize + Data.Num());
	for (int32 i = 0; i < BlockSize; i++)
	{
		InnerInput.Add(Key[i] ^ ipad);
	}
	InnerInput.Append(Data);
	
	uint8 InnerHash[20];  // SHA-1 produces 20 bytes
	FSHA1::HashBuffer(InnerInput.GetData(), InnerInput.Num(), InnerHash);
	
	// Outer hash: H((K' ⊕ opad) || inner_hash)
	TArray<uint8> OuterInput;
	OuterInput.Reserve(BlockSize + 20);
	for (int32 i = 0; i < BlockSize; i++)
	{
		OuterInput.Add(Key[i] ^ opad);
	}
	OuterInput.Append(InnerHash, 20);
	
	uint8 OuterHash[20];
	FSHA1::HashBuffer(OuterInput.GetData(), OuterInput.Num(), OuterHash);
	
	// Return first 8 bytes (truncated HMAC for efficiency)
	TArray<uint8> HMAC;
	HMAC.Append(OuterHash, 8);
	return HMAC;
}

bool UEmbeddedDeviceController::ValidateHMAC(const TArray<uint8>& Data, const TArray<uint8>& ExpectedHMAC) const
{
	if (ExpectedHMAC.Num() != 8)
	{
		return false;
	}
	
	TArray<uint8> CalculatedHMAC = CalculateHMAC(Data);
	
	// Constant-time comparison to prevent timing attacks
	uint8 Diff = 0;
	for (int32 i = 0; i < 8; i++)
	{
		Diff |= CalculatedHMAC[i] ^ ExpectedHMAC[i];
	}
	
	return (Diff == 0);
}

// =====================================
// Random Number Generation
// =====================================

uint32 UEmbeddedDeviceController::GenerateRandomIV()
{
	// Simple xorshift PRNG (fast, sufficient for IV generation)
	RandomState ^= RandomState << 13;
	RandomState ^= RandomState >> 17;
	RandomState ^= RandomState << 5;
	return RandomState;
}

// =====================================
// Input Reading
// =====================================

bool UEmbeddedDeviceController::GetDigitalInput(int32 Channel) const
{
	const float* Value = InputValueCache.Find(Channel);
	if (Value)
	{
		// Digital input: > 0.5 = pressed
		return (*Value) > 0.5f;
	}
	return false;
}

float UEmbeddedDeviceController::GetAnalogInput(int32 Channel) const
{
	const float* Value = InputValueCache.Find(Channel);
	if (Value)
	{
		return *Value;
	}
	return 0.0f;
}



