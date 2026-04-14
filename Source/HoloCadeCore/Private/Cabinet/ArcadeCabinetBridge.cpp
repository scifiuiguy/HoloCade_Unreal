// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "Cabinet/ArcadeCabinetBridge.h"
#include "Networking/HoloCadeUDPTransport.h"

UArcadeCabinetBridge::UArcadeCabinetBridge()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UArcadeCabinetBridge::BeginPlay()
{
	Super::BeginPlay();
	ResolveTransport();
	RegisterTransport();
}

void UArcadeCabinetBridge::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterTransport();
	Super::EndPlay(EndPlayReason);
}

void UArcadeCabinetBridge::ApplyConfiguration(UArcadeCabinetIOConfig* Next)
{
	CabinetConfig = Next;
	if (GetOwner() && GetOwner()->HasActorBegunPlay())
	{
		UnregisterTransport();
		RegisterTransport();
	}
}

void UArcadeCabinetBridge::ResolveTransport()
{
	if (Transport)
	{
		return;
	}
	if (AActor* Owner = GetOwner())
	{
		Transport = Owner->FindComponentByClass<UHoloCadeUDPTransport>();
	}
	if (!Transport)
	{
		UE_LOG(LogTemp, Error, TEXT("[ArcadeCabinetBridge] HoloCadeUDPTransport is required on this actor."));
	}
}

void UArcadeCabinetBridge::RegisterTransport()
{
	if (!Transport)
	{
		return;
	}
	if (!CabinetConfig)
	{
		UE_LOG(LogTemp, Error, TEXT("[ArcadeCabinetBridge] Assign an ArcadeCabinetIOConfig asset."));
		return;
	}
	Transport->OnBytesReceived.AddDynamic(this, &UArcadeCabinetBridge::OnTransportBytes);
}

void UArcadeCabinetBridge::UnregisterTransport()
{
	if (Transport)
	{
		Transport->OnBytesReceived.RemoveDynamic(this, &UArcadeCabinetBridge::OnTransportBytes);
	}
	DigitalState.Empty();
}

void UArcadeCabinetBridge::OnTransportBytes(int32 Channel, const TArray<uint8>& Payload)
{
	if (!CabinetConfig || Channel != CabinetConfig->InputPacketChannel || Payload.Num() < 2)
	{
		return;
	}

	const ECabinetPacketType PacketType = static_cast<ECabinetPacketType>(Payload[0]);
	const int32 PlayerSlot = static_cast<int8>(Payload[1]);

	switch (PacketType)
	{
	case ECabinetPacketType::Start:
		HandleStartPacket(PlayerSlot, Payload);
		return;
	case ECabinetPacketType::Joystick:
		HandleJoystickPacket(PlayerSlot, Payload);
		return;
	case ECabinetPacketType::Button:
		HandleButtonPacket(PlayerSlot, Payload);
		return;
	case ECabinetPacketType::Coin:
		HandleCoinPacket(PlayerSlot, Payload);
		return;
	case ECabinetPacketType::Card:
		HandleCardPacket(PlayerSlot, Payload);
		return;
	case ECabinetPacketType::Other:
		HandleOtherPacket(PlayerSlot, Payload);
		return;
	default:
		HandleOtherPacket(PlayerSlot, Payload);
		return;
	}
}

void UArcadeCabinetBridge::HandleStartPacket(int32 PlayerSlot, const TArray<uint8>& Payload)
{
	if (!IsValidPlayerSlot(PlayerSlot))
	{
		return;
	}
	const FPlayerSlotIoBindings& SlotCfg = CabinetConfig->PlayerSlots[PlayerSlot];
	if (!SlotCfg.bHasStartButton)
	{
		return;
	}

	const bool bActive = Payload.Num() < 3 || Payload[2] != 0;
	if (PulseOnRising(ECabinetPacketType::Start, PlayerSlot, 0, bActive))
	{
		OnStartPressed.Broadcast(PlayerSlot);
	}
}

void UArcadeCabinetBridge::HandleJoystickPacket(int32 PlayerSlot, const TArray<uint8>& Payload)
{
	if (!IsValidPlayerSlot(PlayerSlot) || Payload.Num() < 11)
	{
		return;
	}

	const int32 JoystickIndex = Payload[2];
	const FPlayerSlotIoBindings& SlotCfg = CabinetConfig->PlayerSlots[PlayerSlot];
	if (JoystickIndex < 0 || JoystickIndex >= SlotCfg.JoystickCount)
	{
		return;
	}

	const float X = ReadFloatFromPayload(Payload, 3);
	const float Y = ReadFloatFromPayload(Payload, 7);
	OnJoystick.Broadcast(PlayerSlot, JoystickIndex, FVector2D(X, Y));
}

void UArcadeCabinetBridge::HandleButtonPacket(int32 PlayerSlot, const TArray<uint8>& Payload)
{
	if (!IsValidPlayerSlot(PlayerSlot) || Payload.Num() < 4)
	{
		return;
	}

	const int32 ButtonIndex = Payload[2];
	const FPlayerSlotIoBindings& SlotCfg = CabinetConfig->PlayerSlots[PlayerSlot];
	if (ButtonIndex < 0 || ButtonIndex >= SlotCfg.ButtonCount)
	{
		return;
	}

	const bool bPressed = Payload[3] != 0;
	const FString Key = FString::Printf(TEXT("%d_%d_%d"), (int32)ECabinetPacketType::Button, PlayerSlot, ButtonIndex);
	const bool bPrevious = DigitalState.FindRef(Key);
	if (!DigitalState.Contains(Key) || bPrevious != bPressed)
	{
		DigitalState.FindOrAdd(Key) = bPressed;
		OnButtonState.Broadcast(PlayerSlot, ButtonIndex, bPressed);
	}
}

void UArcadeCabinetBridge::HandleCoinPacket(int32 PlayerSlot, const TArray<uint8>& Payload)
{
	const bool bActive = Payload.Num() < 3 || Payload[2] != 0;
	if (PlayerSlot == -1 && CabinetConfig->bSharedCreditInputs)
	{
		if (PulseOnRising(ECabinetPacketType::Coin, -1, 0, bActive))
		{
			OnSharedCoinPulse.Broadcast();
		}
		return;
	}
	if (CabinetConfig->bSharedCreditInputs)
	{
		return;
	}
	if (!IsValidPlayerSlot(PlayerSlot))
	{
		return;
	}
	if (PulseOnRising(ECabinetPacketType::Coin, PlayerSlot, 0, bActive))
	{
		OnPlayerCoinPulse.Broadcast(PlayerSlot);
	}
}

void UArcadeCabinetBridge::HandleCardPacket(int32 PlayerSlot, const TArray<uint8>& Payload)
{
	const bool bActive = Payload.Num() < 3 || Payload[2] != 0;
	if (PlayerSlot == -1 && CabinetConfig->bSharedCreditInputs)
	{
		if (PulseOnRising(ECabinetPacketType::Card, -1, 0, bActive))
		{
			OnSharedCardPulse.Broadcast();
		}
		return;
	}
	if (CabinetConfig->bSharedCreditInputs)
	{
		return;
	}
	if (!IsValidPlayerSlot(PlayerSlot))
	{
		return;
	}
	if (PulseOnRising(ECabinetPacketType::Card, PlayerSlot, 0, bActive))
	{
		OnPlayerCardPulse.Broadcast(PlayerSlot);
	}
}

void UArcadeCabinetBridge::HandleOtherPacket(int32 PlayerSlot, const TArray<uint8>& Payload)
{
	const int32 BodyLen = Payload.Num() - 2;
	TArray<uint8> Body;
	if (BodyLen > 0)
	{
		Body.SetNum(BodyLen);
		FMemory::Memcpy(Body.GetData(), Payload.GetData() + 2, BodyLen);
	}
	OnOtherInput.Broadcast(PlayerSlot, Body);
}

bool UArcadeCabinetBridge::PulseOnRising(ECabinetPacketType Type, int32 Slot, int32 Index, bool bActive)
{
	const FString Key = FString::Printf(TEXT("%d_%d_%d"), (int32)Type, Slot, Index);
	const bool bPrev = DigitalState.FindRef(Key);
	DigitalState.FindOrAdd(Key) = bActive;
	return bActive && !bPrev;
}

bool UArcadeCabinetBridge::IsValidPlayerSlot(int32 PlayerSlot) const
{
	return PlayerSlot >= 0 && CabinetConfig && CabinetConfig->PlayerSlots.Num() > PlayerSlot;
}

float UArcadeCabinetBridge::ReadFloatFromPayload(const TArray<uint8>& Payload, int32 Offset)
{
	if (Payload.Num() < Offset + 4)
	{
		return 0.f;
	}
	float V = 0.f;
	FMemory::Memcpy(&V, Payload.GetData() + Offset, sizeof(float));
	return V;
}

void UArcadeCabinetBridge::SetButtonLedOutput(int32 PlayerSlot, int32 ButtonIndex, float NormalizedLevel)
{
	if (!CabinetConfig || !Transport || !IsValidPlayerSlot(PlayerSlot))
	{
		return;
	}
	const FPlayerSlotIoBindings& SlotCfg = CabinetConfig->PlayerSlots[PlayerSlot];
	if (!SlotCfg.bButtonsSupportLedMapping || ButtonIndex < 0 || ButtonIndex >= SlotCfg.ButtonCount)
	{
		return;
	}

	uint8 Packet[7];
	Packet[0] = static_cast<uint8>(ECabinetPacketType::ButtonLedCommand);
	Packet[1] = static_cast<uint8>(PlayerSlot);
	Packet[2] = static_cast<uint8>(ButtonIndex);
	FMemory::Memcpy(&Packet[3], &NormalizedLevel, sizeof(float));

	TArray<uint8> Bytes;
	Bytes.Append(Packet, UE_ARRAY_COUNT(Packet));
	Transport->SendBytes(CabinetConfig->OutputPacketChannel, Bytes);
}

void UArcadeCabinetBridge::SendCabinetCommandPacket(const TArray<uint8>& Payload)
{
	if (Transport && CabinetConfig && Payload.Num() > 0)
	{
		Transport->SendBytes(CabinetConfig->OutputPacketChannel, Payload);
	}
}
