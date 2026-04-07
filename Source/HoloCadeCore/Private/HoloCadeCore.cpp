// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "HoloCadeCore.h"

#define LOCTEXT_NAMESPACE "FHoloCadeCoreModule"

void FHoloCadeCoreModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	UE_LOG(LogTemp, Log, TEXT("HoloCade Core Module: Startup"));
}

void FHoloCadeCoreModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	UE_LOG(LogTemp, Log, TEXT("HoloCade Core Module: Shutdown"));
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FHoloCadeCoreModule, HoloCadeCore)



