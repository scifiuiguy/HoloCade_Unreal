// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "ProLighting.h"

// Forward declare to avoid circular include
struct FHoloCadeDMXFixture;

/** Simple registry for fixtures and RDM mappings */
class PROLIGHTING_API FFixtureRegistry
{
public:
	bool Register(const FHoloCadeDMXFixture& Fixture)
	{
		if (Fixtures.Contains(Fixture.VirtualFixtureID)) return false;
		Fixtures.Add(Fixture.VirtualFixtureID, Fixture);
		return true;
	}

	void Unregister(int32 VirtualFixtureID)
	{
		Fixtures.Remove(VirtualFixtureID);
		if (VirtualToRDM.Contains(VirtualFixtureID))
		{
			FString UID = VirtualToRDM[VirtualFixtureID];
			VirtualToRDM.Remove(VirtualFixtureID);
			RDMToVirtual.Remove(UID);
		}
	}

	const FHoloCadeDMXFixture* Find(int32 VirtualFixtureID) const
	{
		return Fixtures.Find(VirtualFixtureID);
	}

	FHoloCadeDMXFixture* FindMutable(int32 VirtualFixtureID)
	{
		return Fixtures.Find(VirtualFixtureID);
	}

	TArray<int32> GetIDs() const
	{
		TArray<int32> Keys; Fixtures.GetKeys(Keys); return Keys;
	}

	void MapRDM(int32 VirtualFixtureID, const FString& UID)
	{
		VirtualToRDM.Add(VirtualFixtureID, UID);
		RDMToVirtual.Add(UID, VirtualFixtureID);
	}

	bool TryGetRDMUID(int32 VirtualFixtureID, FString& OutUID) const
	{
		if (const FString* P = VirtualToRDM.Find(VirtualFixtureID)) { OutUID = *P; return true; }
		return false;
	}

	void Reset()
	{
		Fixtures.Reset();
		VirtualToRDM.Reset();
		RDMToVirtual.Reset();
	}

private:
	TMap<int32, FHoloCadeDMXFixture> Fixtures;
	TMap<int32, FString> VirtualToRDM;
	TMap<FString, int32> RDMToVirtual;
};


