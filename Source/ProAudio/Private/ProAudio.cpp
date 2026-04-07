// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "ProAudio.h"

DEFINE_LOG_CATEGORY(LogProAudio);

#define LOCTEXT_NAMESPACE "FProAudioModule"

void FProAudioModule::StartupModule()
{
	UE_LOG(LogProAudio, Log, TEXT("HoloCade ProAudio module loaded"));
}

void FProAudioModule::ShutdownModule()
{
	UE_LOG(LogProAudio, Log, TEXT("HoloCade ProAudio module unloaded"));
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FProAudioModule, ProAudio)

