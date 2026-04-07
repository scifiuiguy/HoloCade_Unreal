// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "GoKart/GoKartECUController.h"
#include "HoloCadeExperiences.h"
#include "Networking/HoloCadeUDPTransport.h"

UGoKartECUController::UGoKartECUController()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f; // Tick every 100ms for UDP processing
	bECUConnected = false;
	ECUPort = 8888;
	ConnectionTimeout = 2.0f;
}

UGoKartECUController::~UGoKartECUController()
{
	ShutdownECU();
}

void UGoKartECUController::BeginPlay()
{
	Super::BeginPlay();
	// NOOP: ECU initialization should be called explicitly by experience
}

void UGoKartECUController::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// NOOP: UDP data processing will be implemented
	// Check for connection timeout
	if (bECUConnected)
	{
		float CurrentTime = GetWorld()->GetTimeSeconds();
		if (CurrentTime - LastButtonEventTime > ConnectionTimeout &&
			CurrentTime - LastThrottleFeedbackTime > ConnectionTimeout)
		{
			// Connection lost
			bECUConnected = false;
			UE_LOG(LogGoKart, Warning, TEXT("GoKartECU: Connection timeout"));
		}
	}
}

bool UGoKartECUController::InitializeECU(const FString& InECUIPAddress, int32 InECUPort)
{
	ECUIPAddress = InECUIPAddress;
	ECUPort = InECUPort;

	if (!UDPTransport)
	{
		UDPTransport = NewObject<UHoloCadeUDPTransport>(this);
	}

	if (!UDPTransport->InitializeUDPConnection(ECUIPAddress, ECUPort, TEXT("GoKart_ECU")))
	{
		UE_LOG(LogGoKart, Error, TEXT("GoKartECU: Failed to initialize UDP connection to %s:%d"), *ECUIPAddress, ECUPort);
		return false;
	}

	bECUConnected = true;
	UE_LOG(LogGoKart, Log, TEXT("GoKartECU: Connected to %s:%d"), *ECUIPAddress, ECUPort);
	return true;
}

void UGoKartECUController::ShutdownECU()
{
	if (UDPTransport)
	{
		UDPTransport->ShutdownUDPConnection();
	}
	bECUConnected = false;
}

bool UGoKartECUController::IsECUConnected() const
{
	return bECUConnected && UDPTransport && UDPTransport->IsUDPConnected();
}

void UGoKartECUController::SetThrottleMultiplier(float Multiplier)
{
	// NOOP: Will send throttle multiplier to ECU via Channel 0
	if (UDPTransport)
	{
		UDPTransport->SendFloat(0, FMath::Clamp(Multiplier, 0.0f, 2.0f));
	}
}

void UGoKartECUController::SendThrottleState(const FGoKartThrottleState& ThrottleState)
{
	// NOOP: Will send complete throttle state struct to ECU via Channel 100
	if (UDPTransport)
	{
		// Send as struct packet
		TArray<uint8> StructData;
		// Serialize struct to bytes (implementation needed)
		UDPTransport->SendStruct(100, StructData);
	}
}

void UGoKartECUController::SetPlaySessionActive(bool bActive)
{
	// NOOP: Will send play session state to ECU via Channel 9
	if (UDPTransport)
	{
		UDPTransport->SendBool(9, bActive);
	}
}

void UGoKartECUController::EmergencyStop()
{
	// NOOP: Will send emergency stop to ECU via Channel 7
	if (UDPTransport)
	{
		UDPTransport->SendBool(7, true);
	}
}

bool UGoKartECUController::GetButtonEvents(FGoKartButtonEvents& OutButtonEvents) const
{
	// NOOP: Will receive button events from ECU via Channel 310
	if (!UDPTransport)
	{
		return false;
	}

	// Get struct data from Channel 310
	TArray<uint8> ReceivedBytes = UDPTransport->GetReceivedBytes(310);
	if (ReceivedBytes.Num() >= sizeof(FGoKartButtonEvents))
	{
		FMemory::Memcpy(&OutButtonEvents, ReceivedBytes.GetData(), sizeof(FGoKartButtonEvents));
		return true;
	}
	return false;
}

bool UGoKartECUController::GetThrottleStateFeedback(FGoKartThrottleState& OutThrottleState) const
{
	// NOOP: Will receive throttle state feedback from ECU via Channel 311
	if (!UDPTransport)
	{
		return false;
	}

	// Get struct data from Channel 311
	TArray<uint8> ReceivedBytes = UDPTransport->GetReceivedBytes(311);
	if (ReceivedBytes.Num() >= sizeof(FGoKartThrottleState))
	{
		FMemory::Memcpy(&OutThrottleState, ReceivedBytes.GetData(), sizeof(FGoKartThrottleState));
		return true;
	}
	return false;
}

void UGoKartECUController::ProcessReceivedData(const TArray<uint8>& Data)
{
	// NOOP: Will process incoming UDP data and route to appropriate channels
}

