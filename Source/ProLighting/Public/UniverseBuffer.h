// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "ProLighting.h"

/**
 * FUniverseBuffer - Manages per-universe DMX channel data
 * 
 * DMX Universes Explained:
 * 
 * A DMX "universe" is a collection of 512 channels (0-511). Each channel can hold a value from 0-255.
 * 
 * Why Multiple Universes?
 * - Traditional USB DMX interfaces typically support only ONE universe (512 channels total)
 *   - These are usually mapped to Universe 0 in this system
 * - Art-Net and sACN (E1.31) network protocols support MULTIPLE universes
 *   - Art-Net can address up to 32,767 universes (organized as Net:SubNet:Universe)
 *   - This allows large lighting systems with thousands of fixtures
 * 
 * Usage:
 * - Fixtures specify which universe they're on via FHoloCadeDMXFixture.Universe
 * - FixtureService writes fixture data to this buffer (universe-agnostic)
 * - Controller reads from this buffer and flushes to the active transport (USB DMX or Art-Net)
 * - USB DMX transport typically only uses Universe 0, but the abstraction supports multiple
 * - Art-Net transport can send any universe number to network nodes
 * 
 * Example:
 *   - USB DMX: All fixtures on Universe 0, buffer stores 512 channels
 *   - Art-Net: Fixtures on Universe 0, 1, 2, etc. Buffer stores 512 channels per universe
 * 
 * This buffer is transport-agnostic - it's the core DMX data store used by all transports.
 */
class PROLIGHTING_API FUniverseBuffer
{
public:
	/** Ensure universe exists initialized with zeros */
	void EnsureUniverse(int32 Universe)
	{
		if (!UniverseToData.Contains(Universe))
		{
			TArray<uint8>& Data = UniverseToData.Add(Universe);
			Data.SetNumZeroed(512);
		}
	}

	/** Set a channel value (1-512). Clamps and ignores invalid channels */
	void SetChannel(int32 Universe, int32 Channel1Based, uint8 Value)
	{
		if (Channel1Based < 1 || Channel1Based > 512) return;
		EnsureUniverse(Universe);
		UniverseToData[Universe][Channel1Based - 1] = Value;
	}

	/** Get a channel value (1-512). Returns 0 if not present */
	uint8 GetChannel(int32 Universe, int32 Channel1Based) const
	{
		const TArray<uint8>* Data = UniverseToData.Find(Universe);
		if (!Data || Channel1Based < 1 || Channel1Based > 512) return 0;
		return (*Data)[Channel1Based - 1];
	}

	/** Get a const pointer to the 512-byte universe data (or nullptr if missing) */
	const TArray<uint8>* GetUniverse(int32 Universe) const
	{
		return UniverseToData.Find(Universe);
	}

	/** Enumerate universes */
	TArray<int32> GetUniverses() const
	{
		TArray<int32> Keys; UniverseToData.GetKeys(Keys); return Keys;
	}

	/** Clear all universes */
	void Reset() { UniverseToData.Reset(); }

private:
	TMap<int32, TArray<uint8>> UniverseToData;
};



