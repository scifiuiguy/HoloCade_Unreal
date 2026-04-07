// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "InputMappingContext.h"
#include "HOTASInputMappingContext.generated.h"

/**
 * Extended Input Mapping Context for HOTAS devices
 * 
 * This class extends UInputMappingContext to provide a public API
 * for programmatically adding mappings, since the base class's
 * Mappings array is protected.
 */
UCLASS(BlueprintType)
class LARGEHAPTICS_API UHOTASInputMappingContext : public UInputMappingContext
{
	GENERATED_BODY()

public:
	/**
	 * Add a mapping to this context
	 * @param Action - The input action to map
	 * @param Key - The key/axis to map to
	 */
	void AddMapping(UInputAction* Action, const FKey& Key);
};



