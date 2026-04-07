// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "ProLighting/Public/ArtNetManager.h"
#include "ProLighting/Public/ProLightingController.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "IPAddress.h"

bool FArtNetManager::Initialize(const FString& IP, int32 Port, int32 Net, int32 SubNet)
{
    ArtNetPort = (uint16)Port;
    Transport = MakeUnique<FArtNetTransport>(IP, Port, Net, SubNet);
    // Store config for later Initialize() call
    return true;
}

bool FArtNetManager::Initialize()
{
    if (!Transport)
    {
        return false;
    }
    if (!Transport->Initialize())
    {
        return false;
    }
    if (!InitializeDiscovery(ArtNetPort))
    {
        UE_LOG(LogProLighting, Warning, TEXT("ArtNetManager: Discovery init failed; transport only"));
    }
    return true;
}

void FArtNetManager::Shutdown()
{
    if (DiscoverySocket)
    {
        DiscoverySocket->Close();
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(DiscoverySocket);
        DiscoverySocket = nullptr;
    }
    SendAddr.Reset();
    DiscoveredNodes.Empty();
    if (Transport)
    {
        Transport->Shutdown();
        Transport.Reset();
    }
}

void FArtNetManager::Tick(float DeltaTime)
{
    ProcessIncoming();
    Accumulated += DeltaTime;
    if (Accumulated >= PollIntervalSeconds)
    {
        SendArtPoll();
        Accumulated = 0.0f;
    }
}

bool FArtNetManager::IsConnected() const { return Transport && Transport->IsConnected(); }

void FArtNetManager::SendDMX(int32 Universe, const TArray<uint8>& DMXData)
{
    if (Transport && Transport->IsConnected())
    {
        Transport->SendDMX(Universe, DMXData);
    }
}

void FArtNetManager::SendArtPoll()
{
    if (!DiscoverySocket || !SendAddr)
    {
        return;
    }
    TArray<uint8> Packet = BuildArtPollPacket();
    int32 BytesSent = 0;
    DiscoverySocket->SendTo(Packet.GetData(), Packet.Num(), BytesSent, *SendAddr);
    if (BytesSent != Packet.Num())
    {
        UE_LOG(LogProLighting, Warning, TEXT("ArtNetManager: ArtPoll send incomplete (%d/%d)"), BytesSent, Packet.Num());
    }
}

TArray<FHoloCadeArtNetNode> FArtNetManager::GetNodes() const
{
    TArray<FHoloCadeArtNetNode> Nodes;
    DiscoveredNodes.GenerateValueArray(Nodes);
    return Nodes;
}

bool FArtNetManager::InitializeDiscovery(uint16 InPort)
{
    ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
    if (!SocketSubsystem)
    {
        UE_LOG(LogProLighting, Error, TEXT("ArtNetManager: Failed to get socket subsystem"));
        return false;
    }

    DiscoverySocket = SocketSubsystem->CreateSocket(NAME_DGram, TEXT("HoloCadeArtNetDiscovery"), false);
    if (!DiscoverySocket)
    {
        UE_LOG(LogProLighting, Error, TEXT("ArtNetManager: Failed to create discovery socket"));
        return false;
    }

    // Bind to Art-Net port to receive ArtPollReply
    TSharedPtr<FInternetAddr> BindAddr = SocketSubsystem->CreateInternetAddr();
    bool bIsValid = false;
    BindAddr->SetIp(0, bIsValid);
    BindAddr->SetPort(InPort);
    if (!DiscoverySocket->Bind(*BindAddr))
    {
        UE_LOG(LogProLighting, Error, TEXT("ArtNetManager: Failed to bind discovery socket to %d"), (int32)InPort);
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(DiscoverySocket);
        DiscoverySocket = nullptr;
        return false;
    }

    DiscoverySocket->SetNonBlocking(true);
    int32 BufferSize = 2 * 1024 * 1024;
    DiscoverySocket->SetReceiveBufferSize(2 * 1024 * 1024, BufferSize);
    DiscoverySocket->SetBroadcast(true);

    // Send address (broadcast)
    SendAddr = SocketSubsystem->CreateInternetAddr();
    SendAddr->SetBroadcastAddress();
    SendAddr->SetPort(InPort);

    UE_LOG(LogProLighting, Log, TEXT("ArtNetManager: Discovery initialized on port %d"), (int32)InPort);
    return true;
}

void FArtNetManager::ProcessIncoming()
{
    if (!DiscoverySocket)
    {
        return;
    }
    uint32 BufferSize = 2048;
    TArray<uint8> ReceiveBuffer;
    ReceiveBuffer.SetNumUninitialized(BufferSize);
    TSharedRef<FInternetAddr> SourceAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();

    int32 BytesRead = 0;
    while (DiscoverySocket->RecvFrom(ReceiveBuffer.GetData(), BufferSize, BytesRead, *SourceAddr))
    {
        if (BytesRead > 0)
        {
            TArray<uint8> PacketData;
            PacketData.Append(ReceiveBuffer.GetData(), BytesRead);

            FString SourceIP = SourceAddr->ToString(false);
            FHoloCadeArtNetNode Node;
            if (ParseArtPollReply(PacketData, Node))
            {
                if (!DiscoveredNodes.Contains(SourceIP))
                {
                    Node.IPAddress = SourceIP;
                    Node.LastSeenTimestamp = FDateTime::Now();
                    DiscoveredNodes.Add(SourceIP, Node);
                    OnNodeDiscoveredDelegate.Broadcast(Node);
                    UE_LOG(LogProLighting, Log, TEXT("ArtNetManager: Discovered node: %s (%s)"), *Node.NodeName, *SourceIP);
                }
                else
                {
                    DiscoveredNodes[SourceIP].LastSeenTimestamp = FDateTime::Now();
                }
            }
        }
    }
}

TArray<uint8> FArtNetManager::BuildArtPollPacket() const
{
    TArray<uint8> Packet;
    Packet.SetNumUninitialized(14);
    int32 Offset = 0;
    FMemory::Memcpy(Packet.GetData() + Offset, "Art-Net\0", 8); Offset += 8;
    Packet[Offset++] = 0x20; Packet[Offset++] = 0x00; // OpCode ArtPoll
    Packet[Offset++] = 0x0E; Packet[Offset++] = 0x00; // ProtVer 14
    Packet[Offset++] = 0x02; // Flags
    Packet[Offset++] = 0x07; // TalkToMe
    Packet[Offset++] = 0x00;
    Packet[Offset++] = 0x00;
    return Packet;
}

bool FArtNetManager::ParseArtPollReply(const TArray<uint8>& PacketData, FHoloCadeArtNetNode& OutNode)
{
    if (PacketData.Num() < 240) return false;
    if (FMemory::Memcmp(PacketData.GetData(), "Art-Net\0", 8) != 0) return false;
    uint16 OpCode = (uint16)PacketData[8] | ((uint16)PacketData[9] << 8);
    if (OpCode != 0x0021) return false; // ArtPollReply

    int32 NameOffset = 26;
    OutNode.NodeName = FString(ANSI_TO_TCHAR((const char*)PacketData.GetData() + NameOffset));
    NameOffset = 44;
    FString LongName = FString(ANSI_TO_TCHAR((const char*)PacketData.GetData() + NameOffset));
    if (LongName.Len() > 0) OutNode.NodeType = LongName;

    int32 NumPortsHi = (PacketData[173] >> 4) & 0x0F;
    int32 NumPortsLo = PacketData[173] & 0x0F;
    OutNode.OutputCount = FMath::Max(1, NumPortsHi * 16 + NumPortsLo);
    OutNode.UniversesPerOutput = 1;
    return true;
}

void FArtNetManager::BridgeEvents(UProLightingController* Controller)
{
	if (!Controller)
	{
		return;
	}

	// Bridge Art-Net node discovery events
	OnNodeDiscoveredDelegate.AddLambda([Controller](const FHoloCadeArtNetNode& Node)
	{
		Controller->OnArtNetNodeDiscovered.Broadcast(Node);
	});
}


