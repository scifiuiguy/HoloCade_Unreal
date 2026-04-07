// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "ProLighting/Public/IDMXTransport.h"
#include "ProLighting/Public/USBDMXTransport.h"
#include "ProLighting/Public/ArtNetManager.h"
#include "ProLighting/Public/ProLightingController.h"
#include "ProLighting/Public/RDMService.h"

FTransportSetupResult IDMXTransport::CreateTransport(const FHoloCadeProLightingConfig& Config)
{
	FTransportSetupResult Result;
	
	switch (Config.DMXMode)
	{
	case EHoloCadeDMXMode::USBDMX:
	{
		FUSBDMXTransport* USBTransport = new FUSBDMXTransport(Config.COMPort, Config.BaudRate);
		if (!USBTransport->Initialize())
		{
			UE_LOG(LogProLighting, Error, TEXT("IDMXTransport: USB DMX transport initialization failed"));
			delete USBTransport;
			return Result; // Empty result
		}
		Result.USBDMXTransport = USBTransport; // Store raw pointer (ownership transferred to controller)
		Result.Transport = USBTransport; // Set polymorphic pointer
		// USB DMX setup is simple - just store the transport
		Result.SetupCallback = [](UProLightingController* Controller) -> bool
		{
			UE_LOG(LogProLighting, Warning, TEXT("ProLightingController: USB DMX transport initialized (stub - not yet fully implemented)"));
			return true;
		};
		break;
	}
	case EHoloCadeDMXMode::ArtNet:
	{
		FArtNetManager* Manager = new FArtNetManager();
		if (!Manager->Initialize(Config.ArtNetIPAddress, Config.ArtNetPort, Config.ArtNetNet, Config.ArtNetSubNet))
		{
			UE_LOG(LogProLighting, Error, TEXT("IDMXTransport: Art-Net manager configuration failed"));
			delete Manager;
			return Result; // Empty result
		}
		if (!Manager->Initialize()) // IDMXTransport::Initialize()
		{
			UE_LOG(LogProLighting, Error, TEXT("IDMXTransport: Art-Net transport initialization failed"));
			delete Manager;
			return Result; // Empty result
		}
		Result.ArtNetManager = Manager; // Store raw pointer (ownership transferred to controller)
		Result.Transport = Manager; // Set polymorphic pointer
		// Art-Net setup: bridge discovery events and initialize RDM
		// Capture config by value to avoid dangling reference
		FHoloCadeProLightingConfig ConfigCopy = Config;
		Result.SetupCallback = [ConfigCopy](UProLightingController* Controller) -> bool
		{
			if (!Controller || !Controller->ArtNetManager)
			{
				return false;
			}
			
			UE_LOG(LogProLighting, Log, TEXT("ProLightingController: Art-Net initialized (IP: %s:%d, Net: %d, SubNet: %d)"), 
				*ConfigCopy.ArtNetIPAddress, ConfigCopy.ArtNetPort, ConfigCopy.ArtNetNet, ConfigCopy.ArtNetSubNet);

			// Initialize RDM service after Art-Net init
			Controller->RDMService = MakeUnique<FRDMService>();
			Controller->RDMService->Initialize(ConfigCopy.RDMPollInterval);
			
			// Note: Event bridging is handled by Controller->BridgeServiceEvents() which is called after initialization
			if (Controller->FixtureService)
			{
				Controller->FixtureService->SetRDMContext(
					Controller->RDMService.Get(), 
					&Controller->VirtualFixtureToRDMUIDMap, 
					&Controller->RDMUIDToVirtualFixtureMap);
			}
			return true;
		};
		break;
	}
	case EHoloCadeDMXMode::SACN:
		UE_LOG(LogProLighting, Warning, TEXT("IDMXTransport: sACN not yet implemented"));
		return Result; // Empty result
	default:
		return Result; // Empty result
	}
	
	return Result;
}

