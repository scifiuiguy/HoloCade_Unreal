// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "AIFacemask/AIFacemaskASRManager.h"
#include "AIFacemask/AIFacemaskImprovManager.h"

UAIFacemaskASRManager::UAIFacemaskASRManager()
{
	// Initialize facemask-specific members
	// Base class handles all initialization
}

void UAIFacemaskASRManager::BeginPlay()
{
	Super::BeginPlay();
	
	// Find AIFacemaskImprovManager component on the same actor
	AActor* Owner = GetOwner();
	if (Owner)
	{
		ImprovManager = Owner->FindComponentByClass<UAIFacemaskImprovManager>();
		if (!ImprovManager)
		{
			UE_LOG(LogTemp, Warning, TEXT("UAIFacemaskASRManager: No UAIFacemaskImprovManager found on owner actor. Auto-trigger improv will be disabled."));
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("UAIFacemaskASRManager: Found UAIFacemaskImprovManager - auto-trigger improv enabled"));
		}
	}
}

void UAIFacemaskASRManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	// Base class handles all timing logic (voice activity detection, etc.)
	// No facemask-specific timing needed
}

bool UAIFacemaskASRManager::InitializeASRManager()
{
	// Copy facemask config to base config
	ASRConfig = FacemaskASRConfig.BaseConfig;
	
	// Initialize base class
	bool bSuccess = Super::InitializeASRManager();
	
	if (bSuccess)
	{
		UE_LOG(LogTemp, Log, TEXT("UAIFacemaskASRManager: Initialized with auto-trigger improv: %s"), 
			FacemaskASRConfig.bAutoTriggerImprov ? TEXT("enabled") : TEXT("disabled"));
	}
	
	return bSuccess;
}

void UAIFacemaskASRManager::HandleTranscriptionResult(int32 SourceId, const FString& TranscribedText)
{
	// Call base class first (broadcasts event)
	Super::HandleTranscriptionResult(SourceId, TranscribedText);
	
	// Auto-trigger improv if enabled and we have transcribed text
	if (FacemaskASRConfig.bAutoTriggerImprov && ImprovManager && !TranscribedText.IsEmpty())
	{
		UE_LOG(LogTemp, Log, TEXT("UAIFacemaskASRManager: Auto-triggering improv with transcribed text: '%s'"), *TranscribedText);
		ImprovManager->GenerateAndPlayImprovResponse(TranscribedText, true);
	}
	else if (FacemaskASRConfig.bAutoTriggerImprov && !ImprovManager)
	{
		UE_LOG(LogTemp, Warning, TEXT("UAIFacemaskASRManager: Auto-trigger improv enabled but ImprovManager not found. Ensure UAIFacemaskImprovManager is on the same actor."));
	}
}
