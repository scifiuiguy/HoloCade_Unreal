// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * HoloCade Examples Module
 * 
 * Contains example implementations showing how to use HoloCade APIs.
 * These are reference implementations for developers to learn from.
 */
class FExamplesModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

