// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "RF433MHzTypes.generated.h"

/**
 * 433MHz USB Receiver Module Types
 */
UENUM(BlueprintType)
enum class ERF433MHzReceiverType : uint8
{
	RTL_SDR     UMETA(DisplayName = "RTL-SDR USB Dongle"),
	CC1101      UMETA(DisplayName = "CC1101 USB Module"),
	RFM69       UMETA(DisplayName = "RFM69 USB Module"),
	RFM95       UMETA(DisplayName = "RFM95 USB Module"),
	Generic     UMETA(DisplayName = "Generic 433MHz USB Receiver")
};

/**
 * RF433MHz Receiver Configuration
 */
USTRUCT(BlueprintType)
struct RF433MHZ_API FRF433MHzReceiverConfig
{
	GENERATED_BODY()

	/** USB receiver module type */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RF433MHz")
	ERF433MHzReceiverType ReceiverType = ERF433MHzReceiverType::Generic;

	/** USB device path (COM port on Windows, /dev/ttyUSB0 on Linux, varies by module) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RF433MHz")
	FString USBDevicePath = TEXT("COM3");

	/** Enable rolling code validation (prevents replay attacks) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RF433MHz|Security")
	bool bEnableRollingCodeValidation = true;

	/** Rolling code seed (must match remote firmware) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RF433MHz|Security", meta = (EditCondition = "bEnableRollingCodeValidation"))
	int32 RollingCodeSeed = 0x12345678;

	/** Enable replay attack prevention (reject duplicate codes) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RF433MHz|Security")
	bool bEnableReplayAttackPrevention = true;

	/** Replay attack window (ms) - reject codes within this window of last code */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RF433MHz|Security", meta = (EditCondition = "bEnableReplayAttackPrevention"))
	int32 ReplayAttackWindow = 100;

	/** Enable AES encryption (for custom solutions with encrypted remotes) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RF433MHz|Security")
	bool bEnableAESEncryption = false;

	/** AES encryption key (128-bit = 16 bytes, 256-bit = 32 bytes) - stored as hex string */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RF433MHz|Security", meta = (EditCondition = "bEnableAESEncryption"))
	FString AESEncryptionKey = TEXT("");  // Hex string: "0123456789ABCDEF0123456789ABCDEF" for AES-128

	/** AES key size (128 or 256 bits) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RF433MHz|Security", meta = (EditCondition = "bEnableAESEncryption"))
	int32 AESKeySize = 128;  // 128 or 256

	/** Update rate for button event polling (Hz) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RF433MHz|Performance")
	float UpdateRate = 20.0f;  // 20 Hz default
};

/**
 * Button Event from 433MHz Remote
 */
USTRUCT(BlueprintType)
struct RF433MHZ_API FRF433MHzButtonEvent
{
	GENERATED_BODY()

	/** Button code (0-255, mapped from remote) */
	UPROPERTY(BlueprintReadOnly, Category = "RF433MHz")
	int32 ButtonCode = 0;

	/** Button state (true = pressed, false = released) */
	UPROPERTY(BlueprintReadOnly, Category = "RF433MHz")
	bool bPressed = false;

	/** Rolling code (if rolling code validation enabled) */
	UPROPERTY(BlueprintReadOnly, Category = "RF433MHz")
	int32 RollingCode = 0;

	/** Timestamp when event occurred (seconds since receiver initialization) */
	UPROPERTY(BlueprintReadOnly, Category = "RF433MHz")
	float Timestamp = 0.0f;
};

/**
 * Learned Button Information
 * Tracks a button that has been learned during learning mode
 */
USTRUCT(BlueprintType)
struct RF433MHZ_API FRF433MHzLearnedButton
{
	GENERATED_BODY()

	/** Button code (0-255, unique identifier) */
	UPROPERTY(BlueprintReadOnly, Category = "RF433MHz")
	int32 ButtonCode = 0;

	/** Rolling code seed for this button (for validation) */
	UPROPERTY(BlueprintReadOnly, Category = "RF433MHz")
	int32 RollingCodeSeed = 0;

	/** Timestamp when button was learned */
	UPROPERTY(BlueprintReadOnly, Category = "RF433MHz")
	float LearnedTimestamp = 0.0f;

	/** Assigned function name (empty if not assigned) */
	UPROPERTY(BlueprintReadWrite, Category = "RF433MHz")
	FString AssignedFunctionName = TEXT("");

	/** Whether this button is currently mapped/active */
	UPROPERTY(BlueprintReadWrite, Category = "RF433MHz")
	bool bIsMapped = false;
};

/**
 * Button Function Mapping
 * Maps a button code to a specific function/action
 */
USTRUCT(BlueprintType)
struct RF433MHZ_API FRF433MHzButtonMapping
{
	GENERATED_BODY()

	/** Button code */
	UPROPERTY(BlueprintReadWrite, Category = "RF433MHz")
	int32 ButtonCode = 0;

	/** Function name (e.g., "HeightUp", "HeightDown", "Calibrate") */
	UPROPERTY(BlueprintReadWrite, Category = "RF433MHz")
	FString FunctionName = TEXT("");

	/** Whether this mapping is active */
	UPROPERTY(BlueprintReadWrite, Category = "RF433MHz")
	bool bIsActive = true;
};

