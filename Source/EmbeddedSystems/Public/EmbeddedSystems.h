// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * Embedded Systems Module
 * 
 * Provides communication and control systems for embedded microcontrollers.
 * This module enables developers to:
 * - Communicate with Arduino, ESP32, STM32, Raspberry Pi, and Jetson devices
 * - Receive discrete and continuous button/trigger input
 * - Send discrete and continuous vibrator/kicker output
 * - Support wireless communication (ESP32 preferred)
 * - Integrate with narrative state machines
 * - Control embedded haptics in costumes and props
 */
class FEmbeddedSystemsModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};





