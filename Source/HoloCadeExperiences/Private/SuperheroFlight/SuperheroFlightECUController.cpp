// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "SuperheroFlight/SuperheroFlightECUController.h"
#include "HoloCadeExperiences.h"
#include "Networking/HoloCadeUDPTransport.h"

USuperheroFlightECUController::USuperheroFlightECUController()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.05f; // Tick every 50ms for 20 Hz winch state updates
	bECUConnected = false;
	ECUPort = 8888;
	ConnectionTimeout = 2.0f;
}

USuperheroFlightECUController::~USuperheroFlightECUController()
{
	ShutdownECU();
}

void USuperheroFlightECUController::BeginPlay()
{
	Super::BeginPlay();
	// NOOP: ECU initialization should be called explicitly by experience
}

void USuperheroFlightECUController::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Check for connection timeout
	if (bECUConnected)
	{
		float CurrentTime = GetWorld()->GetTimeSeconds();
		if (CurrentTime - LastWinchStateTime > ConnectionTimeout &&
			CurrentTime - LastTelemetryTime > ConnectionTimeout)
		{
			// Connection lost
			bECUConnected = false;
			UE_LOG(LogSuperheroFlight, Warning, TEXT("SuperheroFlightECU: Connection timeout"));
		}
	}
}

bool USuperheroFlightECUController::InitializeECU(const FString& InECUIPAddress, int32 InECUPort)
{
	ECUIPAddress = InECUIPAddress;
	ECUPort = InECUPort;

	if (!UDPTransport)
	{
		UDPTransport = NewObject<UHoloCadeUDPTransport>(this);
	}

	if (!UDPTransport->InitializeUDPConnection(ECUIPAddress, ECUPort, TEXT("SuperheroFlight_ECU")))
	{
		UE_LOG(LogSuperheroFlight, Error, TEXT("SuperheroFlightECU: Failed to initialize UDP connection to %s:%d"), *ECUIPAddress, ECUPort);
		return false;
	}

	// Subscribe to UDP data reception events
	UDPTransport->OnBytesReceived.AddDynamic(this, &USuperheroFlightECUController::OnBytesReceived);

	bECUConnected = true;
	UE_LOG(LogSuperheroFlight, Log, TEXT("SuperheroFlightECU: Connected to %s:%d"), *ECUIPAddress, ECUPort);
	return true;
}

void USuperheroFlightECUController::ShutdownECU()
{
	if (UDPTransport)
	{
		// Unsubscribe from UDP data reception events
		UDPTransport->OnBytesReceived.RemoveDynamic(this, &USuperheroFlightECUController::OnBytesReceived);
		UDPTransport->ShutdownUDPConnection();
	}
	bECUConnected = false;
}

bool USuperheroFlightECUController::IsECUConnected() const
{
	return bECUConnected && UDPTransport && UDPTransport->IsUDPConnected();
}

void USuperheroFlightECUController::SetFrontWinchPosition(float Position)
{
	if (UDPTransport)
	{
		UDPTransport->SendFloat(0, Position);
	}
}

void USuperheroFlightECUController::SetFrontWinchSpeed(float Speed)
{
	if (UDPTransport)
	{
		UDPTransport->SendFloat(1, FMath::Max(0.0f, Speed));
	}
}

void USuperheroFlightECUController::SetRearWinchPosition(float Position)
{
	if (UDPTransport)
	{
		UDPTransport->SendFloat(2, Position);
	}
}

void USuperheroFlightECUController::SetRearWinchSpeed(float Speed)
{
	if (UDPTransport)
	{
		UDPTransport->SendFloat(3, FMath::Max(0.0f, Speed));
	}
}

void USuperheroFlightECUController::SetDualWinchPositions(float FrontPosition, float RearPosition, float Speed)
{
	SetFrontWinchPosition(FrontPosition);
	SetRearWinchPosition(RearPosition);
	SetFrontWinchSpeed(Speed);
	SetRearWinchSpeed(Speed);
}

void USuperheroFlightECUController::SetGameState(ESuperheroFlightGameState GameState)
{
	if (UDPTransport)
	{
		int32 StateValue = static_cast<int32>(GameState);
		UDPTransport->SendInt32(6, StateValue);
	}
}

void USuperheroFlightECUController::SetPlaySessionActive(bool bActive)
{
	if (UDPTransport)
	{
		UDPTransport->SendBool(9, bActive);
	}
}

void USuperheroFlightECUController::EmergencyStop()
{
	if (UDPTransport)
	{
		UDPTransport->SendBool(7, true);
	}
}

void USuperheroFlightECUController::AcknowledgeStandingGroundHeight(float Height)
{
	if (UDPTransport)
	{
		UDPTransport->SendFloat(10, Height);
	}
}

void USuperheroFlightECUController::SetAirHeight(float Height)
{
	if (UDPTransport)
	{
		UDPTransport->SendFloat(11, FMath::Max(0.0f, Height));
	}
}

void USuperheroFlightECUController::SetProneHeight(float Height)
{
	if (UDPTransport)
	{
		UDPTransport->SendFloat(12, FMath::Max(0.0f, Height));
	}
}

void USuperheroFlightECUController::SetPlayerHeightCompensation(float Multiplier)
{
	if (UDPTransport)
	{
		UDPTransport->SendFloat(13, FMath::Clamp(Multiplier, 0.5f, 2.0f));
	}
}

bool USuperheroFlightECUController::GetDualWinchState(FSuperheroFlightDualWinchState& OutWinchState) const
{
	OutWinchState = LastWinchState;
	return (GetWorld()->GetTimeSeconds() - LastWinchStateTime) < ConnectionTimeout;
}

bool USuperheroFlightECUController::GetSystemTelemetry(FSuperheroFlightTelemetry& OutTelemetry) const
{
	OutTelemetry = LastTelemetry;
	return (GetWorld()->GetTimeSeconds() - LastTelemetryTime) < ConnectionTimeout;
}

void USuperheroFlightECUController::OnBytesReceived(int32 Channel, TArray<uint8> Data)
{
	// Process Channel 310 (dual-winch state) and Channel 311 (telemetry)
	if (Channel == 310)
	{
		// Parse FSuperheroFlightDualWinchState struct
		if (Data.Num() >= sizeof(FSuperheroFlightDualWinchState))
		{
			FMemory::Memcpy(&LastWinchState, Data.GetData(), sizeof(FSuperheroFlightDualWinchState));
			LastWinchStateTime = GetWorld()->GetTimeSeconds();
			UE_LOG(LogSuperheroFlight, VeryVerbose, TEXT("SuperheroFlightECU: Received winch state - Front:%.2f Rear:%.2f"), 
				LastWinchState.FrontWinchPosition, LastWinchState.RearWinchPosition);
		}
		else
		{
			UE_LOG(LogSuperheroFlight, Warning, TEXT("SuperheroFlightECU: Invalid winch state packet size (%d bytes, expected %d)"), 
				Data.Num(), sizeof(FSuperheroFlightDualWinchState));
		}
	}
	else if (Channel == 311)
	{
		// Parse FSuperheroFlightTelemetry struct
		if (Data.Num() >= sizeof(FSuperheroFlightTelemetry))
		{
			FMemory::Memcpy(&LastTelemetry, Data.GetData(), sizeof(FSuperheroFlightTelemetry));
			LastTelemetryTime = GetWorld()->GetTimeSeconds();
			UE_LOG(LogSuperheroFlight, VeryVerbose, TEXT("SuperheroFlightECU: Received telemetry - Voltage:%.2fV Current:%.2fA"), 
				LastTelemetry.SystemVoltage, LastTelemetry.SystemCurrent);
		}
		else
		{
			UE_LOG(LogSuperheroFlight, Warning, TEXT("SuperheroFlightECU: Invalid telemetry packet size (%d bytes, expected %d)"), 
				Data.Num(), sizeof(FSuperheroFlightTelemetry));
		}
	}
}

