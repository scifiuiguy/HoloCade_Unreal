// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Sockets.h"
#include "SocketSubsystem.h"

/**
 * Base UDP Transport (Non-UObject)
 * 
 * Provides raw UDP socket management for protocol-agnostic UDP communication.
 * This is the foundation for all UDP-based transports in HoloCade.
 * 
 * Used by:
 * - UHoloCadeUDPTransport (adds HoloCade binary protocol)
 * - FArtNetTransport (adds Art-Net protocol)
 * - Any future UDP-based protocols
 * 
 * This class handles:
 * - Socket creation and lifecycle
 * - IP address parsing
 * - Send/Receive operations
 * - Non-blocking I/O
 * 
 * Protocol-specific logic (packet building, parsing) is handled by subclasses.
 */
class HOLOCADECORE_API FUDPTransportBase
{
public:
	FUDPTransportBase();
	virtual ~FUDPTransportBase();

	/**
	 * Initialize UDP socket connection
	 * @param RemoteIP - IP address of the remote device
	 * @param RemotePort - UDP port
	 * @param SocketName - Name for the socket (for debugging)
	 * @param bEnableBroadcast - If true, enables broadcast (for Art-Net, etc.)
	 * @return True if initialization successful
	 */
	bool InitializeUDPConnection(const FString& RemoteIP, int32 RemotePort, const FString& SocketName = TEXT("HoloCade_UDP"), bool bEnableBroadcast = false);

	/**
	 * Shutdown UDP connection
	 */
	void ShutdownUDPConnection();

	/**
	 * Check if UDP connection is active
	 */
	bool IsUDPConnected() const { return UDPSocket != nullptr && RemoteAddress.IsValid(); }

	/**
	 * Send raw data via UDP
	 * @param Data - Raw byte array to send
	 * @return True if send was successful
	 */
	bool SendUDPData(const TArray<uint8>& Data);

	/**
	 * Receive raw data via UDP (non-blocking)
	 * @param OutData - Output buffer for received data
	 * @param OutBytesRead - Number of bytes actually read
	 * @param OutSenderAddr - Address of the sender (optional, can be nullptr)
	 * @return True if data was received
	 */
	bool ReceiveUDPData(TArray<uint8>& OutData, int32& OutBytesRead, TSharedPtr<FInternetAddr>* OutSenderAddr = nullptr);

	/**
	 * Check if data is pending on the socket
	 * @param OutPendingSize - Output size of pending data
	 * @return True if data is pending
	 */
	bool HasPendingData(uint32& OutPendingSize) const;

	/**
	 * Get the remote address
	 */
	TSharedPtr<FInternetAddr> GetRemoteAddress() const { return RemoteAddress; }

	/**
	 * Get the UDP socket (for advanced use cases)
	 */
	FSocket* GetSocket() const { return UDPSocket; }

protected:
	/** UDP Socket for communication */
	FSocket* UDPSocket = nullptr;

	/** Remote address for UDP communication */
	TSharedPtr<FInternetAddr> RemoteAddress;
};



