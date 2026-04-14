// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "Cabinet/CabinetConfigPresets.h"
#include "Cabinet/ArcadeCabinetIOConfig.h"

UArcadeCabinetIOConfig* FCabinetConfigPresets::CreateDodgeThisTwoPlayerTemplate(UObject* Outer)
{
	UArcadeCabinetIOConfig* C = NewObject<UArcadeCabinetIOConfig>(Outer);
	C->PlayerSlotCount = 2;
	C->bSharedCreditInputs = true;
	C->InputPacketChannel = 40;
	C->OutputPacketChannel = 41;
	C->PlayerSlots.SetNum(2);
	for (int32 i = 0; i < 2; ++i)
	{
		C->PlayerSlots[i].bHasStartButton = true;
		C->PlayerSlots[i].JoystickCount = 0;
		C->PlayerSlots[i].ButtonCount = 0;
		C->PlayerSlots[i].bButtonsSupportLedMapping = false;
	}
	return C;
}
