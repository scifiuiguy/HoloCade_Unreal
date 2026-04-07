// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "RF433MHz.h"

DEFINE_LOG_CATEGORY(LogRF433MHz);

void FRF433MHzModule::StartupModule()
{
	UE_LOG(LogRF433MHz, Log, TEXT("RF433MHz module started"));
}

void FRF433MHzModule::ShutdownModule()
{
	UE_LOG(LogRF433MHz, Log, TEXT("RF433MHz module shut down"));
}

IMPLEMENT_MODULE(FRF433MHzModule, RF433MHz)

