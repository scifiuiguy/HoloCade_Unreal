// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "ProLighting/Public/RDMService.h"
#include "ProLighting/Public/ProLightingController.h"

void FRDMService::BridgeEvents(UProLightingController* Controller)
{
	if (!Controller)
	{
		return;
	}

	// Bridge RDM discovery events
	OnDiscoveredEvent().AddLambda([Controller](const FHoloCadeDiscoveredFixture& F)
	{
		Controller->OnFixtureDiscovered.Broadcast(F);
	});

	// Bridge RDM offline events
	OnWentOfflineEvent().AddLambda([Controller](int32 Id)
	{
		Controller->OnFixtureWentOffline.Broadcast(Id);
	});

	// Bridge RDM online events
	OnCameOnlineEvent().AddLambda([Controller](int32 Id)
	{
		Controller->OnFixtureCameOnline.Broadcast(Id);
	});
}












