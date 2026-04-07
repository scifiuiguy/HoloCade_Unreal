// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "ProLighting.h"
#include "IDMXTransport.h"
#include "Networking/UDPTransportBase.h"

/** Minimal Art-Net transport (UDP) */
class PROLIGHTING_API FArtNetTransport : public IDMXTransport
{
public:
	FArtNetTransport(const FString& InIP, int32 InPort, int32 InNet, int32 InSubNet)
		: TargetIP(InIP), Port(InPort), Net(InNet), SubNet(InSubNet) {}

	virtual bool Initialize() override
	{
		// Use base UDP transport for socket management (with broadcast enabled for Art-Net)
		return UDPTransport.InitializeUDPConnection(TargetIP, Port, TEXT("HoloCade_ArtNetTransport"), true);
	}

	virtual void Shutdown() override
	{
		UDPTransport.ShutdownUDPConnection();
	}

	virtual bool IsConnected() const override { return UDPTransport.IsUDPConnected(); }

	virtual void SendDMX(int32 Universe, const TArray<uint8>& DMXData) override
	{
		if (!IsConnected()) return;
		TArray<uint8> Packet = BuildArtDmxPacket(Universe, DMXData);
		UDPTransport.SendUDPData(Packet);
	}

private:
	TArray<uint8> BuildArtDmxPacket(int32 Universe, const TArray<uint8>& DMXData) const
	{
		TArray<uint8> Packet;
		Packet.SetNumUninitialized(18 + 512);
		int32 Offset = 0;
		FMemory::Memcpy(Packet.GetData() + Offset, "Art-Net\0", 8); Offset += 8;
		Packet[Offset++] = 0x00; Packet[Offset++] = 0x50; // OpDmx (0x5000 LE)
		Packet[Offset++] = 0x0E; Packet[Offset++] = 0x00; // ProtVer 14
		Packet[Offset++] = 0x00; // Sequence
		Packet[Offset++] = 0x00; // Physical
		int32 ArtUniverse = (SubNet * 16) + Universe;
		Packet[Offset++] = ArtUniverse & 0xFF;
		Packet[Offset++] = (ArtUniverse >> 8) & 0xFF;
		Packet[Offset++] = 0x02; // Length hi (512)
		Packet[Offset++] = 0x00; // Length lo
		FMemory::Memset(Packet.GetData() + Offset, 0, 512);
		int32 CopyLen = FMath::Min(512, DMXData.Num());
		FMemory::Memcpy(Packet.GetData() + Offset, DMXData.GetData(), CopyLen);
		return Packet;
	}

private:
	/** Base UDP transport (handles raw socket management) */
	FUDPTransportBase UDPTransport;

	/** Art-Net configuration */
	FString TargetIP;
	int32 Port = 6454;
	int32 Net = 0;
	int32 SubNet = 0;
};


