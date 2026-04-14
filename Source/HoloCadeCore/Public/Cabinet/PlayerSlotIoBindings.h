// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "PlayerSlotIoBindings.generated.h"

/**
 * Logical IO profile for one player slot.
 * Firmware sends packets with (message type + player slot index); configure capabilities here instead of per-control UDP channels.
 */
USTRUCT(BlueprintType)
struct HOLOCADECORE_API FPlayerSlotIoBindings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cabinet")
	bool bHasStartButton = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cabinet", meta = (ClampMin = "0"))
	int32 JoystickCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cabinet", meta = (ClampMin = "0"))
	int32 ButtonCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cabinet")
	bool bButtonsSupportLedMapping = true;
};
