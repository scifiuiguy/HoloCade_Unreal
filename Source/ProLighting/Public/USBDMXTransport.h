// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "ProLighting.h"
#include "IDMXTransport.h"

/** Stub USB DMX transport (placeholder) */
class PROLIGHTING_API FUSBDMXTransport : public IDMXTransport
{
public:
	FUSBDMXTransport(const FString& InCOMPort, int32 InBaud);
	virtual bool Initialize() override;
	virtual void Shutdown() override;
	virtual bool IsConnected() const override;
	virtual void SendDMX(int32 Universe, const TArray<uint8>& DMXData) override;

private:
	FString COMPort;
	int32 BaudRate = 115200;
	bool bConnected = false;
};


