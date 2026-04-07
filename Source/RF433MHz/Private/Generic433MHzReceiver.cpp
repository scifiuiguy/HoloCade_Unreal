// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "Generic433MHzReceiver.h"
#include "RF433MHz.h"
#include "HAL/PlatformFilemanager.h"
#include "HAL/PlatformTime.h"
#include "Misc/DateTime.h"

FGeneric433MHzReceiver::FGeneric433MHzReceiver()
	: bIsConnected(false)
	, bLearningModeActive(false)
	, LearningModeTimeout(0.0f)
	, LearningModeTimer(0.0f)
	, ExpectedRollingCode(0)
	, LastReceivedRollingCode(0)
	, LastEventTimestamp(0.0f)
{
}

FGeneric433MHzReceiver::~FGeneric433MHzReceiver()
{
	Shutdown();
}

bool FGeneric433MHzReceiver::Initialize(const FRF433MHzReceiverConfig& Config)
{
	CurrentConfig = Config;
	bIsConnected = false;

	// NOOP: Initialize USB serial connection
	// TODO: Implement actual USB serial communication
	// This will use platform-specific serial APIs (Windows: CreateFile/ReadFile, Linux: open/read)
	
	if (InitializeSerialConnection())
	{
		bIsConnected = true;
		ExpectedRollingCode = Config.RollingCodeSeed;
		UE_LOG(LogRF433MHz, Log, TEXT("Generic433MHzReceiver: Initialized (Device: %s)"), *Config.USBDevicePath);
		return true;
	}

	UE_LOG(LogRF433MHz, Error, TEXT("Generic433MHzReceiver: Failed to initialize (Device: %s)"), *Config.USBDevicePath);
	return false;
}

void FGeneric433MHzReceiver::Shutdown()
{
	if (bIsConnected)
	{
		CloseSerialConnection();
		bIsConnected = false;
		ButtonEventBuffer.Empty();
		ReceivedRollingCodes.Empty();
		LastCodeTimestamps.Empty();
		UE_LOG(LogRF433MHz, Log, TEXT("Generic433MHzReceiver: Shutdown"));
	}
}

bool FGeneric433MHzReceiver::IsConnected() const
{
	return bIsConnected;
}

bool FGeneric433MHzReceiver::GetButtonEvents(TArray<FRF433MHzButtonEvent>& OutEvents)
{
	if (!bIsConnected)
	{
		return false;
	}

	OutEvents.Empty();

	// NOOP: Read from USB serial port
	// TODO: Implement actual USB serial data reading
	// This will read raw bytes from the USB device and parse them into button events
	
	TArray<uint8> RawData;
	if (ReadSerialData(RawData))
	{
		// Parse button events from raw data
		FRF433MHzButtonEvent Event;
		if (ParseButtonEvent(RawData, Event))
		{
			// Validate rolling code if enabled
			if (CurrentConfig.bEnableRollingCodeValidation)
			{
				if (!ValidateRollingCode(Event.RollingCode))
				{
					UE_LOG(LogRF433MHz, Warning, TEXT("Generic433MHzReceiver: Invalid rolling code %u (expected: %u)"), 
						Event.RollingCode, ExpectedRollingCode);
					return false;
				}
			}

			// Check for replay attack if enabled
			if (CurrentConfig.bEnableReplayAttackPrevention)
			{
				if (IsReplayAttack(Event.RollingCode, Event.Timestamp))
				{
					UE_LOG(LogRF433MHz, Warning, TEXT("Generic433MHzReceiver: Replay attack detected (code: %u)"), 
						Event.RollingCode);
					return false;
				}
			}

			// Update learning mode if active
			if (bLearningModeActive)
			{
				// Store learned code (delegate will be triggered by component)
				UE_LOG(LogRF433MHz, Log, TEXT("Generic433MHzReceiver: Learned code (Button: %d, RollingCode: %u)"), 
					Event.ButtonCode, Event.RollingCode);
			}

			OutEvents.Add(Event);
			LastReceivedRollingCode = Event.RollingCode;
			LastEventTimestamp = Event.Timestamp;
			return true;
		}
	}

	return false;
}

bool FGeneric433MHzReceiver::IsRollingCodeValid() const
{
	if (!CurrentConfig.bEnableRollingCodeValidation)
	{
		return true; // Validation disabled
	}

	// Check if rolling code is within acceptable drift
	int32 Drift = GetRollingCodeDrift();
	return FMath::Abs(Drift) <= 10; // Allow ±10 code drift
}

int32 FGeneric433MHzReceiver::GetRollingCodeDrift() const
{
	if (!CurrentConfig.bEnableRollingCodeValidation)
	{
		return 0;
	}

	return static_cast<int32>(LastReceivedRollingCode) - static_cast<int32>(ExpectedRollingCode);
}

void FGeneric433MHzReceiver::EnableLearningMode(float TimeoutSeconds)
{
	bLearningModeActive = true;
	LearningModeTimeout = TimeoutSeconds;
	LearningModeTimer = 0.0f;
	UE_LOG(LogRF433MHz, Log, TEXT("Generic433MHzReceiver: Learning mode enabled (Timeout: %.1f seconds)"), TimeoutSeconds);
}

void FGeneric433MHzReceiver::DisableLearningMode()
{
	bLearningModeActive = false;
	LearningModeTimeout = 0.0f;
	LearningModeTimer = 0.0f;
	UE_LOG(LogRF433MHz, Log, TEXT("Generic433MHzReceiver: Learning mode disabled"));
}

bool FGeneric433MHzReceiver::IsLearningModeActive() const
{
	return bLearningModeActive;
}

bool FGeneric433MHzReceiver::InitializeSerialConnection()
{
	// NOOP: Initialize USB serial connection
	// TODO: Implement platform-specific serial port initialization
	// Windows: CreateFile with COM port path
	// Linux: open() with /dev/ttyUSB* path
	
	UE_LOG(LogRF433MHz, Warning, TEXT("Generic433MHzReceiver: InitializeSerialConnection() is NOOP - USB serial communication not yet implemented"));
	return false; // Return false until implemented
}

void FGeneric433MHzReceiver::CloseSerialConnection()
{
	// NOOP: Close USB serial connection
	// TODO: Implement platform-specific serial port cleanup
	// Windows: CloseHandle
	// Linux: close()
}

bool FGeneric433MHzReceiver::ReadSerialData(TArray<uint8>& OutData)
{
	// NOOP: Read from USB serial port
	// TODO: Implement platform-specific serial data reading
	// Windows: ReadFile
	// Linux: read()
	
	OutData.Empty();
	return false; // Return false until implemented
}

bool FGeneric433MHzReceiver::ParseButtonEvent(const TArray<uint8>& Data, FRF433MHzButtonEvent& OutEvent)
{
	// NOOP: Parse button event from raw data
	// TODO: Implement protocol-specific parsing
	// Most generic receivers send simple byte sequences:
	// Format: [ButtonCode (1 byte)] [State (1 byte: 0=released, 1=pressed)] [RollingCode (4 bytes, optional)]
	
	if (Data.Num() < 2)
	{
		return false;
	}

	OutEvent.ButtonCode = Data[0];
	OutEvent.bPressed = (Data[1] != 0);
	OutEvent.Timestamp = FPlatformTime::Seconds();

	// Parse rolling code if present (4 bytes, little-endian)
	if (Data.Num() >= 6 && CurrentConfig.bEnableRollingCodeValidation)
	{
		OutEvent.RollingCode = (Data[2] << 0) | (Data[3] << 8) | (Data[4] << 16) | (Data[5] << 24);
	}
	else
	{
		OutEvent.RollingCode = 0;
	}

	return true;
}

bool FGeneric433MHzReceiver::ValidateRollingCode(uint32 Code)
{
	if (!CurrentConfig.bEnableRollingCodeValidation)
	{
		return true;
	}

	// Simple validation: code should be >= expected (allows for some drift)
	// More sophisticated validation would track code sequence and detect gaps
	int32 Drift = static_cast<int32>(Code) - static_cast<int32>(ExpectedRollingCode);
	
	if (Drift >= 0 && Drift <= 100) // Allow up to 100 codes ahead
	{
		ExpectedRollingCode = Code + 1; // Update expected for next event
		return true;
	}

	return false;
}

bool FGeneric433MHzReceiver::IsReplayAttack(uint32 RollingCode, float Timestamp)
{
	if (!CurrentConfig.bEnableReplayAttackPrevention)
	{
		return false;
	}

	// Check if we've seen this code recently
	float* LastTimestamp = LastCodeTimestamps.Find(RollingCode);
	if (LastTimestamp)
	{
		float TimeSinceLastCode = Timestamp - *LastTimestamp;
		if (TimeSinceLastCode < (CurrentConfig.ReplayAttackWindow / 1000.0f))
		{
			return true; // Replay attack detected
		}
	}

	// Update timestamp for this code
	LastCodeTimestamps.Add(RollingCode, Timestamp);

	// Clean up old timestamps (older than replay attack window)
	TArray<uint32> CodesToRemove;
	for (const auto& Pair : LastCodeTimestamps)
	{
		if (Timestamp - Pair.Value > (CurrentConfig.ReplayAttackWindow / 1000.0f))
		{
			CodesToRemove.Add(Pair.Key);
		}
	}
	for (uint32 Code : CodesToRemove)
	{
		LastCodeTimestamps.Remove(Code);
	}

	return false;
}

