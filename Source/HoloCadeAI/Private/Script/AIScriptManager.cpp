// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "Script/AIScriptManager.h"
#include "AIHTTPClient.h"

UAIScriptManager::UAIScriptManager()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	
	bIsInitialized = false;
	bIsPlayingScript = false;
	CurrentScriptID = NAME_None;
}

void UAIScriptManager::BeginPlay()
{
	Super::BeginPlay();

	if (!HTTPClient)
	{
		HTTPClient = NewObject<UAIHTTPClient>(this, TEXT("HTTPClient"));
	}
}

void UAIScriptManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	// Generic script manager doesn't handle playback timing
	// Subclasses should override for experience-specific playback logic
}

bool UAIScriptManager::InitializeScriptManager(const FString& InAIServerBaseURL)
{
	if (bIsInitialized)
	{
		UE_LOG(LogTemp, Warning, TEXT("AIScriptManager: Already initialized"));
		return true;
	}

	AIServerBaseURL = InAIServerBaseURL;
	bIsInitialized = true;

	UE_LOG(LogTemp, Log, TEXT("AIScriptManager: Initialized with AI server URL: %s"), *AIServerBaseURL);
	
	return true;
}

bool UAIScriptManager::PlayScript(FName ScriptID)
{
	if (!bIsInitialized)
	{
		UE_LOG(LogTemp, Warning, TEXT("AIScriptManager: Cannot play script - not initialized"));
		return false;
	}

	if (!HasScript(ScriptID))
	{
		UE_LOG(LogTemp, Warning, TEXT("AIScriptManager: Script not found: %s"), *ScriptID.ToString());
		return false;
	}

	CurrentScriptID = ScriptID;
	bIsPlayingScript = true;
	
	RequestScriptPlayback(ScriptID);
	
	return true;
}

void UAIScriptManager::StopCurrentScript()
{
	if (!bIsPlayingScript)
	{
		return;
	}

	bIsPlayingScript = false;
	CurrentScriptID = NAME_None;
	
	UE_LOG(LogTemp, Log, TEXT("AIScriptManager: Stopped current script"));
}

void UAIScriptManager::PreBakeScript(FName ScriptID, bool bAsync)
{
	if (!bIsInitialized)
	{
		UE_LOG(LogTemp, Warning, TEXT("AIScriptManager: Cannot pre-bake script - not initialized"));
		return;
	}

	if (!HasScript(ScriptID))
	{
		UE_LOG(LogTemp, Warning, TEXT("AIScriptManager: Script not found: %s"), *ScriptID.ToString());
		return;
	}

	RequestScriptPreBake(ScriptID);
}

bool UAIScriptManager::HasScript(FName ScriptID) const
{
	return Scripts.Contains(ScriptID);
}

void UAIScriptManager::RequestScriptPlayback(FName ScriptID)
{
	// Generic implementation - subclasses should override
	UE_LOG(LogTemp, Log, TEXT("AIScriptManager: Requesting script playback for: %s"), *ScriptID.ToString());
	// TODO: Implement generic script playback request
}

void UAIScriptManager::RequestScriptPreBake(FName ScriptID)
{
	// Generic implementation - subclasses should override
	UE_LOG(LogTemp, Log, TEXT("AIScriptManager: Requesting script pre-bake for: %s"), *ScriptID.ToString());
	// TODO: Implement generic script pre-baking request
}

