// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Networking/HoloCadeUDPTransport.h"
#include "HapticPlatformController.generated.h"

/**
 * Platform type enumeration
 */
UENUM(BlueprintType)
enum class EHoloCadePlatformType : uint8
{
	MovingPlatform_SinglePlayer UMETA(DisplayName = "5DOF Moving Platform (Single Player)"),
	Gunship_FourPlayer UMETA(DisplayName = "4DOF Gunship (Four Player)"),
	CarSim_SinglePlayer UMETA(DisplayName = "5DOF Car Sim (Single Player)"),
	FlightSim_2DOF UMETA(DisplayName = "2DOF Full 360 Flight Sim"),
	Custom UMETA(DisplayName = "Custom Configuration")
};

/**
 * Actuator configuration for a single hydraulic cylinder
 */
USTRUCT(BlueprintType)
struct FHydraulicActuator
{
	GENERATED_BODY()

	/** Unique identifier for this actuator */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Haptics")
	FName ActuatorID;

	/** Current extension (0.0 = fully retracted, 1.0 = fully extended) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Haptics")
	float Extension = 0.5f;

	/** Position of this actuator relative to platform center */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Haptics")
	FVector RelativePosition = FVector::ZeroVector;

	/** Maximum extension range in cm */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Haptics")
	float MaxExtensionCm = 30.0f;
};

/**
 * Supported HOTAS controller types
 */
UENUM(BlueprintType)
enum class EHoloCadeHOTASType : uint8
{
	None UMETA(DisplayName = "None (Standard VR Controllers)"),
	LogitechX56 UMETA(DisplayName = "Logitech G X56"),
	ThrustmasterTFlight UMETA(DisplayName = "Thrustmaster T.Flight"),
	Custom UMETA(DisplayName = "Custom HOTAS")
};

/**
 * Gyroscope configuration for 2DOF flight simulators
 */
USTRUCT(BlueprintType)
struct FGyroscopeConfig
{
	GENERATED_BODY()

	/** Enable continuous rotation beyond 360 degrees on pitch axis */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Haptics")
	bool bEnableContinuousPitch = true;

	/** Enable continuous rotation beyond 360 degrees on roll axis */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Haptics")
	bool bEnableContinuousRoll = true;

	/** Maximum rotation speed in degrees per second */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Haptics")
	float MaxRotationSpeed = 60.0f;

	/** Pitch axis invert */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Haptics")
	bool bInvertPitchAxis = false;

	/** Roll axis invert */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Haptics")
	bool bInvertRollAxis = false;

	/** HOTAS controller type to use */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Haptics|HOTAS")
	EHoloCadeHOTASType HOTASType = EHoloCadeHOTASType::None;

	/** Enable HOTAS joystick input */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Haptics|HOTAS")
	bool bEnableJoystick = true;

	/** Enable HOTAS throttle input */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Haptics|HOTAS")
	bool bEnableThrottle = true;

	/** Enable pedal controls */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Haptics|HOTAS")
	bool bEnablePedals = false;

	/** Joystick sensitivity multiplier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Haptics|HOTAS", meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float JoystickSensitivity = 1.0f;

	/** Throttle sensitivity multiplier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Haptics|HOTAS", meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float ThrottleSensitivity = 1.0f;

	/** 
	 * Axis mapping configuration (for GenericUSBController axes)
	 * These can be adjusted if your HOTAS device uses different axis numbers
	 * Default: Axis0=Roll, Axis1=Pitch, Axis2=Throttle, Axis3=Pedals
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Haptics|HOTAS|Advanced", meta = (ClampMin = "0", ClampMax = "15"))
	int32 RollAxisIndex = 0;  // GenericUSBController_Axis0

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Haptics|HOTAS|Advanced", meta = (ClampMin = "0", ClampMax = "15"))
	int32 PitchAxisIndex = 1;  // GenericUSBController_Axis1

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Haptics|HOTAS|Advanced", meta = (ClampMin = "0", ClampMax = "15"))
	int32 ThrottleAxisIndex = 2;  // GenericUSBController_Axis2

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Haptics|HOTAS|Advanced", meta = (ClampMin = "0", ClampMax = "15"))
	int32 PedalsAxisIndex = 3;  // GenericUSBController_Axis3
};

/**
 * Platform configuration
 */
USTRUCT(BlueprintType)
struct FHapticPlatformConfig
{
	GENERATED_BODY()

	/** Type of platform */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Haptics")
	EHoloCadePlatformType PlatformType = EHoloCadePlatformType::MovingPlatform_SinglePlayer;

	/** Array of hydraulic actuators */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Haptics")
	TArray<FHydraulicActuator> Actuators;

	/** Maximum pitch angle in degrees */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Haptics")
	float MaxPitchDegrees = 10.0f;

	/** Maximum roll angle in degrees */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Haptics")
	float MaxRollDegrees = 10.0f;

	/** Maximum Y translation in cm (scissor lift) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Haptics")
	float MaxTranslationY = 100.0f;

	/** Maximum Z translation in cm (scissor lift) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Haptics")
	float MaxTranslationZ = 100.0f;

	/** Gyroscope configuration (for 2DOF Flight Sim) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Haptics")
	FGyroscopeConfig GyroscopeConfig;

	/** Network address of the platform controller hardware */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Haptics")
	FString ControllerIPAddress = TEXT("192.168.1.100");

	/** Network port for platform controller */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Haptics")
	int32 ControllerPort = 8888;
};

/**
 * Motion command for platform
 */
USTRUCT(BlueprintType)
struct FPlatformMotionCommand
{
	GENERATED_BODY()

	/** Target pitch angle in degrees (for 2DOF gyroscope: unlimited; for 4DOF platforms: clamped to MaxPitchDegrees) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Haptics")
	float Pitch = 0.0f;

	/** Target roll angle in degrees (for 2DOF gyroscope: unlimited; for 4DOF platforms: clamped to MaxRollDegrees) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Haptics")
	float Roll = 0.0f;

	/** Target Y translation in cm (4DOF platforms only - scissor lift forward/reverse, positive = forward) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Haptics")
	float TranslationY = 0.0f;

	/** Target Z translation in cm (4DOF platforms only - scissor lift up/down, positive = up) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Haptics")
	float TranslationZ = 0.0f;

	/** Duration to reach target position (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Haptics")
	float Duration = 1.0f;

	/** Use continuous rotation (2DOF gyroscope only - allows rotation beyond 360 degrees) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Haptics")
	bool bUseContinuousRotation = false;
};

/**
 * Haptic Platform Controller Component
 * 
 * Controls large-scale motion platforms including:
 * - 4DOF Moving Platform (single player standing)
 * - 4DOF Gunship (four player seated)
 * - 4DOF Car Sim (single player seated racing/driving simulator)
 * - 2DOF Full 360 Flight Sim (single player gyroscope with continuous rotation)
 * 
 * Provides both high-level motion commands and low-level actuator control.
 */
UCLASS(ClassGroup=(HoloCade), meta=(BlueprintSpawnableComponent))
class LARGEHAPTICS_API UHapticPlatformController : public UHoloCadeUDPTransport
{
	GENERATED_BODY()

public:	
	UHapticPlatformController();

	/** Platform configuration */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Haptics")
	FHapticPlatformConfig Config;

	/**
	 * Initialize the haptic platform system
	 * @param InConfig - Configuration settings
	 * @return true if initialization was successful
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Haptics")
	bool InitializePlatform(const FHapticPlatformConfig& InConfig);

	/**
	 * Send a motion command to the platform (advanced - uses absolute angles)
	 * @param Command - The motion command to execute
	 * @param bUseStructPacket - If true, sends as single struct packet; if false, sends as 5 separate channel packets (default: false for compatibility)
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Haptics|Advanced")
	void SendMotionCommand(const FPlatformMotionCommand& Command, bool bUseStructPacket = false);

	/**
	 * Send normalized platform motion (recommended for game code)
	 * Uses joystick-style input that automatically scales to hardware capabilities
	 * @param TiltX - Left/Right roll (-1.0 = full left, +1.0 = full right, 0.0 = level)
	 * @param TiltY - Forward/Backward pitch (-1.0 = full backward, +1.0 = full forward, 0.0 = level)
	 * @param ForwardOffset - Scissor lift forward/reverse (-1.0 = full reverse, +1.0 = full forward, 0.0 = neutral)
	 * @param VerticalOffset - Scissor lift up/down (-1.0 = full down, +1.0 = full up, 0.0 = neutral)
	 * @param Duration - Time to reach target position (seconds)
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Haptics")
	void SendNormalizedMotion(float TiltX, float TiltY, float ForwardOffset = 0.0f, float VerticalOffset = 0.0f, float Duration = 1.0f);

	/**
	 * Set a specific actuator extension
	 * @param ActuatorID - ID of the actuator to control
	 * @param Extension - Extension value (0.0 - 1.0)
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Haptics")
	void SetActuatorExtension(FName ActuatorID, float Extension);

	/**
	 * Emergency stop - immediately halt all platform motion
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Haptics")
	void EmergencyStop();

	/**
	 * Return platform to neutral position
	 * @param Duration - Time to return to neutral (seconds)
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Haptics")
	void ReturnToNeutral(float Duration = 2.0f);

	/**
	 * Get current platform transform relative to neutral
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Haptics")
	FTransform GetCurrentPlatformTransform() const;

	// Note: HOTAS methods are implemented in subclasses (e.g., U2DOFGyroPlatformController)
	// Base class does not handle HOTAS - it's platform-specific

	// =====================================
	// Channel-Based IO API (Generic - Experience-Agnostic)
	// =====================================
	// Note: SendFloat, SendBool, SendInt32, SendBytes, SendStruct, GetReceivedFloat, GetReceivedBool, GetReceivedInt32
	// are inherited from UHoloCadeUDPTransport base class. OnFloatReceived, OnBoolReceived, OnInt32Received delegates
	// are also inherited from base class.

	/**
	 * Check if platform controller is connected to hardware
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Haptics|IO")
	bool IsHardwareConnected() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Whether the system is initialized and connected */
	bool bIsInitialized = false;

private:

	/** Current platform state */
	FPlatformMotionCommand CurrentState;

	/** Target platform state */
	FPlatformMotionCommand TargetState;

	/** Time remaining for current motion */
	float MotionTimeRemaining = 0.0f;

	/** Total duration of current motion */
	float MotionTotalDuration = 0.0f;

	// Note: HOTAS member variables are in subclasses (e.g., U2DOFGyroPlatformController)
	// Base class does not handle HOTAS - it's platform-specific

	// UDP socket and protocol management now handled by base class (UHoloCadeUDPTransport)

	/**
	 * Send command to hardware controller (uses channel-based API internally)
	 * @param Command - The motion command to send
	 * @param bUseStructPacket - If true, sends as single struct packet; if false, sends as 5 separate channel packets
	 */
	void SendCommandToHardware(const FPlatformMotionCommand& Command, bool bUseStructPacket = false);

	// UDP socket and protocol management now handled by base class (UHoloCadeUDPTransport)

	/**
	 * Interpolate between current and target state
	 */
	void UpdateMotionInterpolation(float DeltaTime);

	// Note: HOTAS methods are implemented in subclasses (e.g., U2DOFGyroPlatformController)
	// Base class does not handle HOTAS - it's platform-specific
};


