// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "HoloCadeServerBeacon.generated.h"

/**
 * Server information broadcast over LAN
 */
USTRUCT(BlueprintType)
struct FHoloCadeServerInfo
{
	GENERATED_BODY()

	/** Server IP address */
	UPROPERTY(BlueprintReadOnly, Category = "HoloCade|Networking")
	FString ServerIP;

	/** Server port */
	UPROPERTY(BlueprintReadOnly, Category = "HoloCade|Networking")
	int32 ServerPort = 7777;

	/** Experience type (e.g., "AIFacemask", "Gunship") */
	UPROPERTY(BlueprintReadOnly, Category = "HoloCade|Networking")
	FString ExperienceType;

	/** Server name/identifier */
	UPROPERTY(BlueprintReadOnly, Category = "HoloCade|Networking")
	FString ServerName;

	/** Current player count */
	UPROPERTY(BlueprintReadOnly, Category = "HoloCade|Networking")
	int32 CurrentPlayers = 0;

	/** Maximum player count */
	UPROPERTY(BlueprintReadOnly, Category = "HoloCade|Networking")
	int32 MaxPlayers = 8;

	/** Current experience state (e.g., "Lobby", "InProgress", "Complete") */
	UPROPERTY(BlueprintReadOnly, Category = "HoloCade|Networking")
	FString ExperienceState;

	/** Server version (for compatibility checks) */
	UPROPERTY(BlueprintReadOnly, Category = "HoloCade|Networking")
	FString ServerVersion = TEXT("1.0.0");

	/** Timestamp of last beacon (for timeout detection) */
	float LastBeaconTime = 0.0f;

	/** Is this server accepting new connections? */
	UPROPERTY(BlueprintReadOnly, Category = "HoloCade|Networking")
	bool bAcceptingConnections = true;
};

/**
 * Delegate for server discovery events
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnServerDiscovered, const FHoloCadeServerInfo&, ServerInfo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnServerLost, const FString&, ServerIP);

/**
 * HoloCade Server Beacon
 * 
 * Handles automatic server discovery on LAN using UDP broadcasting.
 * 
 * SERVER MODE:
 * - Broadcasts server presence every X seconds
 * - Includes server metadata (experience type, player count, etc.)
 * - Runs on dedicated server to advertise availability
 * 
 * CLIENT MODE:
 * - Listens for server broadcasts
 * - Maintains list of available servers
 * - Auto-connects to appropriate server
 * - Detects when servers go offline
 * 
 * Perfect for LBE installations with multiple concurrent experiences.
 */
UCLASS(BlueprintType)
class HOLOCADECORE_API UHoloCadeServerBeacon : public UObject
{
	GENERATED_BODY()

public:
	UHoloCadeServerBeacon();
	virtual ~UHoloCadeServerBeacon();

	/** Broadcast port for server discovery (same for all HoloCade installations) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Networking")
	int32 BroadcastPort = 7778;

	/** How often server broadcasts presence (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Networking")
	float BroadcastInterval = 2.0f;

	/** How long before considering a server lost (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Networking")
	float ServerTimeout = 10.0f;

	/** Fired when a new server is discovered */
	UPROPERTY(BlueprintAssignable, Category = "HoloCade|Networking")
	FOnServerDiscovered OnServerDiscovered;

	/** Fired when a server is no longer responding */
	UPROPERTY(BlueprintAssignable, Category = "HoloCade|Networking")
	FOnServerLost OnServerLost;

	/**
	 * Start broadcasting as a server (Dedicated Server only)
	 * @param ServerInfo - Information about this server to broadcast
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Networking")
	bool StartServerBroadcast(const FHoloCadeServerInfo& ServerInfo);

	/**
	 * Start listening for server broadcasts (Clients only)
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Networking")
	bool StartClientDiscovery();

	/**
	 * Stop broadcasting/listening
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Networking")
	void Stop();

	/**
	 * Get list of currently discovered servers
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Networking")
	TArray<FHoloCadeServerInfo> GetDiscoveredServers() const;

	/**
	 * Get a specific server by experience type
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Networking")
	bool GetServerByExperienceType(const FString& ExperienceType, FHoloCadeServerInfo& OutServerInfo) const;

	/**
	 * Update server info (for servers to update player count, state, etc.)
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Networking")
	void UpdateServerInfo(const FHoloCadeServerInfo& NewServerInfo);

	/** Is this beacon active? */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|Networking")
	bool IsActive() const { return bIsActive; }

	/** Is this beacon in server mode? */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|Networking")
	bool IsServerMode() const { return bIsServerMode; }

	/** Tick function for periodic broadcasts and server timeout checks */
	void Tick(float DeltaTime);

private:
	FSocket* BroadcastSocket = nullptr;
	FSocket* ListenSocket = nullptr;
	
	bool bIsActive = false;
	bool bIsServerMode = false;
	
	FHoloCadeServerInfo CurrentServerInfo;
	TMap<FString, FHoloCadeServerInfo> DiscoveredServers;  // Key = ServerIP
	
	float TimeSinceLastBroadcast = 0.0f;

	/** Serialize server info to binary for network transmission */
	TArray<uint8> SerializeServerInfo(const FHoloCadeServerInfo& ServerInfo) const;

	/** Deserialize server info from binary */
	bool DeserializeServerInfo(const TArray<uint8>& Data, FHoloCadeServerInfo& OutServerInfo) const;

	/** Send broadcast packet */
	void SendBroadcast();

	/** Receive and process incoming packets */
	void ReceivePackets();

	/** Check for server timeouts */
	void CheckServerTimeouts();

	/** Create broadcast socket */
	bool CreateBroadcastSocket();

	/** Create listen socket */
	bool CreateListenSocket();

	/** Cleanup sockets */
	void CleanupSockets();
};



