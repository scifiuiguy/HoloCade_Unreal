// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Cabinet/PlayerSlotIoBindings.h"
#include "Engine/DataAsset.h"

#if WITH_EDITOR
#include "UObject/PropertyChangedEvent.h"
#endif

#include "ArcadeCabinetIOConfig.generated.h"

/**
 * Scriptable description of cabinet hardware: credit topology, per-player controls, LED pairings.
 * Game code references this asset from UArcadeCabinetBridge; UDP framing stays in HoloCade networking.
 */
UCLASS(BlueprintType)
class HOLOCADECORE_API UArcadeCabinetIOConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cabinet", meta = (ClampMin = "1"))
	int32 PlayerSlotCount = 2;

	/** When true, coin/card use playerSlotIndex -1 packets. When false, credit packets are per player slot index. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cabinet")
	bool bSharedCreditInputs = true;

	/** HoloCade logical channel: ECU → game (incoming cabinet/sensor data). Payload: messageType + playerSlotIndex + body. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cabinet", meta = (ClampMin = "0"))
	int32 InputPacketChannel = 40;

	/** HoloCade logical channel: game → ECU (outbound commands, e.g. button LEDs). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cabinet", meta = (ClampMin = "0"))
	int32 OutputPacketChannel = 41;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cabinet")
	TArray<FPlayerSlotIoBindings> PlayerSlots;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	virtual void PostLoad() override;

private:
	void ValidateConfig();
};
