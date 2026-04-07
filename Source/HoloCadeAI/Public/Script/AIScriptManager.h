// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AIHTTPClient.h"
#include "AIScriptManager.generated.h"

// Forward declarations (must be at global scope for UHT)
class UAIHTTPClient;

/**
 * Generic script data structure (for use by generic AIScriptManager)
 * Subclasses can extend this with experience-specific fields
 * NOTE: USTRUCT must be at global scope (Unreal Engine limitation)
 */
USTRUCT(BlueprintType)
struct HOLOCADEAI_API FAIScript
{
	GENERATED_BODY()

	/** Script identifier/key (used for lookup) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Script")
	FName ScriptID;

	/** Human-readable description */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Script")
	FString Description;

	/** Text content for this script */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Script")
	FString TextContent;

	/** Whether script has been pre-baked */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Script")
	bool bIsPreBaked = false;

	/** Pre-baked data path (on server) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Script")
	FString PreBakedDataPath;

	FAIScript()
		: ScriptID(NAME_None)
		, Description(TEXT(""))
		, TextContent(TEXT(""))
		, bIsPreBaked(false)
	{}
};

/**
 * Generic Script Manager Component
 * 
 * Base class for managing AI scripts (text-to-speech, audio-to-face, etc.).
 * Provides generic script management without experience-specific logic.
 * 
 * Subclasses should extend this for experience-specific needs:
 * - Narrative state machine integration
 * - Experience-specific script structures
 * - Experience-specific playback triggers
 * 
 * WORKFLOW:
 * 1. Define scripts (text content + settings)
 * 2. Pre-bake scripts on AI server (TTS → Audio, Audio → Facial data)
 * 3. Play scripts by ID/key
 * 4. AI server streams pre-baked data
 * 
 * NOTE: UCLASS must be at global scope (Unreal Engine limitation)
 */
UCLASS(ClassGroup=(HoloCade), meta=(BlueprintSpawnableComponent))
class HOLOCADEAI_API UAIScriptManager : public UActorComponent
{
	GENERATED_BODY()

public:
	UAIScriptManager();

	/** AI server base URL (e.g., "http://192.168.1.100:8000") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Script")
	FString AIServerBaseURL;

	/** Currently playing script ID (if any) */
	UPROPERTY(BlueprintReadOnly, Category = "AI|Script")
	FName CurrentScriptID = NAME_None;

	/** Whether a script is currently playing */
	UPROPERTY(BlueprintReadOnly, Category = "AI|Script")
	bool bIsPlayingScript = false;

	/**
	 * Initialize the script manager
	 * @param InAIServerBaseURL - Base URL for AI server
	 * @return true if initialization was successful
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|Script")
	virtual bool InitializeScriptManager(const FString& InAIServerBaseURL);

	/**
	 * Play a script by ID
	 * @param ScriptID - Script identifier to play
	 * @return true if script was found and started
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|Script")
	virtual bool PlayScript(FName ScriptID);

	/**
	 * Stop the currently playing script
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|Script")
	virtual void StopCurrentScript();

	/**
	 * Pre-bake a script (convert text to audio/facial data)
	 * @param ScriptID - Script identifier to pre-bake
	 * @param bAsync - If true, pre-baking happens asynchronously
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|Script")
	virtual void PreBakeScript(FName ScriptID, bool bAsync = true);

	/**
	 * Check if a script exists
	 * @param ScriptID - Script identifier to check
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI|Script")
	virtual bool HasScript(FName ScriptID) const;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Whether the script manager is initialized */
	bool bIsInitialized = false;

	/** HTTP client for AI server communication */
	UPROPERTY()
	TObjectPtr<UAIHTTPClient> HTTPClient;

	/** Scripts registry (subclasses can extend with experience-specific structures) */
	UPROPERTY()
	TMap<FName, FAIScript> Scripts;

	/**
	 * Request script playback from AI server
	 * Subclasses can override for custom playback logic
	 * @param ScriptID - Script identifier to play
	 */
	virtual void RequestScriptPlayback(FName ScriptID);

	/**
	 * Request script pre-baking from AI server
	 * Subclasses can override for custom pre-baking logic
	 * @param ScriptID - Script identifier to pre-bake
	 */
	virtual void RequestScriptPreBake(FName ScriptID);
};
