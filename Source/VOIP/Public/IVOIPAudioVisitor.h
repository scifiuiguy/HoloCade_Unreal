// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "IVOIPAudioVisitor.generated.h"

/**
 * Visitor interface for VOIP audio events
 * 
 * Allows experience templates and custom components to subscribe to VOIP audio events
 * without creating direct dependencies between modules. This visitor pattern keeps the
 * VOIP module decoupled from experience-specific modules.
 * 
 * USAGE FOR EXPERIENCE TEMPLATES:
 * 
 * If you're building a custom experience template that needs to process player voice
 * (e.g., speech recognition, voice commands, audio analysis), implement this interface:
 * 
 * 1. Create a component in your experience template
 * 2. Implement IVOIPAudioVisitor interface
 * 3. Register with VOIPManager in your experience's InitializeExperienceImpl():
 *    ```cpp
 *    if (UVOIPManager* VOIPManager = FindComponentByClass<UVOIPManager>())
 *    {
 *        VOIPManager->RegisterAudioVisitor(MyAudioProcessorComponent);
 *    }
 *    ```
 * 4. Receive audio events via OnPlayerAudioReceived()
 * 
 * EXAMPLE IMPLEMENTATION:
 * ```cpp
 * class MYEXPERIENCE_API UMyAudioProcessor : public UActorComponent, public IVOIPAudioVisitor
 * {
 *     GENERATED_BODY()
 * 
 * public:
 *     virtual void OnPlayerAudioReceived(int32 PlayerId, const TArray<float>& AudioData, 
 *                                        int32 SampleRate, const FVector& Position) override
 *     {
 *         // Process audio for your custom use case
 *         ProcessVoiceCommand(AudioData, SampleRate);
 *     }
 * };
 * ```
 * 
 * BENEFITS:
 * - Decoupled architecture: VOIP module doesn't know about your experience
 * - Multiple visitors: Multiple components can subscribe to the same audio stream
 * - Clean separation: Your experience code stays in your experience module
 * - Reusable: Same pattern works for any experience template
 * 
 * See AIFacemaskExperience for a complete example using ASRManager.
 */
UINTERFACE(MinimalAPI, BlueprintType)
class UVOIPAudioVisitor : public UInterface
{
	GENERATED_BODY()
};

class VOIP_API IVOIPAudioVisitor
{
	GENERATED_BODY()

public:
	/**
	 * Called when player audio is received via VOIP/Mumble
	 * @param PlayerId - ID of the player who spoke
	 * @param AudioData - PCM audio data (decoded from Opus)
	 * @param SampleRate - Audio sample rate (typically 48000 for Mumble)
	 * @param Position - 3D position of the player (for spatial audio)
	 */
	virtual void OnPlayerAudioReceived(int32 PlayerId, const TArray<float>& AudioData, int32 SampleRate, const FVector& Position) = 0;
};

