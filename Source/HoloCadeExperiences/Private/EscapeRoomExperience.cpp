// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "EscapeRoomExperience.h"
#include "EmbeddedDeviceController.h"
#include "ExperienceLoop/ExperienceStateMachine.h"

AEscapeRoomExperience::AEscapeRoomExperience()
{
	// Enable narrative state machine by default for escape rooms
	bUseNarrativeStateMachine = true;

	// Initialize arrays
	DoorUnlockStates.SetNum(NumberOfDoors);
	PropSensorValues.SetNum(NumberOfProps);

	// Initialize all doors as locked
	for (int32 i = 0; i < DoorUnlockStates.Num(); i++)
	{
		DoorUnlockStates[i] = false;
	}

	// Initialize all prop sensors to 0
	for (int32 i = 0; i < PropSensorValues.Num(); i++)
	{
		PropSensorValues[i] = 0.0f;
	}

	// Initialize default state-to-door mapping (example mappings)
	// Users can modify these in Blueprint or override in C++
	StateToDoorMapping.Add(FName("Puzzle1"), 0);
	StateToDoorMapping.Add(FName("Puzzle2"), 1);
	StateToDoorMapping.Add(FName("Puzzle3"), 2);
	StateToDoorMapping.Add(FName("Finale"), 3);
}

bool AEscapeRoomExperience::InitializeExperienceImpl()
{
	if (!Super::InitializeExperienceImpl())
	{
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("EscapeRoomExperience: Initializing escape room experience..."));

	// Initialize embedded devices
	InitializeEmbeddedDevices();

	// Initialize narrative state machine with default escape room states
	if (NarrativeStateMachine && bUseNarrativeStateMachine)
	{
		TArray<FExperienceState> DefaultStates;
		DefaultStates.Add(FExperienceState(FName("Intro"), TEXT("Introduction and briefing")));
		DefaultStates.Add(FExperienceState(FName("Puzzle1"), TEXT("First puzzle challenge")));
		DefaultStates.Add(FExperienceState(FName("Puzzle2"), TEXT("Second puzzle challenge")));
		DefaultStates.Add(FExperienceState(FName("Puzzle3"), TEXT("Third puzzle challenge")));
		DefaultStates.Add(FExperienceState(FName("Finale"), TEXT("Final challenge and escape")));
		DefaultStates.Add(FExperienceState(FName("Credits"), TEXT("Completion and credits")));

		NarrativeStateMachine->Initialize(DefaultStates);
		UE_LOG(LogTemp, Log, TEXT("EscapeRoomExperience: Narrative state machine initialized with %d states"), DefaultStates.Num());
	}

	UE_LOG(LogTemp, Log, TEXT("EscapeRoomExperience: Initialization complete"));
	return true;
}

void AEscapeRoomExperience::ShutdownExperienceImpl()
{
	// Disconnect embedded devices
	if (DoorController)
	{
		DoorController->DisconnectDevice();
	}

	if (PropController)
	{
		PropController->DisconnectDevice();
	}

	Super::ShutdownExperienceImpl();
}

void AEscapeRoomExperience::InitializeEmbeddedDevices()
{
	// Create door controller if not already created
	if (!DoorController)
	{
		DoorController = NewObject<UEmbeddedDeviceController>(this, UEmbeddedDeviceController::StaticClass());
	}

	if (DoorController)
	{
		// Configure for door locks (example: ESP32 over WiFi)
		FEmbeddedDeviceConfig DoorConfig;
		DoorConfig.DeviceType = EHoloCadeMicrocontrollerType::ESP32;
		DoorConfig.Protocol = EHoloCadeCommProtocol::WiFi;
		DoorConfig.DeviceAddress = TEXT("192.168.1.50");  // Change to your door controller IP
		DoorConfig.Port = 8888;
		DoorConfig.InputChannelCount = NumberOfDoors;  // One input per door (lock state)
		DoorConfig.OutputChannelCount = NumberOfDoors; // One output per door (unlock command)
		DoorConfig.bDebugMode = false;  // Use binary mode in production

		if (DoorController->InitializeDevice(DoorConfig))
		{
			// Bind to door state change events
			DoorController->OnBoolReceived.AddDynamic(this, &AEscapeRoomExperience::OnDoorStateChanged);
			UE_LOG(LogTemp, Log, TEXT("EscapeRoomExperience: Door controller initialized at %s:%d"), 
				*DoorConfig.DeviceAddress, DoorConfig.Port);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("EscapeRoomExperience: Failed to initialize door controller"));
		}
	}

	// Create prop controller if needed
	if (NumberOfProps > 0 && !PropController)
	{
		PropController = NewObject<UEmbeddedDeviceController>(this, UEmbeddedDeviceController::StaticClass());
	}

	if (PropController && NumberOfProps > 0)
	{
		// Configure for props (example: ESP32 over WiFi)
		FEmbeddedDeviceConfig PropConfig;
		PropConfig.DeviceType = EHoloCadeMicrocontrollerType::ESP32;
		PropConfig.Protocol = EHoloCadeCommProtocol::WiFi;
		PropConfig.DeviceAddress = TEXT("192.168.1.51");  // Change to your prop controller IP
		PropConfig.Port = 8888;
		PropConfig.InputChannelCount = NumberOfProps;  // One input per prop (sensor)
		PropConfig.OutputChannelCount = NumberOfProps; // One output per prop (actuator)

		if (PropController->InitializeDevice(PropConfig))
		{
			// Bind to prop sensor events
			PropController->OnFloatReceived.AddDynamic(this, &AEscapeRoomExperience::OnPropSensorValue);
			UE_LOG(LogTemp, Log, TEXT("EscapeRoomExperience: Prop controller initialized at %s:%d"), 
				*PropConfig.DeviceAddress, PropConfig.Port);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("EscapeRoomExperience: Failed to initialize prop controller"));
		}
	}
}

bool AEscapeRoomExperience::UnlockDoor(int32 DoorIndex)
{
	if (DoorIndex < 0 || DoorIndex >= NumberOfDoors)
	{
		UE_LOG(LogTemp, Warning, TEXT("EscapeRoomExperience: Invalid door index %d"), DoorIndex);
		return false;
	}

	if (!DoorController || !DoorController->IsDeviceConnected())
	{
		UE_LOG(LogTemp, Warning, TEXT("EscapeRoomExperience: Door controller not connected"));
		return false;
	}

	// Send unlock command (bool true = unlock) to DoorLock_Example.ino firmware
	// Note: This sends the command to the firmware. For unlock confirmation callback,
	// bind to OnDoorUnlockConfirmed delegate. The firmware will send back confirmation
	// when the door actually unlocks, which triggers OnDoorStateChanged and fires OnDoorUnlockConfirmed.
	DoorController->SendBool(DoorIndex, true);
	
	UE_LOG(LogTemp, Log, TEXT("EscapeRoomExperience: Unlock command sent to door %d"), DoorIndex);
	return true;
}

bool AEscapeRoomExperience::LockDoor(int32 DoorIndex)
{
	if (DoorIndex < 0 || DoorIndex >= NumberOfDoors)
	{
		UE_LOG(LogTemp, Warning, TEXT("EscapeRoomExperience: Invalid door index %d"), DoorIndex);
		return false;
	}

	if (!DoorController || !DoorController->IsDeviceConnected())
	{
		UE_LOG(LogTemp, Warning, TEXT("EscapeRoomExperience: Door controller not connected"));
		return false;
	}

	// Send lock command (bool false = lock)
	DoorController->SendBool(DoorIndex, false);
	
	UE_LOG(LogTemp, Log, TEXT("EscapeRoomExperience: Lock command sent to door %d"), DoorIndex);
	return true;
}

bool AEscapeRoomExperience::IsDoorUnlocked(int32 DoorIndex) const
{
	if (DoorIndex < 0 || DoorIndex >= DoorUnlockStates.Num())
	{
		return false;
	}

	return DoorUnlockStates[DoorIndex];
}

bool AEscapeRoomExperience::TriggerPropAction(int32 PropIndex, float ActionValue)
{
	if (PropIndex < 0 || PropIndex >= NumberOfProps)
	{
		UE_LOG(LogTemp, Warning, TEXT("EscapeRoomExperience: Invalid prop index %d"), PropIndex);
		return false;
	}

	if (!PropController || !PropController->IsDeviceConnected())
	{
		UE_LOG(LogTemp, Warning, TEXT("EscapeRoomExperience: Prop controller not connected"));
		return false;
	}

	// Send action command (float 0.0-1.0 for intensity/position)
	PropController->SendFloat(PropIndex, FMath::Clamp(ActionValue, 0.0f, 1.0f));
	
	UE_LOG(LogTemp, Log, TEXT("EscapeRoomExperience: Prop action triggered on prop %d (value: %.2f)"), PropIndex, ActionValue);
	return true;
}

float AEscapeRoomExperience::ReadPropSensor(int32 PropIndex) const
{
	if (PropIndex < 0 || PropIndex >= PropSensorValues.Num())
	{
		return 0.0f;
	}

	return PropSensorValues[PropIndex];
}

FName AEscapeRoomExperience::GetCurrentPuzzleState() const
{
	return GetCurrentNarrativeState();
}

void AEscapeRoomExperience::OnNarrativeStateChanged(FName OldState, FName NewState, int32 NewStateIndex)
{
	// Note: OnNarrativeStateChanged is a BlueprintImplementableEvent in the base class,
	// so we can't call Super::. This C++ implementation handles the door unlocking logic.
	// Blueprint can still override this if needed.

	UE_LOG(LogTemp, Log, TEXT("EscapeRoomExperience: Narrative state changed from %s to %s"), 
		*OldState.ToString(), *NewState.ToString());

	// Check if this state maps to a door
	int32 DoorIndex = GetDoorIndexForState(NewState);
	if (DoorIndex >= 0 && DoorIndex < NumberOfDoors)
	{
		UnlockDoor(DoorIndex);
		UE_LOG(LogTemp, Log, TEXT("EscapeRoomExperience: Automatically unlocked door %d for state %s"), 
			DoorIndex, *NewState.ToString());
	}
}

int32 AEscapeRoomExperience::GetDoorIndexForState(FName StateName) const
{
	const int32* DoorIndexPtr = StateToDoorMapping.Find(StateName);
	if (DoorIndexPtr != nullptr)
	{
		return *DoorIndexPtr;
	}
	return -1; // No mapping found
}

void AEscapeRoomExperience::OnDoorStateChanged(int32 Channel, bool bIsUnlocked)
{
	if (Channel >= 0 && Channel < DoorUnlockStates.Num())
	{
		bool bWasUnlocked = DoorUnlockStates[Channel];
		DoorUnlockStates[Channel] = bIsUnlocked;
		
		UE_LOG(LogTemp, Log, TEXT("EscapeRoomExperience: Door %d state changed to %s"), 
			Channel, bIsUnlocked ? TEXT("UNLOCKED") : TEXT("LOCKED"));
		
		// Fire callback when door transitions to unlocked state
		// This provides confirmation that the unlock command was received and executed by the firmware
		if (bIsUnlocked && !bWasUnlocked)
		{
			OnDoorUnlockConfirmed.Broadcast(Channel);
			UE_LOG(LogTemp, Log, TEXT("EscapeRoomExperience: Door %d unlock confirmed by firmware"), Channel);
		}
	}
}

void AEscapeRoomExperience::OnPropSensorValue(int32 Channel, float Value)
{
	if (Channel >= 0 && Channel < PropSensorValues.Num())
	{
		PropSensorValues[Channel] = Value;
		UE_LOG(LogTemp, VeryVerbose, TEXT("EscapeRoomExperience: Prop %d sensor value: %.3f"), Channel, Value);
	}
}

