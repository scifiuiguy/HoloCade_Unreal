// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "Retail.h"

DEFINE_LOG_CATEGORY(LogRetail);

void FRetailModule::StartupModule()
{
	// Module startup code
	UE_LOG(LogRetail, Log, TEXT("Retail module started"));
}

void FRetailModule::ShutdownModule()
{
	// Module shutdown code
	UE_LOG(LogRetail, Log, TEXT("Retail module shut down"));
}

IMPLEMENT_MODULE(FRetailModule, Retail)
