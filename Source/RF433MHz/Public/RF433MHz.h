// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogRF433MHz, Log, All);

/**
 * RF433MHz Module
 * 
 * Low-level API for 433MHz wireless remote/receiver integration.
 * Provides abstraction layer for different USB receiver modules (RTL-SDR, CC1101, RFM69, Generic)
 * with rolling code validation and replay attack prevention.
 */
class FRF433MHzModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

