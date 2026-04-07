// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

/**
 * RF Trigger Controller Example
 * 
 * This example demonstrates how to use the RF433MHz low-level API to integrate
 * 433MHz wireless remote/receiver functionality into your HoloCade experience.
 * 
 * The RF433MHz API provides an abstraction layer for different USB receiver modules
 * (RTL-SDR, CC1101, RFM69, Generic) so your game server code doesn't need to know
 * which specific hardware is being used.
 * 
 * Usage:
 * 1. Add URF433MHzReceiver component to your Actor
 * 2. Configure receiver type and USB device
 * 3. Subscribe to button event delegates
 * 4. Handle button events in your experience logic
 * 
 * Example Use Cases:
 * - Height calibration clicker (SuperheroFlightExperience)
 * - Wireless trigger buttons (costume-embedded, prop-mounted)
 * - Remote control for Ops Tech operations
 * - Emergency stop remotes
 * 
 * Security:
 * - Rolling code validation (if remote supports it)
 * - Replay attack prevention
 * - Safety interlock enforcement (calibration mode only, movement limits, timeout)
 * 
 * See FirmwareExamples/Base/Examples/RFTriggerECU_Example.ino for firmware example.
 */

#include "RFTriggerController_Example.h"
#include "RF433MHz/Public/RF433MHzReceiver.h"
#include "RF433MHz/Public/RF433MHzTypes.h"
#include "HAL/PlatformTime.h"

// =====================================
// Constructor and Lifecycle
// =====================================

ARFTriggerControllerExample::ARFTriggerControllerExample()
{
	PrimaryActorTick.bCanEverTick = false;
	RFReceiver = nullptr;
	HeightCalibrationReceiver = nullptr;
	TriggerReceiver = nullptr;
	EmergencyStopReceiver = nullptr;
	bIsCalibrationMode = false;
	bPlaySessionActive = false;
	LastCalibrationActivity = 0.0f;
}

void ARFTriggerControllerExample::BeginPlay()
{
	Super::BeginPlay();
	// NOOP: Initialization should be called explicitly via Blueprint or C++
}

// =====================================
// Example: SuperheroFlightExperience Height Calibration
// =====================================

void ARFTriggerControllerExample::InitializeHeightCalibration()
{
	// Create RF433MHz receiver component
	RFReceiver = NewObject<URF433MHzReceiver>(this);
	if (!RFReceiver)
	{
		UE_LOG(LogTemp, Error, TEXT("RFTriggerControllerExample: Failed to create RF433MHzReceiver"));
		return;
	}
	
	// Configure receiver
	FRF433MHzReceiverConfig Config;
	Config.ReceiverType = ERF433MHzReceiverType::CC1101;  // Or RTL-SDR, RFM69, Generic
	Config.USBDevicePath = TEXT("COM3");  // Or /dev/ttyUSB0 on Linux
	Config.bEnableRollingCodeValidation = true;
	Config.RollingCodeSeed = 0x12345678;  // Must match remote firmware
	Config.bEnableReplayAttackPrevention = true;
	Config.ReplayAttackWindow = 100;  // Reject codes within 100ms of last code
	
	// Initialize receiver
	if (!RFReceiver->InitializeReceiver(Config))
	{
		UE_LOG(LogTemp, Error, TEXT("RFTriggerControllerExample: Failed to initialize RF receiver"));
		return;
	}
	
	// Subscribe to button events
	RFReceiver->OnButtonPressed.AddDynamic(this, &ARFTriggerControllerExample::HandleButtonPressed);
	RFReceiver->OnButtonReleased.AddDynamic(this, &ARFTriggerControllerExample::HandleButtonReleased);
	RFReceiver->OnButtonFunctionTriggered.AddDynamic(this, &ARFTriggerControllerExample::HandleButtonFunctionTriggered);
	RFReceiver->OnCodeLearned.AddDynamic(this, &ARFTriggerControllerExample::OnRemoteCodeLearned);
	
	// Load saved button mappings (if any)
	RFReceiver->LoadButtonMappings();
	
	// If no buttons are learned yet, enable learning mode
	if (RFReceiver->GetLearnedButtonCount() == 0)
	{
		UE_LOG(LogTemp, Log, TEXT("RFTriggerControllerExample: No learned buttons found - enabling learning mode"));
		RFReceiver->EnableLearningMode(60.0f);  // 60 second timeout
	}
	else
	{
		// Assign function names to learned buttons (if not already assigned)
		// Button 0 = HeightUp, Button 1 = HeightDown, Button 2 = HeightFineUp, Button 3 = HeightFineDown
		TArray<FRF433MHzLearnedButton> LearnedButtons;
		RFReceiver->GetLearnedButtons(LearnedButtons);
		
		for (const FRF433MHzLearnedButton& Button : LearnedButtons)
		{
			if (!Button.bIsMapped)
			{
				FString FunctionName;
				if (Button.ButtonCode == 0) FunctionName = TEXT("HeightUp");
				else if (Button.ButtonCode == 1) FunctionName = TEXT("HeightDown");
				else if (Button.ButtonCode == 2) FunctionName = TEXT("HeightFineUp");
				else if (Button.ButtonCode == 3) FunctionName = TEXT("HeightFineDown");
				else FunctionName = FString::Printf(TEXT("Button%d"), Button.ButtonCode);
				
				RFReceiver->AssignButtonFunction(Button.ButtonCode, FunctionName);
			}
		}
	}
	
	UE_LOG(LogTemp, Log, TEXT("RFTriggerControllerExample: Height calibration system initialized (%d learned buttons)"), 
		RFReceiver->GetLearnedButtonCount());
}

void ARFTriggerControllerExample::HandleButtonPressed(int32 ButtonCode)
{
	// Button pressed event (raw button code)
	// Function mapping is handled by HandleButtonFunctionTriggered
	UE_LOG(LogTemp, Verbose, TEXT("RFTriggerControllerExample: Button %d pressed"), ButtonCode);
}

void ARFTriggerControllerExample::HandleButtonFunctionTriggered(int32 ButtonCode, const FString& FunctionName, bool bPressed)
{
	// Handle button function mapping (uses assigned function names)
	// This is the preferred method - uses learned button mappings
	
	if (!bPressed)
	{
		// Button released - stop movement
		StopWinchMovement();
		return;
	}
	
	// Map function names to winch commands
	if (FunctionName == TEXT("HeightUp"))
	{
		AdjustWinchHeight(6.0f);  // +6 inches
	}
	else if (FunctionName == TEXT("HeightDown"))
	{
		AdjustWinchHeight(-6.0f);  // -6 inches
	}
	else if (FunctionName == TEXT("HeightFineUp"))
	{
		AdjustWinchHeight(1.0f);  // +1 inch
	}
	else if (FunctionName == TEXT("HeightFineDown"))
	{
		AdjustWinchHeight(-1.0f);  // -1 inch
	}
	
	UE_LOG(LogTemp, Log, TEXT("RFTriggerControllerExample: Function '%s' triggered (Button %d)"), *FunctionName, ButtonCode);
}

void ARFTriggerControllerExample::HandleButtonReleased(int32 ButtonCode)
{
	// Stop winch movement when button is released
	StopWinchMovement();
	
	UE_LOG(LogTemp, Log, TEXT("RFTriggerControllerExample: Button %d released"), ButtonCode);
}

void ARFTriggerControllerExample::AdjustWinchHeight(float DeltaInches)
{
	// NOOP: Implement winch height adjustment
	// This would send commands to SuperheroFlightExperience ECU
	// Example:
	// if (SuperheroFlightECUController)
	// {
	//     SuperheroFlightECUController->AdjustWinchHeight(DeltaInches);
	// }
}

void ARFTriggerControllerExample::StopWinchMovement()
{
	// NOOP: Implement winch stop
	// This would send stop command to SuperheroFlightExperience ECU
}

// =====================================
// Example: Wireless Trigger Buttons (Costume-Embedded)
// =====================================

void ARFTriggerControllerExample::InitializeWirelessTriggers()
{
	// Create RF433MHz receiver component for wireless trigger buttons
	// (e.g., buttons embedded in live actor's costume for AIFacemaskExperience)
	
	RFReceiver = NewObject<URF433MHzReceiver>(this);
	if (!RFReceiver)
	{
		return;
	}
	
	FRF433MHzReceiverConfig Config;
	Config.ReceiverType = ERF433MHzReceiverType::Generic;  // Generic USB receiver
	Config.USBDevicePath = TEXT("COM4");
	Config.bEnableRollingCodeValidation = true;
	Config.bEnableReplayAttackPrevention = true;
	
	if (!RFReceiver->InitializeReceiver(Config))
	{
		return;
	}
	
	// Subscribe to button events
	RFReceiver->OnButtonPressed.AddDynamic(this, &ARFTriggerControllerExample::HandleTriggerButtonPressed);
	
	UE_LOG(LogTemp, Log, TEXT("RFTriggerControllerExample: Wireless trigger system initialized"));
}

void ARFTriggerControllerExample::HandleTriggerButtonPressed(int32 ButtonCode)
{
	// Raw button pressed event (fallback if function mapping not used)
	UE_LOG(LogTemp, Verbose, TEXT("RFTriggerControllerExample: Trigger button %d pressed"), ButtonCode);
}

void ARFTriggerControllerExample::HandleTriggerButtonFunction(int32 ButtonCode, const FString& FunctionName, bool bPressed)
{
	// Handle button function mapping (uses assigned function names)
	if (!bPressed)
	{
		return;  // Only process press events
	}
	
	if (FunctionName == TEXT("AdvanceNarrative"))
	{
		// Advance narrative state
		// if (AIFacemaskExperience)
		// {
		//     AIFacemaskExperience->RequestAdvanceExperience();
		// }
		UE_LOG(LogTemp, Log, TEXT("RFTriggerControllerExample: Advance narrative triggered (Button %d)"), ButtonCode);
	}
	else if (FunctionName == TEXT("RetreatNarrative"))
	{
		// Retreat narrative state
		// if (AIFacemaskExperience)
		// {
		//     AIFacemaskExperience->RequestRetreatExperience();
		// }
		UE_LOG(LogTemp, Log, TEXT("RFTriggerControllerExample: Retreat narrative triggered (Button %d)"), ButtonCode);
	}
}

// =====================================
// Example: Emergency Stop Remote
// =====================================

void ARFTriggerControllerExample::InitializeEmergencyStop()
{
	RFReceiver = NewObject<URF433MHzReceiver>(this);
	if (!RFReceiver)
	{
		return;
	}
	
	FRF433MHzReceiverConfig Config;
	Config.ReceiverType = ERF433MHzReceiverType::RTL_SDR;  // RTL-SDR USB dongle
	Config.USBDevicePath = TEXT("");  // RTL-SDR uses different device path format
	Config.bEnableRollingCodeValidation = true;  // Critical for e-stop security
	Config.bEnableReplayAttackPrevention = true;
	
	if (!RFReceiver->InitializeReceiver(Config))
	{
		return;
	}
	
	// Subscribe to button events
	RFReceiver->OnButtonPressed.AddDynamic(this, &ARFTriggerControllerExample::HandleEmergencyStop);
	
	UE_LOG(LogTemp, Log, TEXT("RFTriggerControllerExample: Emergency stop remote initialized"));
}

void ARFTriggerControllerExample::HandleEmergencyStop(int32 ButtonCode)
{
	// Emergency stop button pressed - trigger e-stop on all systems
	// if (SuperheroFlightECUController)
	// {
	//     SuperheroFlightECUController->EmergencyStop();
	// }
	// if (GunshipECUController)
	// {
	//     GunshipECUController->EmergencyStop();
	// }
	
	UE_LOG(LogTemp, Warning, TEXT("RFTriggerControllerExample: EMERGENCY STOP triggered via RF remote"));
}

// =====================================
// Example: Safety Interlock Enforcement
// =====================================

void ARFTriggerControllerExample::ProcessCalibrationButton(int32 ButtonCode, bool bPressed)
{
	// Enforce safety interlocks for calibration mode
	// - Calibration only works when playSessionActive = false
	// - Winch movement limited to small increments
	// - Emergency stop always active
	// - Timeout after 5 minutes of inactivity
	
	if (!bIsCalibrationMode)
	{
		UE_LOG(LogTemp, Warning, TEXT("RFTriggerControllerExample: Calibration button ignored - not in calibration mode"));
		return;
	}
	
	if (bPlaySessionActive)
	{
		UE_LOG(LogTemp, Warning, TEXT("RFTriggerControllerExample: Calibration button ignored - play session active"));
		return;
	}
	
	// Check timeout
	float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastCalibrationActivity > 300.0f)  // 5 minutes (300 seconds)
	{
		UE_LOG(LogTemp, Warning, TEXT("RFTriggerControllerExample: Calibration mode timeout"));
		bIsCalibrationMode = false;
		return;
	}
	
	// Update activity timestamp
	LastCalibrationActivity = CurrentTime;
	
	// Process button (movement limits enforced in AdjustWinchHeight)
	if (bPressed)
	{
		HandleButtonPressed(ButtonCode);
	}
	else
	{
		HandleButtonReleased(ButtonCode);
	}
}

// =====================================
// Example: Rolling Code Validation Status
// =====================================

void ARFTriggerControllerExample::CheckRollingCodeStatus()
{
	if (!RFReceiver)
	{
		return;
	}
	
	// Get rolling code validation status
	bool bValid = RFReceiver->IsRollingCodeValid();
	int32 CodeDrift = RFReceiver->GetRollingCodeDrift();
	
	if (!bValid)
	{
		UE_LOG(LogTemp, Warning, TEXT("RFTriggerControllerExample: Rolling code validation failed (drift: %d)"), CodeDrift);
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("RFTriggerControllerExample: Rolling code valid (drift: %d)"), CodeDrift);
	}
}

// =====================================
// Example: Code Learning Mode
// =====================================

void ARFTriggerControllerExample::EnableCodeLearningMode()
{
	if (!RFReceiver)
	{
		return;
	}
	
	// Enable learning mode to pair new remotes
	RFReceiver->EnableLearningMode(30.0f);  // 30 second timeout
	
	UE_LOG(LogTemp, Log, TEXT("RFTriggerControllerExample: Code learning mode enabled"));
}

void ARFTriggerControllerExample::OnRemoteCodeLearned(int32 ButtonCode, int32 RollingCode)
{
	// New remote code learned during learning mode
	UE_LOG(LogTemp, Log, TEXT("RFTriggerControllerExample: Learned new remote - button=%d, code=0x%06X"), 
		ButtonCode, RollingCode);
	
	// Auto-assign function names based on button code (can be customized by Ops Tech later)
	FString FunctionName;
	if (ButtonCode == 0) FunctionName = TEXT("HeightUp");
	else if (ButtonCode == 1) FunctionName = TEXT("HeightDown");
	else if (ButtonCode == 2) FunctionName = TEXT("HeightFineUp");
	else if (ButtonCode == 3) FunctionName = TEXT("HeightFineDown");
	else FunctionName = FString::Printf(TEXT("Button%d"), ButtonCode);
	
	if (RFReceiver)
	{
		RFReceiver->AssignButtonFunction(ButtonCode, FunctionName);
		UE_LOG(LogTemp, Log, TEXT("RFTriggerControllerExample: Auto-assigned function '%s' to button %d"), 
			*FunctionName, ButtonCode);
	}
	
	// Button mappings are automatically saved to JSON (auto-save enabled by default)
}

// =====================================
// Example: Multiple Receivers (Multiple Remotes)
// =====================================

void ARFTriggerControllerExample::InitializeMultipleReceivers()
{
	// Example: Multiple USB receivers for different remotes
	// - Receiver 1: Height calibration clicker (COM3)
	// - Receiver 2: Wireless trigger buttons (COM4)
	// - Receiver 3: Emergency stop remote (RTL-SDR)
	
	// Height calibration receiver
	HeightCalibrationReceiver = NewObject<URF433MHzReceiver>(this);
	if (HeightCalibrationReceiver)
	{
		FRF433MHzReceiverConfig Config;
		Config.ReceiverType = ERF433MHzReceiverType::CC1101;
		Config.USBDevicePath = TEXT("COM3");
		HeightCalibrationReceiver->InitializeReceiver(Config);
		HeightCalibrationReceiver->OnButtonPressed.AddDynamic(this, &ARFTriggerControllerExample::OnCalibrationButtonPressed);
	}
	
	// Wireless trigger receiver
	TriggerReceiver = NewObject<URF433MHzReceiver>(this);
	if (TriggerReceiver)
	{
		FRF433MHzReceiverConfig Config;
		Config.ReceiverType = ERF433MHzReceiverType::Generic;
		Config.USBDevicePath = TEXT("COM4");
		TriggerReceiver->InitializeReceiver(Config);
		TriggerReceiver->OnButtonPressed.AddDynamic(this, &ARFTriggerControllerExample::HandleTriggerButtonPressed);
	}
	
	// Emergency stop receiver
	EmergencyStopReceiver = NewObject<URF433MHzReceiver>(this);
	if (EmergencyStopReceiver)
	{
		FRF433MHzReceiverConfig Config;
		Config.ReceiverType = ERF433MHzReceiverType::RTL_SDR;
		Config.USBDevicePath = TEXT("");  // RTL-SDR uses different path format
		EmergencyStopReceiver->InitializeReceiver(Config);
		EmergencyStopReceiver->OnButtonPressed.AddDynamic(this, &ARFTriggerControllerExample::HandleEmergencyStop);
	}
}

// =====================================
// Example: Integration with SuperheroFlightExperience
// =====================================

void ARFTriggerControllerExample::InitializeSuperheroFlightCalibration()
{
	// This is how SuperheroFlightExperience would use the RF433MHz API
	
	// Create receiver component
	RFReceiver = NewObject<URF433MHzReceiver>(this);
	if (!RFReceiver)
	{
		return;
	}
	
	// Configure for height calibration
	FRF433MHzReceiverConfig Config;
	Config.ReceiverType = ERF433MHzReceiverType::CC1101;  // Or RTL-SDR, RFM69, Generic
	Config.USBDevicePath = TEXT("COM3");  // USB receiver dongle
	Config.bEnableRollingCodeValidation = true;  // Security: prevent replay attacks
	Config.bEnableReplayAttackPrevention = true;
	Config.RollingCodeSeed = 0x12345678;  // Must match remote firmware
	Config.ReplayAttackWindow = 100;  // Reject codes within 100ms
	
	if (!RFReceiver->InitializeReceiver(Config))
	{
		UE_LOG(LogTemp, Error, TEXT("SuperheroFlightExperience: Failed to initialize RF receiver"));
		return;
	}
	
	// Subscribe to button events
	RFReceiver->OnButtonPressed.AddDynamic(this, &ARFTriggerControllerExample::OnCalibrationButtonPressed);
	RFReceiver->OnButtonReleased.AddDynamic(this, &ARFTriggerControllerExample::OnCalibrationButtonReleased);
	
	// Set calibration mode
	bIsCalibrationMode = true;
	LastCalibrationActivity = GetWorld()->GetTimeSeconds();
	
	UE_LOG(LogTemp, Log, TEXT("SuperheroFlightExperience: Height calibration system ready"));
}

void ARFTriggerControllerExample::OnCalibrationButtonPressed(int32 ButtonCode)
{
	// Enforce safety interlocks
	if (bPlaySessionActive)
	{
		UE_LOG(LogTemp, Warning, TEXT("SuperheroFlightExperience: Calibration ignored - play session active"));
		return;
	}
	
	// Check timeout
	float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastCalibrationActivity > 300.0f)  // 5 minutes (300 seconds)
	{
		UE_LOG(LogTemp, Warning, TEXT("SuperheroFlightExperience: Calibration mode timeout"));
		bIsCalibrationMode = false;
		return;
	}
	
	// Update activity timestamp
	LastCalibrationActivity = CurrentTime;
	
	// Map button to winch command
	float DeltaInches = 0.0f;
	if (ButtonCode == 0) DeltaInches = 6.0f;   // Up
	else if (ButtonCode == 1) DeltaInches = -6.0f;  // Down
	else if (ButtonCode == 2) DeltaInches = 1.0f;   // Fine Up
	else if (ButtonCode == 3) DeltaInches = -1.0f; // Fine Down
	
	// Send winch command to ECU (with movement limit enforcement)
	if (FMath::Abs(DeltaInches) > 0.0f)
	{
		// if (SuperheroFlightECUController)
		// {
		//     SuperheroFlightECUController->AdjustWinchHeight(DeltaInches);
		// }
	}
}

void ARFTriggerControllerExample::OnCalibrationButtonReleased(int32 ButtonCode)
{
	// Stop winch movement
	// if (SuperheroFlightECUController)
	// {
	//     SuperheroFlightECUController->StopWinchMovement();
	// }
}

