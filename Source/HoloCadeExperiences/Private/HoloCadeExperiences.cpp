// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "HoloCadeExperiences.h"

DEFINE_LOG_CATEGORY(LogGoKart);
DEFINE_LOG_CATEGORY(LogSuperheroFlight);

#define LOCTEXT_NAMESPACE "FHoloCadeExperiencesModule"

void FHoloCadeExperiencesModule::StartupModule()
{
	UE_LOG(LogTemp, Log, TEXT("HoloCade Experiences Module: Startup"));
}

void FHoloCadeExperiencesModule::ShutdownModule()
{
	UE_LOG(LogTemp, Log, TEXT("HoloCade Experiences Module: Shutdown"));
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FHoloCadeExperiencesModule, HoloCadeExperiences)




