// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Networking/UDPTransportBase.h"
#include "HoloCadeUDPTransport.generated.h"

/**
 * Data type enum for HoloCade binary protocol
 */
UENUM(BlueprintType)
enum class EHoloCadeUDPDataType : uint8
{
	Bool = 0 UMETA(DisplayName = "Boolean"),
	Int32 = 1 UMETA(DisplayName = "Integer"),
	Float = 2 UMETA(DisplayName = "Float"),
	String = 3 UMETA(DisplayName = "String"),
	Bytes = 4 UMETA(DisplayName = "Raw Bytes"),
	Struct = 5 UMETA(DisplayName = "Struct")
};

/**
 * HoloCade UDP Transport Base Component
 * 
 * Provides channel-agnostic UDP communication with HoloCade binary protocol.
 * This is the shared backbone for all UDP-based hardware communication in HoloCade.
 * 
 * Protocol Format: [0xAA][Type][Channel][Payload...][CRC]
 * 
 * Used by:
 * - EmbeddedDeviceController (embedded systems, costume controls)
 * - HapticPlatformController (motion platforms, gunships)
 * - Any future hardware controllers that need UDP communication
 * 
 * Each system maps channels to their specific needs:
 * - EmbeddedDeviceController: Channels = pins/sensors
 * - HapticPlatformController: Channels = motion axes (pitch, roll, etc.)
 */
UCLASS(Abstract, ClassGroup=(HoloCade), meta=(BlueprintSpawnableComponent))
class HOLOCADECORE_API UHoloCadeUDPTransport : public UActorComponent
{
	GENERATED_BODY()

public:
	UHoloCadeUDPTransport();
	virtual ~UHoloCadeUDPTransport();

	/**
	 * Initialize UDP connection to remote device
	 * @param RemoteIP - IP address of the device
	 * @param RemotePort - UDP port
	 * @param SocketName - Name for the socket (for debugging)
	 * @return True if initialization successful
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|UDP")
	bool InitializeUDPConnection(const FString& RemoteIP, int32 RemotePort, const FString& SocketName = TEXT("HoloCade_UDP"));

	/**
	 * Shutdown UDP connection
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|UDP")
	void ShutdownUDPConnection();

	/**
	 * Check if UDP connection is active
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|UDP")
	bool IsUDPConnected() const { return UDPTransport.IsUDPConnected(); }

	// =====================================
	// Channel-Based Send API (Primitive Types)
	// =====================================

	/**
	 * Send a float value on a specific channel
	 * @param Channel - Channel number (system-specific mapping)
	 * @param Value - Float value to send
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|UDP|Send")
	void SendFloat(int32 Channel, float Value);

	/**
	 * Send a boolean value on a specific channel
	 * @param Channel - Channel number (system-specific mapping)
	 * @param Value - Boolean value to send
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|UDP|Send")
	void SendBool(int32 Channel, bool Value);

	/**
	 * Send an integer value on a specific channel
	 * @param Channel - Channel number (system-specific mapping)
	 * @param Value - Integer value to send
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|UDP|Send")
	void SendInt32(int32 Channel, int32 Value);

	/**
	 * Send a string value on a specific channel
	 * @param Channel - Channel number (system-specific mapping)
	 * @param Value - String value to send (max 255 bytes)
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|UDP|Send")
	void SendString(int32 Channel, const FString& Value);

	/**
	 * Send raw bytes on a specific channel
	 * @param Channel - Channel number (system-specific mapping)
	 * @param Data - Raw byte array (max 255 bytes)
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|UDP|Send")
	void SendBytes(int32 Channel, const TArray<uint8>& Data);

	/**
	 * Send a struct on a specific channel (C++ only)
	 * @param Channel - Channel number (system-specific mapping)
	 * @param Data - Struct data (must be trivially copyable - POD or USTRUCT with only primitive members)
	 * 
	 * Note: Works with POD types and simple USTRUCTs containing only primitive types (float, int32, bool, etc.)
	 * For complex structs with pointers or non-trivial types, use SendBytes with manual serialization.
	 */
	template<typename T>
	void SendStruct(int32 Channel, const T& Data)
	{
		// Allow both POD types and USTRUCTs (which are typically trivially copyable if they only contain primitives)
		// We use sizeof and memcpy which works for simple structs with only primitive members
		TArray<uint8> Bytes;
		Bytes.SetNumUninitialized(sizeof(T));
		FMemory::Memcpy(Bytes.GetData(), &Data, sizeof(T));
		SendBytes(Channel, Bytes);
	}

	// =====================================
	// Channel-Based Receive API
	// =====================================

	/**
	 * Get the most recent float value received on a channel
	 * @param Channel - Channel number
	 * @return The most recent float value, or 0.0 if none received
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|UDP|Receive")
	float GetReceivedFloat(int32 Channel) const;

	/**
	 * Get the most recent boolean value received on a channel
	 * @param Channel - Channel number
	 * @return The most recent boolean value, or false if none received
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|UDP|Receive")
	bool GetReceivedBool(int32 Channel) const;

	/**
	 * Get the most recent integer value received on a channel
	 * @param Channel - Channel number
	 * @return The most recent integer value, or 0 if none received
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|UDP|Receive")
	int32 GetReceivedInt32(int32 Channel) const;

	/**
	 * Get the most recent bytes received on a channel (for struct packets)
	 * @param Channel - Channel number
	 * @return The most recent bytes array, or empty array if none received
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|UDP|Receive")
	TArray<uint8> GetReceivedBytes(int32 Channel) const;

	// Delegates for received data (bidirectional IO)
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFloatReceived, int32, Channel, float, Value);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBoolReceived, int32, Channel, bool, Value);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInt32Received, int32, Channel, int32, Value);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStringReceived, int32, Channel, FString, Value);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBytesReceived, int32, Channel, TArray<uint8>, Value);

	/** Event fired when float value is received from hardware */
	UPROPERTY(BlueprintAssignable, Category = "HoloCade|UDP|Events")
	FOnFloatReceived OnFloatReceived;

	/** Event fired when boolean value is received from hardware */
	UPROPERTY(BlueprintAssignable, Category = "HoloCade|UDP|Events")
	FOnBoolReceived OnBoolReceived;

	/** Event fired when integer value is received from hardware */
	UPROPERTY(BlueprintAssignable, Category = "HoloCade|UDP|Events")
	FOnInt32Received OnInt32Received;

	/** Event fired when string value is received from hardware */
	UPROPERTY(BlueprintAssignable, Category = "HoloCade|UDP|Events")
	FOnStringReceived OnStringReceived;

	/** Event fired when raw bytes are received from hardware */
	UPROPERTY(BlueprintAssignable, Category = "HoloCade|UDP|Events")
	FOnBytesReceived OnBytesReceived;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/**
	 * Process incoming UDP data (called from TickComponent)
	 * Override this if you need custom processing before packet parsing
	 */
	virtual void ProcessIncomingUDPData();

protected:
	/** Base UDP transport (handles raw socket management) */
	FUDPTransportBase UDPTransport;

	/** Cache of most recent received values per channel */
	TMap<int32, float> ReceivedFloatCache;
	TMap<int32, bool> ReceivedBoolCache;
	TMap<int32, int32> ReceivedInt32Cache;
	TMap<int32, TArray<uint8>> ReceivedBytesCache;

	/** Protocol start marker (HoloCade binary protocol) - accessible to subclasses */
	static constexpr uint8 PACKET_START_MARKER = 0xAA;

protected:
	/**
	 * Send data via UDP to remote device (uses base transport)
	 * Protected so subclasses can send raw packets (e.g., with encryption/HMAC)
	 */
	void SendUDPData(const TArray<uint8>& Data);

	/**
	 * Receive data via UDP from remote device (non-blocking, uses base transport)
	 * Protected so subclasses can receive raw packets for custom parsing
	 */
	void ReceiveUDPData();

	/**
	 * Build HoloCade binary packet: [0xAA][Type][Ch][Payload][CRC]
	 */
	TArray<uint8> BuildBinaryPacket(EHoloCadeUDPDataType DataType, int32 Channel, const TArray<uint8>& Payload);

	/**
	 * Parse incoming HoloCade binary packet
	 */
	void ParseBinaryPacket(const TArray<uint8>& Data, int32 Length);

	/**
	 * Calculate CRC checksum (XOR-based)
	 */
	uint8 CalculateCRC(const TArray<uint8>& Data, int32 Length) const;

	/**
	 * Validate CRC checksum
	 */
	bool ValidateCRC(const TArray<uint8>& Data, int32 Length, uint8 ExpectedCRC) const;
};

