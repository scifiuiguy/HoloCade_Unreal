// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "ProLighting.h"
#include "ProLightingTypes.h"
#include "ArtNetTransport.h"
#include "IDMXTransport.h"
#include "IBridgeEvents.h"
#include "Sockets.h"
#include "SocketSubsystem.h"

/** Consolidated manager for Art-Net transport + discovery */
/** Implements IDMXTransport for transport operations, with additional discovery capabilities */
class PROLIGHTING_API FArtNetManager : public IDMXTransport, public IBridgeEvents
{
public:
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnNodeDiscovered, const FHoloCadeArtNetNode& /*Node*/);

    // Initialize with Art-Net specific parameters (called before IDMXTransport::Initialize)
    bool Initialize(const FString& IP, int32 Port, int32 Net, int32 SubNet);
    
    // IDMXTransport interface
    virtual bool Initialize() override;
    virtual void Shutdown() override;
    virtual bool IsConnected() const override;
    virtual void SendDMX(int32 Universe, const TArray<uint8>& DMXData) override;
    
    // Art-Net specific methods
    void Tick(float DeltaTime);
    void SendArtPoll();
    const TMap<FString, FHoloCadeArtNetNode>& GetDiscoveredArtNetNodes() const { return DiscoveredNodes; }
    TArray<FHoloCadeArtNetNode> GetNodes() const;
    FOnNodeDiscovered& OnNodeDiscovered() { return OnNodeDiscoveredDelegate; }

	// IBridgeEvents interface
	virtual void BridgeEvents(UProLightingController* Controller) override;

private:
    TUniquePtr<FArtNetTransport> Transport;
    // Discovery (consolidated from ArtNetDiscovery)
    bool InitializeDiscovery(uint16 InPort);
    void ProcessIncoming();
    TArray<uint8> BuildArtPollPacket() const;
    bool ParseArtPollReply(const TArray<uint8>& PacketData, FHoloCadeArtNetNode& OutNode);

    FSocket* DiscoverySocket = nullptr;
    TSharedPtr<FInternetAddr> SendAddr;
    uint16 ArtNetPort = 6454;
    float PollIntervalSeconds = 2.0f;
    float Accumulated = 0.0f;
    TMap<FString, FHoloCadeArtNetNode> DiscoveredNodes; // Key = Source IP
    FOnNodeDiscovered OnNodeDiscoveredDelegate;
};


