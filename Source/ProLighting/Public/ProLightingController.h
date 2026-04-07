// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProLighting.h"  // For LogProLighting
#include "ProLightingTypes.h"
#include "UniverseBuffer.h"
#include "RDMService.h"
#include "IDMXTransport.h"
#include "USBDMXTransport.h"
#include "ArtNetManager.h"
#include "FixtureService.h"
#include "ProLightingController.generated.h"

// EHoloCadeDMXMode and FHoloCadeProLightingConfig moved to ProLightingTypes.h

/**
 * HoloCade ProLighting Controller
 * 
 * Hardware-agnostic DMX lighting control via USB DMX interfaces or Art-Net.
 * Supports all common fixture types (dimmable, RGB, moving heads, etc.)
 * with a unified API.
 * 
 * Works with:
 * - Simple USB-to-DMX interfaces (ENTTEC, DMXKing, etc.) for small setups
 * - Art-Net networks for scalable, distributed lighting systems
 */
UCLASS(ClassGroup=(HoloCade), meta=(BlueprintSpawnableComponent))
class PROLIGHTING_API UProLightingController : public UActorComponent
{
	GENERATED_BODY()

public:
	UProLightingController(const FObjectInitializer& ObjectInitializer);

	/** Configuration */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProLighting")
	FHoloCadeProLightingConfig Config;

	/**
	 * Initialize DMX connection
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|ProLighting")
	bool InitializeDMX(const FHoloCadeProLightingConfig& InConfig);

    // Fixture registration/control is performed by FixtureService; use accessor below if needed.

	// ========================================
	// FIXTURE DISCOVERY (RDM & Art-Net)
	// ========================================

	/**
	 * Discover Art-Net nodes on the network (sends ArtPoll, receives ArtPollReply)
	 * Only works in Art-Net mode
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|ProLighting|Discovery")
	void DiscoverArtNetNodes();

	/**
	 * Discover RDM-capable fixtures on the DMX line
	 * Works with USB DMX (if RDM-capable) or Art-Net (RDM runs over DMX cable from Art-Net nodes)
	 * Requires RDM-capable DMX interface
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|ProLighting|Discovery")
	void DiscoverRDMFixtures();

	/**
	 * Get all discovered Art-Net nodes
	 * @return Array of discovered nodes
	 */
	UFUNCTION(BlueprintPure, Category = "HoloCade|ProLighting|Discovery")
	TArray<FHoloCadeArtNetNode> GetDiscoveredArtNetNodes() const;

	/**
	 * Get all discovered RDM fixtures
	 * @return Array of discovered fixtures
	 */
	UFUNCTION(BlueprintPure, Category = "HoloCade|ProLighting|Discovery")
	TArray<FHoloCadeDiscoveredFixture> GetDiscoveredRDMFixtures() const;

	/**
	 * Auto-register a discovered RDM fixture
	 * Assigns next available VirtualFixtureID and creates mapping
	 * @param RDMUID - RDM Unique ID of discovered fixture
	 * @return VirtualFixtureID if successful, -1 if failed
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|ProLighting|Discovery")
	int32 AutoRegisterDiscoveredFixture(const FString& RDMUID);

	/**
	 * Check if RDM is supported and available
	 * @return true if DMX interface supports RDM and RDM is enabled
	 */
	UFUNCTION(BlueprintPure, Category = "HoloCade|ProLighting|Discovery")
	bool IsRDMSupported() const;

	/**
	 * Get RDM-capable status for a registered fixture
	 * @param VirtualFixtureID - Virtual fixture ID
	 * @return true if fixture is RDM-capable
	 */
	UFUNCTION(BlueprintPure, Category = "HoloCade|ProLighting|Discovery")
	bool IsFixtureRDMCapable(int32 VirtualFixtureID) const;

	// ========================================
	// FIXTURE CONTROL API
	// ========================================

    // Controller exposes service accessor for direct use where appropriate (C++ only - not exposed to Blueprint)
    FFixtureService* GetFixtureService() const { return FixtureService.Get(); }

	/**
	 * Check if DMX is connected
	 */
	UFUNCTION(BlueprintPure, Category = "HoloCade|ProLighting")
	bool IsDMXConnected() const;

	/**
	 * Shutdown DMX connection
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|ProLighting")
	void Shutdown();

	// ========================================
	// UMG WIDGET DELEGATES (For Command Console)
	// ========================================

	/**
	 * Delegate fired when fixture intensity changes (for bidirectional sync with physical console)
	 * UMG widgets can bind to this to auto-sync with physical console
	 */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFixtureIntensityChanged, int32, VirtualFixtureID, float, Intensity);

	/**
	 * Delegate fired when fixture color changes
	 */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnFixtureColorChanged, int32, VirtualFixtureID, float, Red, float, Green, float, Blue);

	/**
	 * Delegate fired when a fixture goes offline (no longer responding to RDM queries)
	 * UMG widgets should update their status indicator
	 */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFixtureWentOffline, int32, VirtualFixtureID);

	/**
	 * Delegate fired when a fixture comes back online (was offline, now responding)
	 */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFixtureCameOnline, int32, VirtualFixtureID);

	/**
	 * Delegate fired when new fixtures are discovered via RDM
	 */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFixtureDiscovered, FHoloCadeDiscoveredFixture, DiscoveredFixture);

	/**
	 * Delegate fired when Art-Net nodes are discovered
	 */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnArtNetNodeDiscovered, FHoloCadeArtNetNode, Node);

	/** Fixture intensity changed event (bind UMG widgets to this) */
	UPROPERTY(BlueprintAssignable, Category = "HoloCade|ProLighting|Events")
	FOnFixtureIntensityChanged OnFixtureIntensityChanged;

	/** Fixture color changed event (bind UMG widgets to this) */
	UPROPERTY(BlueprintAssignable, Category = "HoloCade|ProLighting|Events")
	FOnFixtureColorChanged OnFixtureColorChanged;

	/** Fixture went offline event (bind UMG widgets to this for status updates) */
	UPROPERTY(BlueprintAssignable, Category = "HoloCade|ProLighting|Events")
	FOnFixtureWentOffline OnFixtureWentOffline;

	/** Fixture came online event (bind UMG widgets to this for status updates) */
	UPROPERTY(BlueprintAssignable, Category = "HoloCade|ProLighting|Events")
	FOnFixtureCameOnline OnFixtureCameOnline;

	/** Fixture discovered event (bind UMG widgets to this to update fixture list) */
	UPROPERTY(BlueprintAssignable, Category = "HoloCade|ProLighting|Events")
	FOnFixtureDiscovered OnFixtureDiscovered;

	/** Art-Net node discovered event */
	UPROPERTY(BlueprintAssignable, Category = "HoloCade|ProLighting|Events")
	FOnArtNetNodeDiscovered OnArtNetNodeDiscovered;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	friend class IDMXTransport; // Allow factory to access private members for setup

    /** DMX universe data (shared between controller for flushing and FixtureService for fixture operations) */
    FUniverseBuffer UniverseBuffer;

    /** RDM service */
    TUniquePtr<FRDMService> RDMService;

	/** Mapping from VirtualFixtureID to RDM UID (for RDM-capable fixtures) */
	TMap<int32, FString> VirtualFixtureToRDMUIDMap;

	/** Reverse mapping: RDM UID to VirtualFixtureID */
	TMap<FString, int32> RDMUIDToVirtualFixtureMap;

    /** Fixture service (owns FixtureRegistry and FadeEngine) */
    TUniquePtr<FFixtureService> FixtureService;

    /** RDM polling timer */
    float RDMPollTimer = 0.0f;

	// ========================================
	// Transport/Manager Instances
	// ========================================

	// Active DMX transport (polymorphic pointer - USB DMX or Art-Net)
	// Note: For USB DMX, this owns the transport. For Art-Net, this points to ArtNetManager.
	IDMXTransport* ActiveTransport = nullptr;

	// USB DMX transport (owned when USB mode is active)
	TUniquePtr<FUSBDMXTransport> USBDMXTransport;

	// Art-Net manager (when Art-Net mode is active - provides discovery in addition to transport)
	TUniquePtr<FArtNetManager> ArtNetManager;

    // Art-Net discovery is handled inside FArtNetManager

	// ========================================
	// Event Bridging
	// ========================================

	/** Bridge all service events to Blueprint delegates */
	void BridgeServiceEvents();

	// ========================================
	// RDM Implementation
	// ========================================

	/** Check if DMX interface supports RDM */
	bool CheckRDMSupport() const;

	/** Send RDM discovery packet */
	bool SendRDMDiscoveryPacket(int32 Universe);

	/** Receive RDM discovery response */
	bool ReceiveRDMDiscoveryResponse(int32 Universe, TArray<FHoloCadeDiscoveredFixture>& OutFixtures);

	/** Send RDM GET request (query fixture parameter) */
	bool SendRDMGetRequest(int32 Universe, const FString& RDMUID, int32 PID, TArray<uint8>& OutResponse);

	/** Parse RDM response packet */
	bool ParseRDMResponse(const TArray<uint8>& ResponseData, FHoloCadeDiscoveredFixture& OutFixture);

	/** Poll RDM fixtures for status updates */
	void PollRDMFixtures(float DeltaTime);

	/** Prune fixtures that haven't been seen (offline detection) */
	void PruneOfflineFixtures();

	// ========================================
	// DMX Data Management
	// ========================================

	/** Update DMX channel value */
	void UpdateDMXChannel(int32 Universe, int32 Channel, uint8 Value);

	/** Get DMX channel value */
	uint8 GetDMXChannel(int32 Universe, int32 Channel) const;

	/** Initialize DMX universe (set all channels to 0) */
	void InitializeDMXUniverse(int32 Universe);

	/** Send updated DMX data for a universe */
	void FlushDMXUniverse(int32 Universe);

    // Fixture-level DMX helpers removed; use FixtureService APIs instead

	bool bIsInitialized = false;
	bool bIsConnected = false;
};

