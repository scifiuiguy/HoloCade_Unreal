// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "ProLighting.h"

// Forward declaration
class UProLightingController;

/**
 * IBridgeEvents
 * Interface for services that can bridge their native events to Blueprint delegates in the controller.
 * This eliminates if-chains in BridgeServiceEvents() by allowing each service to handle its own bridging logic.
 */
class PROLIGHTING_API IBridgeEvents
{
public:
	virtual ~IBridgeEvents() = default;
	
	/**
	 * Bridge this service's native events to the controller's Blueprint delegates.
	 * @param Controller The controller to bridge events to (must be non-null)
	 */
	virtual void BridgeEvents(UProLightingController* Controller) = 0;
};












