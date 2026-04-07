// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"
#include "ProLighting.h"

#include "ProLightingTypes.generated.h"

/**
 * DMX Fixture Types
 */
UENUM(BlueprintType)
enum class EHoloCadeDMXFixtureType : uint8
{
	Dimmable     UMETA(DisplayName = "Dimmable (1 ch)"),
	RGB          UMETA(DisplayName = "RGB (3 ch)"),
	RGBW         UMETA(DisplayName = "RGBW (4 ch)"),
	MovingHead   UMETA(DisplayName = "Moving Head (variable)"),
	Custom       UMETA(DisplayName = "Custom (variable)")
};

/**
 * DMX Fixture Definition
 */
USTRUCT(BlueprintType)
struct PROLIGHTING_API FHoloCadeDMXFixture
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DMX Fixture")
	int32 VirtualFixtureID = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DMX Fixture")
	EHoloCadeDMXFixtureType FixtureType = EHoloCadeDMXFixtureType::Dimmable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DMX Fixture", meta = (ClampMin = "1", ClampMax = "512"))
	int32 DMXChannel = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DMX Fixture", meta = (ClampMin = "0", ClampMax = "15"))
	int32 Universe = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DMX Fixture", meta = (ClampMin = "1", ClampMax = "512"))
	int32 ChannelCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DMX Fixture|Custom", meta = (EditCondition = "FixtureType == EHoloCadeDMXFixtureType::Custom || FixtureType == EHoloCadeDMXFixtureType::MovingHead"))
	TArray<int32> CustomChannelMapping;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DMX Fixture|RDM")
	FString RDMUID;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DMX Fixture|RDM")
	bool bRDMCapable = false;
};

/**
 * Discovered Art-Net Node
 */
USTRUCT(BlueprintType)
struct PROLIGHTING_API FHoloCadeArtNetNode
{
	GENERATED_BODY()

	/** Node IP address */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Art-Net Node")
	FString IPAddress;

	/** Node name (from ArtPollReply) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Art-Net Node")
	FString NodeName;

	/** Node type description */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Art-Net Node")
	FString NodeType;

	/** Number of DMX outputs (ports) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Art-Net Node")
	int32 OutputCount = 1;

	/** Universes per output (typical: 1-4) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Art-Net Node")
	int32 UniversesPerOutput = 1;

	/** Last time this node was seen (for offline detection) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Art-Net Node")
	FDateTime LastSeenTimestamp;
};

/**
 * DMX Communication Mode
 */
UENUM(BlueprintType)
enum class EHoloCadeDMXMode : uint8
{
	/** USB-to-DMX interface (e.g., ENTTEC Open DMX USB, DMX USB PRO) */
	USBDMX       UMETA(DisplayName = "USB DMX Interface"),
	
	/** Art-Net protocol over UDP/Ethernet */
	ArtNet       UMETA(DisplayName = "Art-Net (Network)"),
	
	/** sACN protocol over UDP/Ethernet (future) */
	SACN         UMETA(DisplayName = "sACN (Network - Future)")
};

/**
 * ProLighting Controller Configuration
 */
USTRUCT(BlueprintType)
struct PROLIGHTING_API FHoloCadeProLightingConfig
{
	GENERATED_BODY()

	/** Communication mode (USB DMX or Art-Net) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProLighting")
	EHoloCadeDMXMode DMXMode = EHoloCadeDMXMode::USBDMX;

	// ========================================
	// USB DMX Settings
	// ========================================

	/** COM port for USB DMX interface (e.g., "COM3" on Windows) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProLighting|USB DMX", meta = (EditCondition = "DMXMode == EHoloCadeDMXMode::USBDMX"))
	FString COMPort = TEXT("COM3");

	/** Baud rate for USB DMX (typically 57600 for ENTTEC) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProLighting|USB DMX", meta = (EditCondition = "DMXMode == EHoloCadeDMXMode::USBDMX"))
	int32 BaudRate = 57600;

	// ========================================
	// Art-Net Settings
	// ========================================

	/** Art-Net target IP address (broadcast address for all nodes, or specific node IP) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProLighting|Art-Net", meta = (EditCondition = "DMXMode == EHoloCadeDMXMode::ArtNet"))
	FString ArtNetIPAddress = TEXT("255.255.255.255");  // Broadcast to all Art-Net nodes

	/** Art-Net port (default: 6454) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProLighting|Art-Net", meta = (EditCondition = "DMXMode == EHoloCadeDMXMode::ArtNet"))
	int32 ArtNetPort = 6454;

	/** Art-Net Net (0-127, default: 0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProLighting|Art-Net", meta = (EditCondition = "DMXMode == EHoloCadeDMXMode::ArtNet", ClampMin = "0", ClampMax = "127"))
	int32 ArtNetNet = 0;

	/** Art-Net SubNet (0-15, default: 0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProLighting|Art-Net", meta = (EditCondition = "DMXMode == EHoloCadeDMXMode::ArtNet", ClampMin = "0", ClampMax = "15"))
	int32 ArtNetSubNet = 0;

	/** Maximum universe number to support (0-15 per subnet, default: 0 = single universe) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProLighting|Art-Net", meta = (EditCondition = "DMXMode == EHoloCadeDMXMode::ArtNet", ClampMin = "0", ClampMax = "15"))
	int32 MaxUniverse = 0;

	// ========================================
	// RDM Settings
	// ========================================

	/** Enable RDM (Remote Device Management) for fixture discovery and bidirectional sync */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProLighting|RDM")
	bool bEnableRDM = false;

	/** RDM polling interval in seconds (how often to query fixture status) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProLighting|RDM", meta = (EditCondition = "bEnableRDM", ClampMin = "0.1", ClampMax = "10.0"))
	float RDMPollInterval = 0.5f;

	/** RDM discovery timeout in seconds (how long to wait for discovery responses) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProLighting|RDM", meta = (EditCondition = "bEnableRDM", ClampMin = "1.0", ClampMax = "30.0"))
	float RDMDiscoveryTimeout = 5.0f;

	/** If true, only use RDM-capable fixtures (ignore non-RDM fixtures) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProLighting|RDM", meta = (EditCondition = "bEnableRDM"))
	bool bRDMOnlyMode = false;
};

/**
 * Discovered RDM Fixture (from RDM discovery)
 */
USTRUCT(BlueprintType)
struct PROLIGHTING_API FHoloCadeDiscoveredFixture
{
	GENERATED_BODY()

	/** RDM Unique ID (64-bit, formatted as hex string like "0x12345678ABCDEF01") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RDM Fixture")
	FString RDMUID;

	/** Fixture manufacturer ID (from RDM) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RDM Fixture")
	int32 ManufacturerID = 0;

	/** Fixture model ID (from RDM) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RDM Fixture")
	int32 ModelID = 0;

	/** Manufacturer name (from RDM) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RDM Fixture")
	FString ManufacturerName;

	/** Fixture model name (from RDM) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RDM Fixture")
	FString ModelName;

	/** Current DMX address (from RDM) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RDM Fixture")
	int32 DMXAddress = 0;

	/** Universe (0-based, from RDM) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RDM Fixture")
	int32 Universe = 0;

	/** Number of DMX channels this fixture uses (from RDM) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RDM Fixture")
	int32 ChannelCount = 1;

	/** Fixture type (inferred from model or user-specified) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RDM Fixture")
	EHoloCadeDMXFixtureType FixtureType = EHoloCadeDMXFixtureType::Dimmable;

	/** Whether fixture is currently online (last RDM query succeeded) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RDM Fixture")
	bool bIsOnline = true;

	/** Last time this fixture was seen (for offline detection) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RDM Fixture")
	FDateTime LastSeenTimestamp;

	/** Virtual fixture ID (if mapped to UMG widget, -1 if not mapped) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RDM Fixture")
	int32 VirtualFixtureID = -1;
};


