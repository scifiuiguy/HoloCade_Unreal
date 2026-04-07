// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "HoloCadeExperienceInterface.generated.h"

// Forward declaration
class UHoloCadeInputAdapter;

/**
 * Interface for HoloCade Experience - allows HoloCadeCore to reference experiences without dependency on HoloCadeExperiences module
 */
UINTERFACE(MinimalAPI, BlueprintType, NotBlueprintable)
class UHoloCadeExperienceInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface that all HoloCade Experiences must implement
 * This breaks the circular dependency between HoloCadeCore and HoloCadeExperiences
 */
class HOLOCADECOMMON_API IHoloCadeExperienceInterface
{
	GENERATED_BODY()

public:
	/**
	 * Get the InputAdapter associated with this experience
	 * @return InputAdapter component, or nullptr if not available
	 */
	virtual UHoloCadeInputAdapter* GetInputAdapter() const { return nullptr; }
};

