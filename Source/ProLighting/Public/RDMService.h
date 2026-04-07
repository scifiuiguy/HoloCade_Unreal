// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "ProLighting.h"
#include "ProLightingTypes.h"
#include "IBridgeEvents.h"

/** Lightweight RDM management service (discovery cache + lifecycle). */
class PROLIGHTING_API FRDMService : public IBridgeEvents
{
public:
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnDiscoveredNative, const FHoloCadeDiscoveredFixture& /*Fixture*/);
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnWentOfflineNative, int32 /*VirtualFixtureID*/);
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnCameOnlineNative, int32 /*VirtualFixtureID*/);

	FRDMService() {}

	void Initialize(float InPollIntervalSeconds)
	{
		PollInterval = FMath::Max(0.1f, InPollIntervalSeconds);
		Accumulated = 0.0f;
	}

	void Tick(float DeltaTime)
	{
		Accumulated += DeltaTime;
		if (Accumulated >= PollInterval)
		{
			Accumulated = 0.0f;
			// Polling is driven externally (controller) for now; nothing here.
		}
	}

	// Add or update a discovered fixture (returns true if new)
	bool AddOrUpdate(const FHoloCadeDiscoveredFixture& Fixture)
	{
		bool bIsNew = !Discovered.Contains(Fixture.RDMUID);
		FHoloCadeDiscoveredFixture& Entry = Discovered.FindOrAdd(Fixture.RDMUID);
		Entry = Fixture;
		Entry.LastSeenTimestamp = FDateTime::Now();
		if (bIsNew)
		{
			OnDiscovered.Broadcast(Entry);
		}
		return bIsNew;
	}

	bool TryGet(const FString& RDMUID, FHoloCadeDiscoveredFixture& OutFixture) const
	{
		if (const FHoloCadeDiscoveredFixture* P = Discovered.Find(RDMUID))
		{
			OutFixture = *P; return true;
		}
		return false;
	}

    FHoloCadeDiscoveredFixture* FindMutable(const FString& RDMUID)
    {
        return Discovered.Find(RDMUID);
    }

	TArray<FHoloCadeDiscoveredFixture> GetAll() const
	{
		TArray<FHoloCadeDiscoveredFixture> Arr; Discovered.GenerateValueArray(Arr); return Arr;
	}

	void MarkOnline(const FString& RDMUID, int32 VirtualFixtureID)
	{
		if (FHoloCadeDiscoveredFixture* P = Discovered.Find(RDMUID))
		{
			bool WasOffline = !P->bIsOnline;
			P->bIsOnline = true;
			P->LastSeenTimestamp = FDateTime::Now();
			if (WasOffline) OnCameOnline.Broadcast(VirtualFixtureID);
		}
	}

	void MarkOffline(const FString& RDMUID, int32 VirtualFixtureID)
	{
		if (FHoloCadeDiscoveredFixture* P = Discovered.Find(RDMUID))
		{
			if (P->bIsOnline)
			{
				P->bIsOnline = false;
				OnWentOffline.Broadcast(VirtualFixtureID);
			}
		}
	}

	void Prune(float OfflineThresholdSeconds, float RemoveThresholdSeconds, TArray<int32>& OutWentOfflineVirtualIDs, TArray<FString>& OutRemovedUIDs, const TMap<FString,int32>& RDMToVirtual)
	{
		FDateTime Now = FDateTime::Now();
		TArray<FString> Keys; Discovered.GetKeys(Keys);
		for (const FString& UID : Keys)
		{
			FHoloCadeDiscoveredFixture& F = Discovered[UID];
			FTimespan Since = Now - F.LastSeenTimestamp;
			if (Since.GetTotalSeconds() > OfflineThresholdSeconds && F.bIsOnline)
			{
				F.bIsOnline = false;
				if (const int32* V = RDMToVirtual.Find(UID))
				{
					OutWentOfflineVirtualIDs.Add(*V);
					// Fire event for bridged delegates (consistent with MarkOffline behavior)
					OnWentOffline.Broadcast(*V);
				}
			}
			if (Since.GetTotalSeconds() > RemoveThresholdSeconds)
			{
				OutRemovedUIDs.Add(UID);
			}
		}
		for (const FString& UID : OutRemovedUIDs)
		{
			Discovered.Remove(UID);
		}
	}

	FOnDiscoveredNative& OnDiscoveredEvent() { return OnDiscovered; }
	FOnWentOfflineNative& OnWentOfflineEvent() { return OnWentOffline; }
	FOnCameOnlineNative& OnCameOnlineEvent() { return OnCameOnline; }

	// IBridgeEvents interface
	virtual void BridgeEvents(UProLightingController* Controller) override;

private:
	TMap<FString, FHoloCadeDiscoveredFixture> Discovered;
	float PollInterval = 0.5f;
	float Accumulated = 0.0f;
	FOnDiscoveredNative OnDiscovered;
	FOnWentOfflineNative OnWentOffline;
	FOnCameOnlineNative OnCameOnline;
};


