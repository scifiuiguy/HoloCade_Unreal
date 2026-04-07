// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Script/AIScriptManager.h"
#include "AIFacemaskScript.h"  // Script structures (still in AI module)
#include "AIFacemaskScriptManager.generated.h"

// Forward declarations
class UAIFacemaskFaceController;

/**
 * Delegate for script playback events (facemask-specific)
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAIFacemaskScriptStarted, FName, StateName, const FAIFacemaskScript&, Script);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnAIFacemaskScriptLineStarted, FName, StateName, int32, LineIndex, const FAIFacemaskScriptLine&, ScriptLine);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAIFacemaskScriptFinished, FName, StateName, const FAIFacemaskScript&, Script);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAIFacemaskScriptPreBakeComplete, FName, StateName);

/**
 * AIFacemask Script Manager Component
 * 
 * Facemask-specific script manager that extends UAIScriptManager.
 * Adds narrative state machine integration and facemask-specific script structures.
 * 
 * Inherits from UAIScriptManager for generic script management.
 * Adds:
 * - Narrative state machine integration
 * - Facemask-specific script structures (FAIFacemaskScript)
 * - Face controller integration
 * - Experience-specific delegates
 */
UCLASS(ClassGroup=(HoloCade), meta=(BlueprintSpawnableComponent))
class HOLOCADEEXPERIENCES_API UAIFacemaskScriptManager : public UAIScriptManager
{
	GENERATED_BODY()

public:
	UAIFacemaskScriptManager();

	/** Script collection for this experience */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|AIFacemask Script")
	FAIFacemaskScriptCollection ScriptCollection;

	/** NVIDIA ACE server base URL (e.g., "http://192.168.1.100:8000") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|AIFacemask Script")
	FString ACEServerBaseURL;

	/** Whether to auto-trigger scripts on narrative state changes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|AIFacemask Script")
	bool bAutoTriggerOnStateChange = true;

	/** Currently playing script (if any) */
	UPROPERTY(BlueprintReadOnly, Category = "HoloCade|AIFacemask Script")
	FAIFacemaskScript CurrentScript;

	/** Current script line index being played */
	UPROPERTY(BlueprintReadOnly, Category = "HoloCade|AIFacemask Script")
	int32 CurrentScriptLineIndex = -1;

	/** Event fired when a script starts playing */
	UPROPERTY(BlueprintAssignable, Category = "HoloCade|AIFacemask Script")
	FOnAIFacemaskScriptStarted OnScriptStarted;

	/** Event fired when a script line starts playing */
	UPROPERTY(BlueprintAssignable, Category = "HoloCade|AIFacemask Script")
	FOnAIFacemaskScriptLineStarted OnScriptLineStarted;

	/** Event fired when a script finishes playing */
	UPROPERTY(BlueprintAssignable, Category = "HoloCade|AIFacemask Script")
	FOnAIFacemaskScriptFinished OnScriptFinished;

	/** Event fired when script pre-baking completes */
	UPROPERTY(BlueprintAssignable, Category = "HoloCade|AIFacemask Script")
	FOnAIFacemaskScriptPreBakeComplete OnScriptPreBakeComplete;

	// Override generic base class methods
	virtual bool InitializeScriptManager(const FString& InAIServerBaseURL) override;
	virtual bool PlayScript(FName ScriptID) override;
	virtual void StopCurrentScript() override;
	virtual void PreBakeScript(FName ScriptID, bool bAsync = true) override;
	virtual bool HasScript(FName ScriptID) const override;

	/**
	 * Trigger a script for a specific narrative state
	 * @param StateName - Narrative state name to trigger script for
	 * @return true if script was found and triggered
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|AIFacemask Script")
	bool TriggerScriptForState(FName StateName);

	/**
	 * Pre-bake all scripts in the collection on the ACE server
	 * @param bAsync - If true, pre-baking happens asynchronously
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|AIFacemask Script")
	void PreBakeAllScripts(bool bAsync = true);

	/**
	 * Pre-bake a specific script for a state
	 * @param StateName - State name to pre-bake script for
	 * @param bAsync - If true, pre-baking happens asynchronously
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|AIFacemask Script")
	void PreBakeScriptForState(FName StateName, bool bAsync = true);

	/**
	 * Get script for a specific state
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|AIFacemask Script")
	FAIFacemaskScript GetScriptForState(FName StateName) const;

	/**
	 * Handle narrative state change (called by experience base)
	 * @param OldState - Previous state name
	 * @param NewState - New state name
	 * @param NewStateIndex - Index of new state
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|AIFacemask Script")
	void HandleNarrativeStateChanged(FName OldState, FName NewState, int32 NewStateIndex);

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Override generic base class protected methods
	virtual void RequestScriptPlayback(FName ScriptID) override;
	virtual void RequestScriptPreBake(FName ScriptID) override;

private:
	/** Timer for script playback */
	float ScriptPlaybackTimer = 0.0f;

	/** Current script line start time */
	float CurrentScriptLineStartTime = 0.0f;

	/** Whether we're waiting for script start delay */
	bool bWaitingForStartDelay = false;

	/** Start delay timer */
	float StartDelayTimer = 0.0f;

	/**
	 * Start playing a script line
	 */
	void StartScriptLine(int32 LineIndex);

	/**
	 * Advance to next script line (or finish script)
	 */
	void AdvanceToNextScriptLine();

	/**
	 * Finish current script
	 */
	void FinishCurrentScript();

	/**
	 * Request script playback from ACE server (facemask-specific)
	 */
	void RequestScriptPlaybackFromACE(const FAIFacemaskScript& Script, int32 StartLineIndex = 0);

	/**
	 * Request script pre-baking from ACE server (facemask-specific)
	 */
	void RequestScriptPreBakeFromACE(const FAIFacemaskScript& Script);

	/**
	 * Request TTS conversion for a script line
	 */
	void RequestTTSConversion(const FAIFacemaskScriptLine& ScriptLine, TFunction<void(const FString& AudioFilePath, float Duration)> Callback);

	/**
	 * Request Audio2Face conversion for a script line
	 */
	void RequestAudio2FaceConversion(const FAIFacemaskScriptLine& ScriptLine, const FString& AudioFilePath, TFunction<void(bool bSuccess)> Callback);

	/** Reference to AIFacemaskFaceController for streaming facial animation */
	UPROPERTY()
	TObjectPtr<UAIFacemaskFaceController> FaceController;

	/** Scripts currently being pre-baked (for async tracking) */
	TMap<FName, bool> ScriptsBeingPreBaked;

	/**
	 * Recursively pre-bake script lines (TTS → Audio, then Audio → Facial data)
	 */
	void PreBakeScriptLineRecursive(const FAIFacemaskScript& Script, int32 LineIndex);

	/**
	 * Get voice type as string for API requests
	 */
	FString GetVoiceTypeString(EHoloCadeACEVoiceType VoiceType) const;

	/**
	 * Get emotion preset as string for API requests
	 */
	FString GetEmotionPresetString(EHoloCadeACEEmotionPreset EmotionPreset) const;
};

