// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

class UArcadeCabinetIOConfig;

/**
 * In-memory cabinet presets for titles that do not use classic sticks/buttons (e.g. DodgeThis with piezos).
 */
struct HOLOCADECORE_API FCabinetConfigPresets
{
	/** Two player stations, shared coin + card, no joysticks and no face buttons (piezo / alternate inputs only). */
	static UArcadeCabinetIOConfig* CreateDodgeThisTwoPlayerTemplate(UObject* Outer);
};
