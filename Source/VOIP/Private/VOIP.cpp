// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "VOIP.h"

DEFINE_LOG_CATEGORY(LogVOIP);

#define LOCTEXT_NAMESPACE "FVOIPModule"

void FVOIPModule::StartupModule()
{
	UE_LOG(LogVOIP, Log, TEXT("HoloCade VOIP module loaded"));
	UE_LOG(LogVOIP, Log, TEXT("VOIP: Mumble + Steam Audio integration ready"));
}

void FVOIPModule::ShutdownModule()
{
	UE_LOG(LogVOIP, Log, TEXT("HoloCade VOIP module unloaded"));
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FVOIPModule, VOIP)



