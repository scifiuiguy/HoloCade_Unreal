// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogProAudio, Log, All);

/**
 * HoloCade ProAudio Module
 * 
 * Hardware-agnostic professional audio console control via OSC (Open Sound Control).
 * Supports Behringer X32/M32, Yamaha QL/CL/TF, Allen & Heath SQ/dLive, and more.
 * 
 * Uses Unreal Engine's built-in OSC plugin for native, dependency-free integration.
 */
class FProAudioModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

