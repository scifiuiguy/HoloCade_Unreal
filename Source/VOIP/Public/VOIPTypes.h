// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "VOIPTypes.generated.h"

/**
 * VOIP connection state
 */
UENUM(BlueprintType)
enum class EVOIPConnectionState : uint8
{
	Disconnected,
	Connecting,
	Connected,
	Error
};



