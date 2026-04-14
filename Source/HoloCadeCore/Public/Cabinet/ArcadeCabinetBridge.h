// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Cabinet/ArcadeCabinetIOConfig.h"
#include "ArcadeCabinetBridge.generated.h"

class UHoloCadeUDPTransport;

UENUM()
enum class ECabinetPacketType : uint8
{
	Start = 1,
	Joystick = 2,
	Button = 3,
	Coin = 4,
	Card = 5,
	Other = 99,
	ButtonLedCommand = 100
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCabinetSharedPulse);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCabinetPlayerSlotPulse, int32, PlayerSlot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnCabinetJoystick, int32, PlayerSlot, int32, JoystickIndex, FVector2D, Axis);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnCabinetButtonState, int32, PlayerSlot, int32, ButtonIndex, bool, bPressed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCabinetOtherInput, int32, PlayerSlot, TArray<uint8>, Payload);

/**
 * Semantic façade over UHoloCadeUDPTransport for arcade cabinets (game engine side).
 * Incoming cabinet IO is packet-based: [messageType][playerSlotIndex][payload...].
 * Shared credit mode uses playerSlotIndex = -1 for coin/card packets.
 */
UCLASS(ClassGroup = (HoloCade), meta = (BlueprintSpawnableComponent))
class HOLOCADECORE_API UArcadeCabinetBridge : public UActorComponent
{
	GENERATED_BODY()

public:
	UArcadeCabinetBridge();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cabinet")
	TObjectPtr<UArcadeCabinetIOConfig> CabinetConfig;

	UPROPERTY(BlueprintAssignable, Category = "Cabinet|Events", meta = (HideInDetailPanel))
	FOnCabinetSharedPulse OnSharedCoinPulse;

	UPROPERTY(BlueprintAssignable, Category = "Cabinet|Events", meta = (HideInDetailPanel))
	FOnCabinetSharedPulse OnSharedCardPulse;

	UPROPERTY(BlueprintAssignable, Category = "Cabinet|Events", meta = (HideInDetailPanel))
	FOnCabinetPlayerSlotPulse OnPlayerCoinPulse;

	UPROPERTY(BlueprintAssignable, Category = "Cabinet|Events", meta = (HideInDetailPanel))
	FOnCabinetPlayerSlotPulse OnPlayerCardPulse;

	UPROPERTY(BlueprintAssignable, Category = "Cabinet|Events", meta = (HideInDetailPanel))
	FOnCabinetPlayerSlotPulse OnStartPressed;

	UPROPERTY(BlueprintAssignable, Category = "Cabinet|Events", meta = (HideInDetailPanel))
	FOnCabinetJoystick OnJoystick;

	UPROPERTY(BlueprintAssignable, Category = "Cabinet|Events", meta = (HideInDetailPanel))
	FOnCabinetButtonState OnButtonState;

	UPROPERTY(BlueprintAssignable, Category = "Cabinet|Events", meta = (HideInDetailPanel))
	FOnCabinetOtherInput OnOtherInput;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cabinet")
	UArcadeCabinetIOConfig* GetCabinetConfiguration() const { return CabinetConfig; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cabinet")
	UHoloCadeUDPTransport* GetTransport() const { return Transport; }

	UFUNCTION(BlueprintCallable, Category = "Cabinet")
	void ApplyConfiguration(UArcadeCabinetIOConfig* Next);

	/** Sends [ButtonLedCommand][playerSlot][buttonIndex][float LE] on outputPacketChannel. */
	UFUNCTION(BlueprintCallable, Category = "Cabinet")
	void SetButtonLedOutput(int32 PlayerSlot, int32 ButtonIndex, float NormalizedLevel);

	UFUNCTION(BlueprintCallable, Category = "Cabinet")
	void SendCabinetCommandPacket(const TArray<uint8>& Payload);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void ResolveTransport();
	void RegisterTransport();
	void UnregisterTransport();

	UFUNCTION()
	void OnTransportBytes(int32 Channel, const TArray<uint8>& Payload);

	void HandleStartPacket(int32 PlayerSlot, const TArray<uint8>& Payload);
	void HandleJoystickPacket(int32 PlayerSlot, const TArray<uint8>& Payload);
	void HandleButtonPacket(int32 PlayerSlot, const TArray<uint8>& Payload);
	void HandleCoinPacket(int32 PlayerSlot, const TArray<uint8>& Payload);
	void HandleCardPacket(int32 PlayerSlot, const TArray<uint8>& Payload);
	void HandleOtherPacket(int32 PlayerSlot, const TArray<uint8>& Payload);

	bool PulseOnRising(ECabinetPacketType Type, int32 Slot, int32 Index, bool bActive);
	bool IsValidPlayerSlot(int32 PlayerSlot) const;

	static float ReadFloatFromPayload(const TArray<uint8>& Payload, int32 Offset);

	UPROPERTY(Transient)
	TObjectPtr<UHoloCadeUDPTransport> Transport;

	TMap<FString, bool> DigitalState;
};
