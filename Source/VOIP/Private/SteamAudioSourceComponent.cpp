// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "SteamAudioSourceComponent.h"
#include "VOIP.h"
#include "AudioDevice.h"
#include "Sound/SoundWave.h"
#include "Engine/Engine.h"

USteamAudioSourceComponent::USteamAudioSourceComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Volume = 1.0f;
	CurrentRemotePosition = FVector::ZeroVector;
}

USteamAudioSourceComponent::~USteamAudioSourceComponent()
{
	CleanupSteamAudio();
}

void USteamAudioSourceComponent::BeginPlay()
{
	Super::BeginPlay();

	// Initialize Steam Audio
	if (!InitializeSteamAudio())
	{
		UE_LOG(LogVOIP, Error, TEXT("SteamAudioSourceComponent: Failed to initialize Steam Audio plugin"));
	}
}

void USteamAudioSourceComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CleanupSteamAudio();
	Super::EndPlay(EndPlayReason);
}

void USteamAudioSourceComponent::ProcessAudioData(const TArray<uint8>& OpusData, const FVector& RemotePosition)
{
	CurrentRemotePosition = RemotePosition;

	// Decode Opus to PCM
	TArray<float> PCMData;
	if (!DecodeOpus(OpusData, PCMData))
	{
		UE_LOG(LogVOIP, Warning, TEXT("SteamAudioSourceComponent: Failed to decode Opus data"));
		return;
	}

	// Get listener position (HMD)
	FVector ListenerPosition = FVector::ZeroVector;
	FRotator ListenerRotation = FRotator::ZeroRotator;

	// TODO: Get from HMD interface or player controller
	// For now, use component owner position
	if (AActor* Owner = GetOwner())
	{
		ListenerPosition = Owner->GetActorLocation();
		ListenerRotation = Owner->GetActorRotation();
	}

	// Process through Steam Audio HRTF
	TArray<float> BinauralData;
	if (!ProcessHRTF(PCMData, RemotePosition, ListenerPosition, ListenerRotation, BinauralData))
	{
		UE_LOG(LogVOIP, Warning, TEXT("SteamAudioSourceComponent: Failed to process HRTF"));
		return;
	}

	// Play binaural audio
	PlayBinauralAudio(BinauralData, 48000); // Mumble uses 48kHz
}

void USteamAudioSourceComponent::UpdatePosition(const FVector& RemotePosition, const FVector& ListenerPosition, const FRotator& ListenerRotation)
{
	CurrentRemotePosition = RemotePosition;

	// TODO: Update Steam Audio source position
	// if (SteamAudioSource)
	// {
	//     // Update source transform in Steam Audio
	//     SteamAudioSource->SetPosition(RemotePosition);
	//     SteamAudioSource->SetListenerTransform(ListenerPosition, ListenerRotation);
	// }
}

void USteamAudioSourceComponent::SetVolume(float NewVolume)
{
	Volume = FMath::Clamp(NewVolume, 0.0f, 1.0f);
	
	// TODO: Update Steam Audio source volume
	// if (SteamAudioSource)
	// {
	//     SteamAudioSource->SetVolume(Volume);
	// }
}

bool USteamAudioSourceComponent::DecodeOpus(const TArray<uint8>& OpusData, TArray<float>& OutPCMData)
{
	// TODO: Decode Opus using MumbleLink plugin or Opus library
	// This will be implemented when MumbleLink submodule is added
	// 
	// Example:
	// if (MumbleLinkInterface)
	// {
	//     return MumbleLinkInterface->DecodeOpus(OpusData, OutPCMData);
	// }

	UE_LOG(LogVOIP, Warning, TEXT("SteamAudioSourceComponent: Opus decoding not yet implemented. Using placeholder."));
	return false; // Placeholder
}

bool USteamAudioSourceComponent::ProcessHRTF(const TArray<float>& PCMData, const FVector& SourcePosition,
	const FVector& ListenerPosition, const FRotator& ListenerRotation, TArray<float>& OutBinauralData)
{
	// TODO: Process through Steam Audio HRTF
	// This will be implemented when Steam Audio submodule is added
	// 
	// Example:
	// if (SteamAudioSource)
	// {
	//     return SteamAudioSource->ProcessHRTF(PCMData, SourcePosition, 
	//         ListenerPosition, ListenerRotation, OutBinauralData);
	// }

	UE_LOG(LogVOIP, Warning, TEXT("SteamAudioSourceComponent: Steam Audio HRTF processing not yet implemented. Using placeholder."));
	return false; // Placeholder
}

void USteamAudioSourceComponent::PlayBinauralAudio(const TArray<float>& BinauralData, int32 SampleRate)
{
	// TODO: Play binaural audio via Unreal's audio system
	// This will create a procedural sound wave and play it
	// 
	// Example:
	// USoundWave* SoundWave = NewObject<USoundWave>(this);
	// SoundWave->RawData.Lock(LOCK_READ_WRITE);
	// FMemory::Memcpy(SoundWave->RawData.Realloc(BinauralData.Num() * sizeof(float)), 
	//     BinauralData.GetData(), BinauralData.Num() * sizeof(float));
	// SoundWave->RawData.Unlock();
	// SoundWave->SampleRate = SampleRate;
	// SoundWave->NumChannels = 2; // Binaural = stereo
	// 
	// // Play via audio component
	// UAudioComponent* AudioComponent = NewObject<UAudioComponent>(this);
	// AudioComponent->SetSound(SoundWave);
	// AudioComponent->SetVolumeMultiplier(Volume);
	// AudioComponent->Play();

	UE_LOG(LogVOIP, Warning, TEXT("SteamAudioSourceComponent: Audio playback not yet implemented. Using placeholder."));
}

bool USteamAudioSourceComponent::InitializeSteamAudio()
{
	// TODO: Initialize Steam Audio plugin
	// This will be implemented when Steam Audio submodule is added
	// 
	// Example:
	// IPluginManager& PluginManager = IPluginManager::Get();
	// TSharedPtr<IPlugin> SteamAudioPlugin = PluginManager.FindPlugin("SteamAudio");
	// if (SteamAudioPlugin && SteamAudioPlugin->IsEnabled())
	// {
	//     // Get interface from plugin
	//     // Create Steam Audio source
	//     SteamAudioSource = ...;
	//     return SteamAudioSource != nullptr;
	// }

	UE_LOG(LogVOIP, Warning, TEXT("SteamAudioSourceComponent: Steam Audio plugin not yet integrated. Using placeholder."));
	return true; // Placeholder - return true for now
}

void USteamAudioSourceComponent::CleanupSteamAudio()
{
	// TODO: Cleanup Steam Audio source
	// if (SteamAudioSource)
	// {
	//     // Destroy source
	//     SteamAudioSource = nullptr;
	// }
}

