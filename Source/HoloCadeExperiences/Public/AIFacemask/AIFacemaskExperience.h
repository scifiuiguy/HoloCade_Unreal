// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "HoloCadeExperienceBase.h"
#include "Networking/HoloCadeServerBeacon.h"
#include "AIFacemaskExperience.generated.h"

// Forward declarations
class UAIFacemaskFaceController;
class UEmbeddedDeviceController;
class UAIFacemaskScriptManager;
class UAIFacemaskImprovManager;
class UAIFacemaskASRManager;

/**
 * AI Facemask Experience Template
 * 
 * Pre-configured experience for LAN multiplayer VR with immersive theater live actors.
 * 
 * NETWORK ARCHITECTURE (REQUIRED):
 * This experience REQUIRES a dedicated server setup:
 * - Separate local PC running headless dedicated server
 * - Same PC runs NVIDIA ACE pipeline: Audio → NLU → Emotion → Facial Animation
 * - NVIDIA ACE streams facial textures and blend shapes to HMDs over network
 * - Offloads AI processing from HMDs for optimal performance
 * - Supports parallelization for multiple live actors
 * 
 * ServerMode is ENFORCED to DedicatedServer - attempting to use Listen Server will fail.
 * 
 * AI FACIAL ANIMATION:
 * - Fully automated by NVIDIA ACE - NO manual control, keyframe animation, or rigging
 * - Live actor wears HMD with AIFace mesh tracked on top of their face (like a mask)
 * - NVIDIA ACE determines facial expressions based on:
 *   - Audio track (speech recognition)
 *   - NLU (natural language understanding)
 *   - Emotion detection
 *   - State machine context
 * - AIFaceController receives NVIDIA ACE output and applies it to mesh in real-time
 * 
 * LIVE ACTOR CONTROLS:
 * - Live actors wear wrist-mounted button controls (4 buttons: 2 left, 2 right)
 * - Buttons control the Experience Loop state machine (NOT facial animation)
 * - Live actor directs experience flow, AI face handles expressions autonomously
 * 
 * Button Layout:
 * - Left Wrist:  Button 0 (Forward), Button 1 (Backward)
 * - Right Wrist: Button 2 (Forward), Button 3 (Backward)
 * 
 * Perfect for interactive theater, escape rooms, and narrative-driven LBE experiences
 * requiring professional performers to guide players through story beats.
 */
UCLASS(Blueprintable, ClassGroup=(HoloCade))
class HOLOCADEEXPERIENCES_API AAIFacemaskExperience : public AHoloCadeExperienceBase
{
	GENERATED_BODY()
	
public:
	AAIFacemaskExperience();

	/** Reference to the live actor's skeletal mesh for facial animation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|AI Facemask")
	TObjectPtr<USkeletalMeshComponent> LiveActorMesh;

	/** AI Face controller component (autonomous, driven by Neural Face) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HoloCade|AI Facemask|Components")
	TObjectPtr<UAIFacemaskFaceController> FaceController;

	/** Embedded systems controller for wireless trigger buttons embedded in costume/clothes */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HoloCade|AI Facemask|Components")
	TObjectPtr<UEmbeddedDeviceController> CostumeController;

	/** Script Manager for pre-baked script collections and automatic script triggering */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HoloCade|AI Facemask|Components")
	TObjectPtr<UAIFacemaskScriptManager> ScriptManager;

	/** Improv Manager for real-time improvised responses (local LLM + TTS + Audio2Face) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HoloCade|AI Facemask|Components")
	TObjectPtr<UAIFacemaskImprovManager> ImprovManager;

	/** ASR Manager for converting player voice to text (Automatic Speech Recognition) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HoloCade|AI Facemask|Components")
	TObjectPtr<UAIFacemaskASRManager> ASRManager;

	/** Live Actor HUD Component (stereo VR HUD overlay for live actors) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HoloCade|AI Facemask|Components")
	TObjectPtr<class UAIFacemaskLiveActorHUDComponent> LiveActorHUD;

	/** Server beacon for automatic discovery/connection */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HoloCade|AI Facemask|Components")
	TObjectPtr<UHoloCadeServerBeacon> ServerBeacon;

	/** Enable passthrough for live actors to help players */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|AI Facemask|Config")
	bool bEnableLiveActorPassthrough = true;

	/** Number of live actor roles in this experience */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|AI Facemask|Config", meta = (ClampMin = "1", ClampMax = "4"))
	int32 NumberOfLiveActors = 1;

	/** Number of player roles in this experience */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|AI Facemask|Config", meta = (ClampMin = "1", ClampMax = "8"))
	int32 NumberOfPlayers = 1;

	/**
	 * Get the current narrative state (from base class narrative state machine)
	 * This is the same as GetCurrentNarrativeState() from the base class
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|AI Facemask|Narrative")
	FName GetCurrentExperienceState() const { return GetCurrentNarrativeState(); }

	/**
	 * Request to advance the narrative state (input-agnostic, works with any input source)
	 * Call this from any input source (EmbeddedSystems, VR controllers, keyboard, etc.)
	 * Automatically handles server RPC if called on client
	 * Advances the narrative state machine, which triggers automated AI facemask performances
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|AI Facemask|Narrative")
	void RequestAdvanceExperience();

	/**
	 * Request to retreat the narrative state (input-agnostic, works with any input source)
	 * Call this from any input source (EmbeddedSystems, VR controllers, keyboard, etc.)
	 * Automatically handles server RPC if called on client
	 * Retreats the narrative state machine, which triggers automated AI facemask performances
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|AI Facemask|Narrative")
	void RequestRetreatExperience();

	/**
	 * Server RPC: Advance experience (called automatically by RequestAdvanceExperience)
	 */
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerAdvanceExperience();

	/**
	 * Server RPC: Retreat experience (called automatically by RequestRetreatExperience)
	 */
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerRetreatExperience();

	virtual int32 GetMaxPlayers() const override { return NumberOfLiveActors + NumberOfPlayers; }

protected:
	virtual bool InitializeExperienceImpl() override;
	virtual void ShutdownExperienceImpl() override;
	virtual void Tick(float DeltaTime) override;

	/**
	 * Process input from VR controllers (for listen server hosts or Blueprint extension)
	 * Override in Blueprint to add VR controller input support
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "HoloCade|AI Facemask|Input")
	void ProcessVRControllerInput();
	virtual void ProcessVRControllerInput_Implementation() {}

private:
	/**
	 * Process input from wrist-mounted embedded system buttons
	 * Only runs on authority (server or listen server host)
	 */
	void ProcessEmbeddedSystemInput();

	/**
	 * Internal: Advance narrative state on server authority
	 * Only called on server after authority check
	 * Uses base class AdvanceNarrativeState() method
	 */
	bool AdvanceExperienceInternal();

	/**
	 * Internal: Retreat narrative state on server authority
	 * Only called on server after authority check
	 * Uses base class RetreatNarrativeState() method
	 */
	bool RetreatExperienceInternal();

	/**
	 * Handle narrative state changes (implements base class BlueprintImplementableEvent)
	 * Called when live actor advances/retreats narrative state via wireless trigger buttons
	 * Each state change triggers automated AI facemask performances
	 * Override in Blueprint to trigger game events based on state changes
	 */
	void OnNarrativeStateChanged(FName OldState, FName NewState, int32 NewStateIndex);

	/** Handle server discovery (auto-connect) */
	UFUNCTION()
	void OnServerDiscovered(const FHoloCadeServerInfo& ServerInfo);

	/** Previous button states for edge detection (embedded systems) */
	bool PreviousEmbeddedButtonStates[4] = {false, false, false, false};
};

