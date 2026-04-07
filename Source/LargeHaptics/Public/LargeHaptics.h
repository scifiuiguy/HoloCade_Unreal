// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * Large Haptics Module
 * 
 * Provides control systems for large-scale hydraulic haptic platforms.
 * This module enables developers to:
 * - Control individual hydraulic actuators for pitch and roll motion
 * - Control scissor lift for Y/Z translation
 * - Configure 5DOF moving platforms (single player standing)
 * - Configure 4DOF gunship platforms (4-player seated)
 * - Integrate motion profiles with VR experiences
 */
class FLargeHapticsModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};





