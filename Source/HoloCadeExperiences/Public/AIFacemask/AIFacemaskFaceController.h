// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/Texture2D.h"
#include "IWebSocket.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "AIFacemaskFaceController.generated.h"

/**
 * Facial animation data structure - receives output from NVIDIA ACE
 * 
 * This structure receives facial textures and blend shapes from NVIDIA ACE pipeline.
 * The AI facial animation is fully automated - no manual control or keyframe animation.
 * NVIDIA ACE determines facial expressions based on audio track and state machine context.
 */
USTRUCT(BlueprintType)
struct FFacialAnimationData
{
	GENERATED_BODY()

	/** Blend shape weights from NVIDIA ACE (normalized 0-1) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|AI Face")
	TMap<FName, float> BlendShapeWeights;

	/** Facial texture data from NVIDIA ACE (if applicable) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|AI Face")
	TObjectPtr<UTexture2D> FacialTexture = nullptr;

	/** Timestamp of this animation frame */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|AI Face")
	float Timestamp = 0.0f;
};

/**
 * Configuration for AI Face system
 */
USTRUCT(BlueprintType)
struct FAIFaceConfig
{
	GENERATED_BODY()

	/** Target skeletal mesh component attached to live actor's HMD/head */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|AI Face")
	TObjectPtr<USkeletalMeshComponent> TargetMesh = nullptr;

	/** NVIDIA ACE endpoint URL for receiving facial animation data */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|AI Face")
	FString NVIDIAACEEndpointURL;

	/** Update rate for receiving facial animation data from NVIDIA ACE (Hz) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|AI Face", meta = (ClampMin = "1", ClampMax = "120"))
	float UpdateRate = 30.0f;
};

/**
 * AI Face Controller Component
 * 
 * Receives and applies NVIDIA ACE facial animation output to a live actor's HMD-mounted mesh.
 * 
 * ARCHITECTURE:
 * - Live actor wears HMD with AIFace mesh tracked on top of their face (like a mask)
 * - NVIDIA ACE pipeline (Audio → NLU → Emotion → Facial Animation) generates facial textures
 *   and blend shapes automatically based on audio track and state machine context
 * - This component receives NVIDIA ACE output and applies it to the mesh in real-time
 * - NO manual control, keyframe animation, rigging, or blend shape tools required
 * 
 * USAGE:
 * - Attach to live actor's HMD/head actor
 * - Configure TargetMesh to point to the AIFace skeletal mesh component
 * - NVIDIA ACE streams facial animation data to this component
 * - Component applies received data to mesh automatically
 * 
 * IMPORTANT:
 * - This is a RECEIVER/DISPLAY system, not a control system
 * - Facial expressions are determined by NVIDIA ACE, not manually configured
 * - Live actor controls experience flow via wrist buttons, not facial animation
 */
UCLASS(ClassGroup=(HoloCade), meta=(BlueprintSpawnableComponent))
class HOLOCADEEXPERIENCES_API UAIFacemaskFaceController : public UActorComponent
{
	GENERATED_BODY()

public:	
	UAIFacemaskFaceController();

	/** Configuration for this AI face controller */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|AI Face")
	FAIFaceConfig Config;

	/**
	 * Initialize the AI face system with given configuration
	 * @param InConfig - Configuration settings
	 * @return true if initialization was successful
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|AI Face")
	bool InitializeAIFace(const FAIFaceConfig& InConfig);

	/**
	 * Connect to NVIDIA ACE endpoint to receive streaming facial animation data
	 * @return true if connection was successful
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|AI Face")
	bool ConnectToACEEndpoint();

	/**
	 * Disconnect from NVIDIA ACE endpoint
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|AI Face")
	void DisconnectFromACEEndpoint();

	/**
	 * Check if connected to NVIDIA ACE endpoint
	 * @return true if connected
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|AI Face")
	bool IsConnected() const { return bIsConnected; }

	/**
	 * Receive and apply facial animation data from NVIDIA ACE
	 * Called automatically when NVIDIA ACE sends new facial animation data
	 * @param AnimationData - Facial animation data from NVIDIA ACE (blend shapes + textures)
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|AI Face")
	void ReceiveFacialAnimationData(const FFacialAnimationData& AnimationData);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	/** Apply received blend shapes to target mesh */
	void ApplyBlendShapesToMesh(const TMap<FName, float>& BlendShapeWeights);

	/** Apply received facial texture to target mesh */
	void ApplyFacialTextureToMesh(UTexture2D* FacialTexture);

	/** Parse JSON facial animation data from NVIDIA ACE */
	bool ParseFacialAnimationData(const FString& JsonString, FFacialAnimationData& OutAnimationData);

	/** Create texture from base64 image data */
	UTexture2D* CreateTextureFromBase64(const FString& Base64Data);

	/** Handle WebSocket connection opened */
	void OnWebSocketConnected();

	/** Handle WebSocket connection closed */
	void OnWebSocketConnectionClosed(int32 StatusCode, const FString& Reason, bool bWasClean);

	/** Handle WebSocket message received */
	void OnWebSocketMessageReceived(const FString& Message);

	/** Handle WebSocket error */
	void OnWebSocketError(const FString& Error);

	/** Current facial animation data from NVIDIA ACE */
	FFacialAnimationData CurrentAnimationData;

	/** Whether the system is initialized */
	bool bIsInitialized = false;

	/** Whether connected to NVIDIA ACE endpoint */
	bool bIsConnected = false;

	/** WebSocket connection to NVIDIA ACE endpoint */
	TSharedPtr<IWebSocket> WebSocket;

	/** Dynamic material instance for facial texture updates */
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> DynamicMaterial = nullptr;

	/** Blend shape name mapping (NVIDIA ACE names → Unreal morph target names) */
	UPROPERTY(EditAnywhere, Category = "HoloCade|AI Face")
	TMap<FName, FName> BlendShapeNameMapping;

	/** Material parameter name for facial texture */
	UPROPERTY(EditAnywhere, Category = "HoloCade|AI Face", meta = (DisplayName = "Facial Texture Parameter Name"))
	FName FacialTextureParameterName = TEXT("FacialTexture");
};

