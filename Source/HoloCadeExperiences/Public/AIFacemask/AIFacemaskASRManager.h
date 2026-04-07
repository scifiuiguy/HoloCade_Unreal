// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "ASR/AIASRManager.h"
#include "AIFacemaskASRManager.generated.h"


// Forward declarations
class UAIFacemaskImprovManager;

/**
 * Facemask-specific configuration for ASR
 */
USTRUCT(BlueprintType)
struct HOLOCADEEXPERIENCES_API FAIFacemaskASRConfig
{
	GENERATED_BODY()

	/** Base ASR config (inherited from generic) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|AIFacemask ASR")
	FAIASRConfig BaseConfig;

	/** Whether to automatically trigger improv after transcription */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|AIFacemask ASR")
	bool bAutoTriggerImprov = true;

	FAIFacemaskASRConfig()
		: bAutoTriggerImprov(true)
	{}
};

/**
 * AIFacemask ASR Manager Component
 * 
 * Facemask-specific ASR manager that extends UAIASRManager.
 * Adds auto-triggering of improv responses after transcription.
 * 
 * Inherits from UAIASRManager for generic ASR functionality.
 * Adds:
 * - Auto-triggering improv responses after transcription
 * - Facemask-specific transcription handling
 * - Integration with AIFacemaskImprovManager
 */
UCLASS(ClassGroup=(HoloCade), meta=(BlueprintSpawnableComponent))
class HOLOCADEEXPERIENCES_API UAIFacemaskASRManager : public UAIASRManager
{
	GENERATED_BODY()

public:
	UAIFacemaskASRManager();

	/** Facemask-specific configuration for ASR */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|AIFacemask ASR")
	FAIFacemaskASRConfig FacemaskASRConfig;

	// Override generic base class methods
	virtual bool InitializeASRManager() override;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Override generic base class protected methods
	virtual void HandleTranscriptionResult(int32 SourceId, const FString& TranscribedText) override;

	/** Reference to AIFacemask Improv Manager (for auto-triggering improv) */
	UPROPERTY()
	TObjectPtr<UAIFacemaskImprovManager> ImprovManager;
};

