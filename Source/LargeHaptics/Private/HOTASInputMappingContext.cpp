// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "HOTASInputMappingContext.h"

void UHOTASInputMappingContext::AddMapping(UInputAction* Action, const FKey& Key)
{
	FEnhancedActionKeyMapping Mapping;
	Mapping.Action = Action;
	Mapping.Key = Key;
	// Now we can access the protected Mappings array since we're in a derived class
	Mappings.Add(Mapping);
}



