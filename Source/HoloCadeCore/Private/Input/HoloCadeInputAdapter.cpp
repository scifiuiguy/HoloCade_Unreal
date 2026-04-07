// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "Input/HoloCadeInputAdapter.h"
#include "HoloCadeEmbeddedDeviceInterface.h"
#include "Net/UnrealNetwork.h"

UHoloCadeInputAdapter::UHoloCadeInputAdapter()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	// Enable replication
	SetIsReplicatedByDefault(true);
}

void UHoloCadeInputAdapter::BeginPlay()
{
	Super::BeginPlay();

	// Initialize axis arrays
	ReplicatedAxisValues.SetNum(AxisCount);
	PreviousAxisValues.SetNum(AxisCount);

	for (int32 i = 0; i < AxisCount; i++)
	{
		ReplicatedAxisValues[i] = 0.0f;
		PreviousAxisValues[i] = 0.0f;
	}
}

void UHoloCadeInputAdapter::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Only process input on authority (server or listen server host)
	if (GetOwner()->HasAuthority())
	{
		// Process embedded system input (ESP32, Arduino, etc.)
		if (bEnableEmbeddedSystemInput)
		{
			ProcessEmbeddedSystemInput();
		}

		// Process VR controller input (Blueprint override)
		if (bEnableVRControllerInput)
		{
			ProcessVRControllerInput();
		}
	}
}

void UHoloCadeInputAdapter::ProcessEmbeddedSystemInput()
{
	// Only process if embedded device is connected
	IHoloCadeEmbeddedDeviceInterface* Device = EmbeddedDeviceController.GetInterface();
	if (!Device || !Device->IsDeviceConnected())
	{
		return;
	}

	// NOTE: This function only runs on authority
	// Authority check is done in TickComponent() before calling this function

	// Read button states from embedded device
	for (int32 i = 0; i < ButtonCount; i++)
	{
		bool bCurrentState = Device->GetDigitalInput(i);
		bool bPreviousState = (PreviousButtonStates & (1 << i)) != 0;

		// Edge detection: trigger on state change
		if (bCurrentState != bPreviousState)
		{
			UpdateButtonState(i, bCurrentState);
		}
	}

	// Read axis values from embedded device
	for (int32 i = 0; i < AxisCount; i++)
	{
		float CurrentValue = Device->GetAnalogInput(i);
		float PreviousValue = PreviousAxisValues[i];

		// Only update if value changed significantly (prevent noise)
		if (FMath::Abs(CurrentValue - PreviousValue) > 0.01f)
		{
			UpdateAxisValue(i, CurrentValue);
		}
	}
}

void UHoloCadeInputAdapter::InjectButtonPress(int32 ButtonIndex)
{
	if (ButtonIndex < 0 || ButtonIndex >= 32)
	{
		UE_LOG(LogTemp, Warning, TEXT("HoloCadeInputAdapter: Invalid button index %d (must be 0-31)"), ButtonIndex);
		return;
	}

	if (GetOwner()->HasAuthority())
	{
		// We're on authority - update directly
		UpdateButtonState(ButtonIndex, true);
	}
	else
	{
		// We're on client - send RPC to server
		ServerInjectButtonPress(ButtonIndex);
	}
}

void UHoloCadeInputAdapter::InjectButtonRelease(int32 ButtonIndex)
{
	if (ButtonIndex < 0 || ButtonIndex >= 32)
	{
		UE_LOG(LogTemp, Warning, TEXT("HoloCadeInputAdapter: Invalid button index %d (must be 0-31)"), ButtonIndex);
		return;
	}

	if (GetOwner()->HasAuthority())
	{
		// We're on authority - update directly
		UpdateButtonState(ButtonIndex, false);
	}
	else
	{
		// We're on client - send RPC to server
		ServerInjectButtonRelease(ButtonIndex);
	}
}

void UHoloCadeInputAdapter::InjectAxisValue(int32 AxisIndex, float Value)
{
	if (AxisIndex < 0 || AxisIndex >= AxisCount)
	{
		UE_LOG(LogTemp, Warning, TEXT("HoloCadeInputAdapter: Invalid axis index %d (must be 0-%d)"), AxisIndex, AxisCount - 1);
		return;
	}

	if (GetOwner()->HasAuthority())
	{
		// We're on authority - update directly
		UpdateAxisValue(AxisIndex, Value);
	}
	else
	{
		// We're on client - send RPC to server
		ServerInjectAxisValue(AxisIndex, Value);
	}
}

bool UHoloCadeInputAdapter::IsButtonPressed(int32 ButtonIndex) const
{
	if (ButtonIndex < 0 || ButtonIndex >= 32)
	{
		return false;
	}

	return (ReplicatedButtonStates & (1 << ButtonIndex)) != 0;
}

float UHoloCadeInputAdapter::GetAxisValue(int32 AxisIndex) const
{
	if (AxisIndex < 0 || AxisIndex >= ReplicatedAxisValues.Num())
	{
		return 0.0f;
	}

	return ReplicatedAxisValues[AxisIndex];
}

void UHoloCadeInputAdapter::UpdateButtonState(int32 ButtonIndex, bool bPressed)
{
	// This function only runs on authority

	// Get previous state
	bool bPreviousState = (PreviousButtonStates & (1 << ButtonIndex)) != 0;

	// Update previous state
	if (bPressed)
	{
		PreviousButtonStates |= (1 << ButtonIndex);
	}
	else
	{
		PreviousButtonStates &= ~(1 << ButtonIndex);
	}

	// Update replicated state (will replicate to clients)
	if (bPressed)
	{
		ReplicatedButtonStates |= (1 << ButtonIndex);
	}
	else
	{
		ReplicatedButtonStates &= ~(1 << ButtonIndex);
	}

	// Broadcast change (on server)
	BroadcastButtonChange(ButtonIndex, bPressed);
}

void UHoloCadeInputAdapter::UpdateAxisValue(int32 AxisIndex, float Value)
{
	// This function only runs on authority

	// Update previous value
	if (AxisIndex >= 0 && AxisIndex < PreviousAxisValues.Num())
	{
		PreviousAxisValues[AxisIndex] = Value;
	}

	// Update replicated value (will replicate to clients)
	if (AxisIndex >= 0 && AxisIndex < ReplicatedAxisValues.Num())
	{
		ReplicatedAxisValues[AxisIndex] = Value;
	}

	// Broadcast change (on server)
	BroadcastAxisChange(AxisIndex, Value);
}

void UHoloCadeInputAdapter::BroadcastButtonChange(int32 ButtonIndex, bool bPressed)
{
	if (bPressed)
	{
		OnButtonPressed.Broadcast(ButtonIndex);
	}
	else
	{
		OnButtonReleased.Broadcast(ButtonIndex);
	}
}

void UHoloCadeInputAdapter::BroadcastAxisChange(int32 AxisIndex, float Value)
{
	OnAxisChanged.Broadcast(AxisIndex, Value);
}

void UHoloCadeInputAdapter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UHoloCadeInputAdapter, ReplicatedButtonStates);
	DOREPLIFETIME(UHoloCadeInputAdapter, ReplicatedAxisValues);
}

void UHoloCadeInputAdapter::OnRep_ButtonStates()
{
	// This fires on clients when button states change

	// Compare against previous state to detect edges
	for (int32 i = 0; i < ButtonCount; i++)
	{
		bool bCurrentState = (ReplicatedButtonStates & (1 << i)) != 0;
		bool bPreviousState = (PreviousButtonStates & (1 << i)) != 0;

		if (bCurrentState != bPreviousState)
		{
			// Update previous state
			if (bCurrentState)
			{
				PreviousButtonStates |= (1 << i);
			}
			else
			{
				PreviousButtonStates &= ~(1 << i);
			}

			// Broadcast change on client
			BroadcastButtonChange(i, bCurrentState);
		}
	}
}

void UHoloCadeInputAdapter::OnRep_AxisValues()
{
	// This fires on clients when axis values change

	// Broadcast all axis changes
	for (int32 i = 0; i < ReplicatedAxisValues.Num(); i++)
	{
		float CurrentValue = ReplicatedAxisValues[i];
		float PreviousValue = (i < PreviousAxisValues.Num()) ? PreviousAxisValues[i] : 0.0f;

		// Only broadcast if value changed significantly
		if (FMath::Abs(CurrentValue - PreviousValue) > 0.01f)
		{
			// Update previous value
			if (i < PreviousAxisValues.Num())
			{
				PreviousAxisValues[i] = CurrentValue;
			}

			// Broadcast change on client
			BroadcastAxisChange(i, CurrentValue);
		}
	}
}

void UHoloCadeInputAdapter::ServerInjectButtonPress_Implementation(int32 ButtonIndex)
{
	// Server RPC: Client requested button press
	UpdateButtonState(ButtonIndex, true);
}

bool UHoloCadeInputAdapter::ServerInjectButtonPress_Validate(int32 ButtonIndex)
{
	// Validate button index
	return ButtonIndex >= 0 && ButtonIndex < 32;
}

void UHoloCadeInputAdapter::ServerInjectButtonRelease_Implementation(int32 ButtonIndex)
{
	// Server RPC: Client requested button release
	UpdateButtonState(ButtonIndex, false);
}

bool UHoloCadeInputAdapter::ServerInjectButtonRelease_Validate(int32 ButtonIndex)
{
	// Validate button index
	return ButtonIndex >= 0 && ButtonIndex < 32;
}

void UHoloCadeInputAdapter::ServerInjectAxisValue_Implementation(int32 AxisIndex, float Value)
{
	// Server RPC: Client requested axis value change
	UpdateAxisValue(AxisIndex, Value);
}

bool UHoloCadeInputAdapter::ServerInjectAxisValue_Validate(int32 AxisIndex, float Value)
{
	// Validate axis index and value range
	return AxisIndex >= 0 && AxisIndex < AxisCount && FMath::Abs(Value) <= 10.0f;
}

