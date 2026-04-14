// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Networking/HoloCadeUDPTransport.h"
#include "HoloCadeUDPTransportSimple.generated.h"

/**
 * Concrete HoloCade UDP transport for plain channel IO (no embedded-device extensions).
 * Use with UArcadeCabinetBridge or any actor that needs the base protocol without a specialized subclass.
 */
UCLASS(ClassGroup = (HoloCade), meta = (BlueprintSpawnableComponent))
class HOLOCADECORE_API UHoloCadeUDPTransportSimple : public UHoloCadeUDPTransport
{
	GENERATED_BODY()
};
