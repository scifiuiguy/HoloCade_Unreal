// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "AIAPI.h"  // For HOLOCADEAI_API macro

/**
 * HoloCadeAI Module
 * 
 * Low-level AI API for all generative AI capabilities in HoloCade.
 * This module provides:
 * - LLM providers (Ollama, OpenAI-compatible, NVIDIA NIM)
 * - ASR providers (NVIDIA Riva, Parakeet, Canary, Whisper)
 * - TTS providers (NVIDIA Riva, etc.)
 * - Audio2Face integration (generic, not mask-specific)
 * - Container management for Docker-based AI services
 * - HTTP/gRPC clients for AI service communication
 * 
 * Future capabilities:
 * - Text-to-Image (TTI)
 * - Text-to-Video (TTV)
 * - Other generative AI models
 */
class HOLOCADEAI_API FHoloCadeAIModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};




