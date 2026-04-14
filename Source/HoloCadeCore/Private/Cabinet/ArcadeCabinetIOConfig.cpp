// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "Cabinet/ArcadeCabinetIOConfig.h"

void UArcadeCabinetIOConfig::PostLoad()
{
	Super::PostLoad();
	ValidateConfig();
}

#if WITH_EDITOR
void UArcadeCabinetIOConfig::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	ValidateConfig();
}
#endif

void UArcadeCabinetIOConfig::ValidateConfig()
{
	if (PlayerSlotCount < 1)
	{
		PlayerSlotCount = 1;
	}

	if (PlayerSlots.Num() != PlayerSlotCount)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ArcadeCabinetIOConfig] '%s': PlayerSlots length (%d) should match PlayerSlotCount (%d)."),
			*GetName(), PlayerSlots.Num(), PlayerSlotCount);
	}

	if (InputPacketChannel < 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ArcadeCabinetIOConfig] '%s': InputPacketChannel should be >= 0."), *GetName());
	}
	if (OutputPacketChannel < 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ArcadeCabinetIOConfig] '%s': OutputPacketChannel should be >= 0."), *GetName());
	}
}
