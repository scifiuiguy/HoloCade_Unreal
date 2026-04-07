// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HoloCadeWorldPositionCalibrator.generated.h"

// Forward declarations
class IHoloCadeTrackingInterface;

/**
 * Calibration mode enumeration
 */
UENUM(BlueprintType)
enum class ECalibrationMode : uint8
{
	/** Manual calibration via drag/drop (requires Ops Tech interaction) */
	Manual UMETA(DisplayName = "Manual (Drag/Drop)"),
	
	/** Automatic calibration to fixed tracker position (happens once at launch) */
	CalibrateToTracker UMETA(DisplayName = "Calibrate to Tracker")
};

/**
 * World Position Calibrator Component
 * 
 * Handles world position calibration to correct for drift throughout the day.
 * Used by all experiences that need position calibration.
 * 
 * Two Calibration Modes:
 * 
 * 1. Manual Calibration (Drag/Drop):
 *    - Trigger-hold any part of the virtual world
 *    - Automatically detects if player is dragging horizontally or vertically
 *    - Constrains to that axis as virtual world recalibrates its origin offset
 *    - Releases when trigger is released
 *    - Networked: Ops Tech toggles calibration mode ON from server
 *    - First HMD client that connects can act as calibrating agent
 *    - Server saves calibration to file when calibration ends
 * 
 * 2. Calibrate to Tracker (Automatic):
 *    - Uses a fixed Ultimate tracker in a known physical location
 *    - Each client finds that tracker at launch
 *    - Calculates offset based on expected vs actual tracker position
 *    - Applies offset once at launch (not continuous - tracker may move during gameplay)
 *    - Ops Tech can add a fixed tracker to any lighthouse-ready experience
 * 
 * This allows Ops Tech to quickly recalibrate if tracking drift occurs.
 */
UCLASS(ClassGroup=(HoloCade), meta=(BlueprintSpawnableComponent))
class HOLOCADECORE_API UHoloCadeWorldPositionCalibrator : public UActorComponent
{
	GENERATED_BODY()

public:
	UHoloCadeWorldPositionCalibrator();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Whether calibration is currently active (local to calibrating client) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HoloCade|Calibration")
	bool bIsCalibrating = false;

	/** 
	 * Calibration mode (Manual drag/drop or Automatic tracker-based)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Calibration")
	ECalibrationMode CalibrationMode = ECalibrationMode::Manual;

	/** 
	 * Whether calibration mode is enabled (server-side, replicated to clients)
	 * Only used for Manual calibration mode.
	 * Toggle this ON from server (Command Console or Blueprint) to enable calibration.
	 * When enabled, any connected HMD client can act as the calibrating agent.
	 */
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Calibration|Manual", meta = (EditCondition = "CalibrationMode == ECalibrationMode::Manual"))
	bool bCalibrationModeEnabled = false;

	/** 
	 * Tracker device index to use for CalibrateToTracker mode
	 * Set to the index of the fixed Ultimate tracker in your tracking system
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Calibration|Tracker", meta = (EditCondition = "CalibrationMode == ECalibrationMode::CalibrateToTracker", ClampMin = "0"))
	int32 CalibrationTrackerIndex = 0;

	/** 
	 * Expected world space position of the fixed tracker (for CalibrateToTracker mode)
	 * This is the known physical location where the tracker should be
	 * Offset = ExpectedPosition - ActualTrackerPosition
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Calibration|Tracker", meta = (EditCondition = "CalibrationMode == ECalibrationMode::CalibrateToTracker"))
	FVector ExpectedTrackerPosition = FVector::ZeroVector;

	/** 
	 * Whether tracker-based calibration has been performed (prevents recalibration during gameplay)
	 * Set to true after successful calibration at launch
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HoloCade|Calibration|Tracker")
	bool bTrackerCalibrationComplete = false;

	/** Current world origin offset (applied to all virtual objects) - Replicated from server */
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "HoloCade|Calibration")
	FVector WorldOriginOffset = FVector::ZeroVector;

	/** 
	 * Calibration save path (on server's hard drive)
	 * If empty, uses default path: Saved/Config/HoloCade/Calibration_[ExperienceName].json
	 * Can be set via Blueprint to customize save location
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Calibration|Persistence", meta = (DisplayName = "Custom Save Path"))
	FString CalibrationSavePath;

	/**
	 * Start calibration mode
	 * @param InitialGrabLocation - World location where player grabbed
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Calibration")
	void StartCalibration(const FVector& InitialGrabLocation);

	/**
	 * Update calibration (called while trigger is held)
	 * @param CurrentGrabLocation - Current world location of grab point
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Calibration")
	void UpdateCalibration(const FVector& CurrentGrabLocation);

	/**
	 * End calibration mode
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Calibration")
	void EndCalibration();

	/**
	 * Reset world origin offset to zero
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Calibration")
	void ResetCalibration();

	/**
	 * Enable calibration mode (server-side only)
	 * Call this from Command Console or Blueprint to allow clients to calibrate.
	 * Once enabled, the first HMD client that connects can act as calibrating agent.
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Calibration")
	void EnableCalibrationMode();

	/**
	 * Disable calibration mode (server-side only)
	 * Call this to prevent further calibration attempts.
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Calibration")
	void DisableCalibrationMode();

	/**
	 * Perform tracker-based calibration (called automatically at launch if CalibrateToTracker mode is enabled)
	 * Finds the tracker and calculates offset based on expected vs actual position
	 * Only happens once at launch - not continuous (tracker may move during gameplay)
	 * @return True if calibration was successful
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Calibration|Tracker")
	bool PerformTrackerCalibration();

	/**
	 * Get calibrated world position (applies offset)
	 * @param RawPosition - Raw world position
	 * @return Calibrated position with offset applied
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|Calibration")
	FVector GetCalibratedPosition(const FVector& RawPosition) const;

	/**
	 * Save calibration offset to persistent storage (JSON file)
	 * Called automatically when calibration ends on server, but can be called manually
	 * @param ExperienceName - Optional experience name for per-experience calibration (default: "Default")
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Calibration|Persistence")
	void SaveCalibrationOffset(const FString& ExperienceName = TEXT("Default"));

	/**
	 * Load calibration offset from persistent storage (JSON file)
	 * Called automatically on BeginPlay on server, but can be called manually
	 * @param ExperienceName - Optional experience name for per-experience calibration (default: "Default")
	 * @return True if calibration was loaded successfully
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Calibration|Persistence")
	bool LoadCalibrationOffset(const FString& ExperienceName = TEXT("Default"));

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// =====================================
	// Network RPCs (Client -> Server)
	// =====================================

	/**
	 * Server RPC: Start calibration (called from client)
	 * @param InInitialGrabLocation - World location where player grabbed
	 */
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerStartCalibration(const FVector& InInitialGrabLocation);

	/**
	 * Server RPC: Update calibration (called from client while trigger is held)
	 * @param CurrentGrabLocation - Current world location of grab point
	 */
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerUpdateCalibration(const FVector& CurrentGrabLocation);

	/**
	 * Server RPC: End calibration (called from client when trigger is released)
	 */
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerEndCalibration();

	// =====================================
	// Network RPCs (Server -> Client)
	// =====================================

	/**
	 * Client RPC: Notify all clients of updated calibration offset
	 * @param NewOffset - New world origin offset
	 */
	UFUNCTION(Client, Reliable)
	void ClientUpdateCalibrationOffset(const FVector& NewOffset);

private:
	/** Initial grab location when calibration started */
	FVector InitialGrabLocation = FVector::ZeroVector;

	/** Last grab location (for axis detection) */
	FVector LastGrabLocation = FVector::ZeroVector;

	/** Detected drag axis (normalized) */
	FVector DragAxis = FVector::ZeroVector;

	/** Whether axis has been detected */
	bool bAxisDetected = false;

	/** Threshold for axis detection (cm) */
	float AxisDetectionThreshold = 5.0f;

	/** Current experience name for per-experience calibration */
	FString CurrentExperienceName = TEXT("Default");

	/** Detect drag axis from movement */
	void DetectDragAxis(const FVector& CurrentLocation);

	/**
	 * Get path to calibration JSON file
	 * Uses CalibrationSavePath if set, otherwise uses default path
	 * @param ExperienceName - Experience name (used if CalibrationSavePath is empty)
	 * @return Full path to calibration file
	 */
	FString GetCalibrationFilePath(const FString& ExperienceName) const;

	/**
	 * Check if this is running on server
	 */
	bool IsServer() const;

	/**
	 * Check if this is running on client
	 */
	bool IsClient() const;

	/**
	 * Get tracking interface from owner actor
	 * @return Tracking interface, or nullptr if not available
	 */
	IHoloCadeTrackingInterface* GetTrackingInterface() const;
};

