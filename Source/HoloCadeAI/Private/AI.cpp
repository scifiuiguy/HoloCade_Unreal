// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "AI.h"

#define LOCTEXT_NAMESPACE "FHoloCadeAIModule"

void FHoloCadeAIModule::StartupModule()
{
	UE_LOG(LogTemp, Log, TEXT("HoloCade AI Module: Startup"));
}

void FHoloCadeAIModule::ShutdownModule()
{
	UE_LOG(LogTemp, Log, TEXT("HoloCade AI Module: Shutdown"));
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FHoloCadeAIModule, HoloCadeAI)




