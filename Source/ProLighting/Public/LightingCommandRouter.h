// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "ProLighting.h"
#include "ProLightingTypes.h"
#include "ProLightingController.h"
#include "FixtureService.h"

/**
 * Legacy command router for convenience APIs.
 * Now delegates to FixtureService for all fixture operations.
 */
class PROLIGHTING_API FLightingCommandRouter
{
public:
	static void SetIntensity(UProLightingController& Controller, int32 VirtualFixtureID, float Intensity)
	{
		if (!Controller.IsDMXConnected()) return;
		Intensity = FMath::Clamp(Intensity, 0.0f, 1.0f);
		if (FFixtureService* Service = Controller.GetFixtureService())
		{
			int32 Universe = Service->SetIntensityById(VirtualFixtureID, Intensity);
			if (Universe >= 0)
			{
				// Note: Controller's Tick will flush universes, but we can flush immediately if needed
				// For now, rely on periodic flush in Tick
			}
		}
	}

	static void SetColor(UProLightingController& Controller, int32 VirtualFixtureID, float Red, float Green, float Blue, float White = -1.0f)
	{
		if (!Controller.IsDMXConnected()) return;
		Red = FMath::Clamp(Red, 0.0f, 1.0f);
		Green = FMath::Clamp(Green, 0.0f, 1.0f);
		Blue = FMath::Clamp(Blue, 0.0f, 1.0f);
		if (White >= 0.0f) White = FMath::Clamp(White, 0.0f, 1.0f);
		if (FFixtureService* Service = Controller.GetFixtureService())
		{
			int32 Universe = Service->SetColorRGBWById(VirtualFixtureID, Red, Green, Blue, White);
			if (Universe >= 0)
			{
				// Note: Controller's Tick will flush universes, but we can flush immediately if needed
				// For now, rely on periodic flush in Tick
			}
		}
	}
};


