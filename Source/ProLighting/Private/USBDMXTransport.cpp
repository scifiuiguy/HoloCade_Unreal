// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "ProLighting/Public/USBDMXTransport.h"
#include "ProLighting/Public/ProLighting.h"

FUSBDMXTransport::FUSBDMXTransport(const FString& InCOMPort, int32 InBaud)
	: COMPort(InCOMPort)
	, BaudRate(InBaud)
{
}

bool FUSBDMXTransport::Initialize()
{
	UE_LOG(LogProLighting, Warning, TEXT("USBDMXTransport: Initialize called (stub). USB DMX not implemented yet. COM=%s, Baud=%d"), *COMPort, BaudRate);
	bConnected = false; // Stub remains disconnected
	return bConnected;
}

void FUSBDMXTransport::Shutdown()
{
	UE_LOG(LogProLighting, Log, TEXT("USBDMXTransport: Shutdown (stub)"));
	bConnected = false;
}

bool FUSBDMXTransport::IsConnected() const
{
	return bConnected;
}

void FUSBDMXTransport::SendDMX(int32 /*Universe*/, const TArray<uint8>& /*DMXData*/)
{
	UE_LOG(LogProLighting, Warning, TEXT("USBDMXTransport: SendDMX called (stub). USB DMX not implemented yet."));
}




