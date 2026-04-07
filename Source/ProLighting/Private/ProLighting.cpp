// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "ProLighting.h"

DEFINE_LOG_CATEGORY(LogProLighting);

#define LOCTEXT_NAMESPACE "FProLightingModule"

void FProLightingModule::StartupModule()
{
	UE_LOG(LogProLighting, Log, TEXT("ProLighting module started!"));
}

void FProLightingModule::ShutdownModule()
{
	UE_LOG(LogProLighting, Log, TEXT("ProLighting module shut down!"));
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FProLightingModule, ProLighting)
