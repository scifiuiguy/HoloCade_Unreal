// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Improv/AIImprovManager.h"
#include "AIFacemaskScript.h"  // Script structures (still in AI module)
#include "AIFacemaskImprovManager.generated.h"

// Forward declarations
class UAIFacemaskFaceController;

/**
 * Usage state for improv responses (queued → spoken when face starts speaking)
 */
UENUM(BlueprintType)
enum class EImprovResponseState : uint8
{
	/** Response is queued (generated but not yet spoken) */
	Queued UMETA(DisplayName = "Queued"),
	
	/** Response is being spoken (face animation has started) */
	Spoken UMETA(DisplayName = "Spoken")
};

/**
 * Facemask-specific configuration for improvised responses
 */
USTRUCT(BlueprintType)
struct HOLOCADEEXPERIENCES_API FAIFacemaskImprovConfig
{
	GENERATED_BODY()

	/** Base improv config (inherited from generic) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|AIFacemask Improv")
	FAIImprovConfig BaseConfig;

	/** Voice type for text-to-speech conversion */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|AIFacemask Improv")
	EHoloCadeACEVoiceType VoiceType = EHoloCadeACEVoiceType::Default;

	/** Custom voice model ID (if VoiceType is Custom) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|AIFacemask Improv")
	FString CustomVoiceModelID;

	FAIFacemaskImprovConfig()
		: VoiceType(EHoloCadeACEVoiceType::Default)
	{}
};

/**
 * AIFacemask Improv Manager Component
 * 
 * Facemask-specific improv manager that extends UAIImprovManager.
 * Adds face controller integration for streaming facial animation.
 * 
 * Inherits from UAIImprovManager for generic LLM + TTS + Audio2Face pipeline.
 * Adds:
 * - Face controller integration
 * - Facemask-specific voice/emotion settings
 * - Experience-specific response formatting
 */
UCLASS(ClassGroup=(HoloCade), meta=(BlueprintSpawnableComponent))
class HOLOCADEEXPERIENCES_API UAIFacemaskImprovManager : public UAIImprovManager
{
	GENERATED_BODY()

public:
	UAIFacemaskImprovManager();

	/** Facemask-specific configuration for improvised responses */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|AIFacemask Improv")
	FAIFacemaskImprovConfig FacemaskImprovConfig;

	// Override generic base class methods
	virtual bool InitializeImprovManager() override;
	virtual void GenerateAndPlayImprovResponse(const FString& Input, bool bAsync = true) override;
	virtual void StopCurrentResponse() override;

	/**
	 * Get current AI response (for HUD display)
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|AIFacemask Improv")
	FString GetCurrentAIResponse() const { return CurrentAIResponse; }

	/**
	 * Get current AI response usage state (queued or spoken)
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|AIFacemask Improv")
	EImprovResponseState GetCurrentAIResponseState() const { return CurrentAIResponseState; }

	/**
	 * Mark current AI response as spoken (called when face animation starts)
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|AIFacemask Improv")
	void MarkCurrentResponseAsSpoken();

	/**
	 * Phase 11: Notify of narrative state change (for transition buffering)
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|AIFacemask Improv")
	void NotifyNarrativeStateChanged(FName OldState, FName NewState, int32 NewStateIndex);

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Override generic base class protected methods
	virtual void RequestLLMResponseAsync(const FString& Input, const FString& SystemPrompt, const TArray<FString>& InConversationHistory) override;
	virtual void RequestTTSConversion(const FString& Text) override;
	virtual void RequestAudio2FaceConversion(const FString& AudioFilePath) override;
	virtual void OnTTSConversionComplete(const FString& AudioFilePath, const TArray<uint8>& AudioData) override;
	virtual void OnAudio2FaceConversionComplete(bool bSuccess) override;

	/**
	 * Convert voice type enum to voice name string for TTS
	 */
	FString GetVoiceNameString(EHoloCadeACEVoiceType VoiceType) const;

	/** Reference to AIFacemaskFaceController for streaming facial animation */
	UPROPERTY()
	TObjectPtr<UAIFacemaskFaceController> FaceController;

	// NOTE: GetBufferedTransition and IsTransitionReady are inherited from base class
	// (not overridden - base class implementation is sufficient)

protected:
	/** Current AI response usage state (queued → spoken when face starts) */
	UPROPERTY(BlueprintReadOnly, Category = "HoloCade|AIFacemask Improv")
	EImprovResponseState CurrentAIResponseState = EImprovResponseState::Queued;

	// Base class handles transition LLM call by default - no override needed
	// (Can override RequestTransitionSentence if facemask needs custom LLM config)

private:
	/** Phase 11: Reference to ScriptManager for querying narrative state (facemask-specific) */
	UPROPERTY()
	TObjectPtr<class UAIFacemaskScriptManager> ScriptManager;
};

