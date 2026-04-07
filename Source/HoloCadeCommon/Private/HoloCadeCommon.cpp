// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "HoloCadeCommon.h"

DEFINE_LOG_CATEGORY(LogHoloCadeCommon);

#define LOCTEXT_NAMESPACE "FHoloCadeCommonModule"

void FHoloCadeCommonModule::StartupModule()
{
	UE_LOG(LogHoloCadeCommon, Log, TEXT("HoloCade Common Module: Startup"));
}

void FHoloCadeCommonModule::ShutdownModule()
{
	UE_LOG(LogHoloCadeCommon, Log, TEXT("HoloCade Common Module: Shutdown"));
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FHoloCadeCommonModule, HoloCadeCommon)

