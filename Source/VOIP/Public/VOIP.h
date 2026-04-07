// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogVOIP, Log, All);

/**
 * HoloCade VOIP Module
 * 
 * Low-latency VOIP system with 3D HRTF spatialization for location-based entertainment.
 * Integrates Mumble (low-latency VOIP) with Steam Audio (3D HRTF spatialization).
 * 
 * Features:
 * - Mumble-based VOIP for LAN communication (low latency, high quality)
 * - Steam Audio 3D HRTF for spatial audio positioning
 * - Per-user audio sources with automatic spatialization
 * - Blueprint-friendly ActorComponent for easy integration
 * - Automatic connection management and audio routing
 * 
 * Architecture:
 * - VOIPManager: Main component (attach to HMD or player actor)
 * - MumbleClient: Wrapper around Mumble protocol
 * - SteamAudioSourceComponent: Per-user spatial audio source
 * 
 * Dependencies:
 * - Steam Audio plugin (git submodule)
 * - MumbleLink plugin (git submodule)
 * - Unreal replication system for player management
 */
class FVOIPModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};



