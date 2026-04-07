// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "ProLighting.h"
#include "ProLightingTypes.h"
#include "FixtureRegistry.h"

class PROLIGHTING_API FFixtureValidator
{
public:
	static bool ValidateRegister(const FHoloCadeDMXFixture& Candidate, const FFixtureRegistry& Registry, FString& OutError)
	{
		if (Candidate.VirtualFixtureID <= 0) { OutError = TEXT("Invalid VirtualFixtureID"); return false; }
		if (Candidate.DMXChannel < 1 || Candidate.DMXChannel > 512) { OutError = TEXT("Invalid DMX channel"); return false; }
		int32 Channels = Candidate.ChannelCount > 0 ? Candidate.ChannelCount : RequiredChannels(Candidate);
		int32 End = Candidate.DMXChannel + Channels - 1;
		if (End > 512) { OutError = TEXT("Fixture exceeds universe size"); return false; }
		for (int32 Id : Registry.GetIDs())
		{
			const FHoloCadeDMXFixture* Existing = Registry.Find(Id);
			if (!Existing) continue;
			if (Existing->Universe != Candidate.Universe) continue;
			int32 ExistingEnd = Existing->DMXChannel + Existing->ChannelCount - 1;
			bool bOverlap = !(End < Existing->DMXChannel || Candidate.DMXChannel > ExistingEnd);
			if (bOverlap)
			{
				OutError = FString::Printf(TEXT("Overlaps with fixture %d"), Existing->VirtualFixtureID);
				return false;
			}
		}
		return true;
	}

private:
	static int32 RequiredChannels(const FHoloCadeDMXFixture& F)
	{
		switch (F.FixtureType)
		{
		case EHoloCadeDMXFixtureType::Dimmable: return 1;
		case EHoloCadeDMXFixtureType::RGB: return 3;
		case EHoloCadeDMXFixtureType::RGBW: return 4;
		case EHoloCadeDMXFixtureType::MovingHead: return 8;
		case EHoloCadeDMXFixtureType::Custom: return FMath::Max(1, F.CustomChannelMapping.Num());
		default: return 1;
		}
	}
};




