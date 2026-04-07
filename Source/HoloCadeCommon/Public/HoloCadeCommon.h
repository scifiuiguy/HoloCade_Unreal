// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

// API macro for HoloCadeCommon module
#if UE_BUILD_DEVELOPMENT || UE_BUILD_DEBUG || UE_BUILD_TEST || UE_BUILD_SHIPPING
#define HOLOCADECOMMON_API __declspec(dllexport)
#else
#define HOLOCADECOMMON_API __declspec(dllimport)
#endif

DECLARE_LOG_CATEGORY_EXTERN(LogHoloCadeCommon, Log, All);

class FHoloCadeCommonModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

