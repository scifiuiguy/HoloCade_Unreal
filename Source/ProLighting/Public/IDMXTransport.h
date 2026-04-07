// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "ProLighting.h"
#include "ProLightingTypes.h"

// Forward declarations
class FUSBDMXTransport;
class FArtNetManager;
class FRDMService;
class FFixtureService;
class UProLightingController;
class IDMXTransport; // Forward declare interface

struct FTransportSetupResult; // Forward declare struct

/**
 * IDMXTransport - Interface for DMX transport implementations
 * 
 * Provides a polymorphic interface for different DMX transport methods:
 * - USB DMX: Direct serial connection to USB-to-DMX interface
 * - Art-Net: Network-based DMX over UDP
 * - sACN (future): Alternative network protocol
 */
class PROLIGHTING_API IDMXTransport
{
public:
	virtual ~IDMXTransport() = default;
	virtual bool Initialize() = 0;
	virtual void Shutdown() = 0;
	virtual bool IsConnected() const = 0;
	virtual void SendDMX(int32 Universe, const TArray<uint8>& DMXData) = 0;

	/**
	 * Factory method to create the appropriate transport based on configuration
	 * Handles all mode-specific setup internally
	 * @param Config - ProLighting configuration containing mode and transport-specific settings
	 * @return Transport setup result with transport and setup callback, or empty result if creation failed
	 */
	static FTransportSetupResult CreateTransport(const FHoloCadeProLightingConfig& Config);
};

/**
 * Transport setup result - contains transport and any additional setup needed
 * Uses raw pointers to avoid incomplete type issues - ownership transferred to controller
 */
struct PROLIGHTING_API FTransportSetupResult
{
	FArtNetManager* ArtNetManager = nullptr; // Only set for Art-Net mode (ownership transferred to controller)
	FUSBDMXTransport* USBDMXTransport = nullptr; // Only set for USB DMX mode (ownership transferred to controller)
	
	// Transport pointer (polymorphic) - set by factory after creation
	IDMXTransport* Transport = nullptr;
	
	// Setup callback - called after transport creation to complete mode-specific initialization
	// Returns true if setup succeeded
	TFunction<bool(UProLightingController*)> SetupCallback;
	
	// Get the transport interface pointer (polymorphic)
	IDMXTransport* GetTransport() const { return Transport; }
	
	// Check if result is valid (has a transport)
	bool IsValid() const { return Transport != nullptr; }
};



