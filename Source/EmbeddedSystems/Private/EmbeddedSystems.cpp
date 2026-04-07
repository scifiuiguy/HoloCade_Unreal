// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "EmbeddedSystems.h"

#define LOCTEXT_NAMESPACE "FEmbeddedSystemsModule"

void FEmbeddedSystemsModule::StartupModule()
{
	UE_LOG(LogTemp, Log, TEXT("HoloCade EmbeddedSystems Module: Startup"));
}

void FEmbeddedSystemsModule::ShutdownModule()
{
	UE_LOG(LogTemp, Log, TEXT("HoloCade EmbeddedSystems Module: Shutdown"));
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FEmbeddedSystemsModule, EmbeddedSystems)





