// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "ProLightingController.h"
#include "ProLighting/Public/RDMService.h"
#include "Misc/DateTime.h"

UProLightingController::UProLightingController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true; // Enable tick for fade updates, DMX flushing, and RDM polling
	bIsInitialized = false;
	bIsConnected = false;
	RDMPollTimer = 0.0f;
}

void UProLightingController::BeginPlay()
{
	Super::BeginPlay();

	// Create fixture service (owns registry and fade engine, uses shared buffer)
	FixtureService = MakeUnique<FFixtureService>(UniverseBuffer);
    // Bridge all service events to Blueprint delegates
    BridgeServiceEvents();

	if (!Config.COMPort.IsEmpty() || !Config.ArtNetIPAddress.IsEmpty())
	{
		InitializeDMX(Config);
	}
}

void UProLightingController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Shutdown();
	Super::EndPlay(EndPlayReason);
}

void UProLightingController::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsConnected)
	{
		return;
	}

	// Update fades via service
    if (FixtureService)
    {
        FixtureService->TickFades(DeltaTime, [this](int32 Id, float NewIntensity)
        {
            if (FFixtureService* Svc = FixtureService.Get())
            {
                int32 Univ = Svc->SetIntensityById(Id, NewIntensity);
                if (Univ >= 0)
                {
                    FlushDMXUniverse(Univ);
                }
            }
        });
    }

	// Flush universes (optimize later)
	for (int32 Universe : UniverseBuffer.GetUniverses())
	{
		FlushDMXUniverse(Universe);
	}

    // Tick discovery
    if (ArtNetManager && Config.DMXMode == EHoloCadeDMXMode::ArtNet)
	{
		ArtNetManager->Tick(DeltaTime);
	}

	// RDM polling...
	if (Config.bEnableRDM && bIsConnected && RDMService)
	{
		RDMPollTimer += DeltaTime;
		if (RDMPollTimer >= Config.RDMPollInterval)
		{
			RDMPollTimer = 0.0f;
			PollRDMFixtures(DeltaTime);
			TArray<int32> WentOffline; TArray<FString> Removed;
			// Prune fires OnWentOfflineEvent internally, which is bridged to Blueprint via BridgeServiceEvents()
			RDMService->Prune(Config.RDMPollInterval * 3.0f, Config.RDMPollInterval * 10.0f, WentOffline, Removed, RDMUIDToVirtualFixtureMap);
		}
		RDMService->Tick(DeltaTime);
	}
}

bool UProLightingController::InitializeDMX(const FHoloCadeProLightingConfig& InConfig)
{
	Config = InConfig;

	if (bIsInitialized)
	{
		UE_LOG(LogProLighting, Warning, TEXT("ProLightingController: Already initialized"));
		return false;
	}

	bool bSuccess = false;
	
	// Create transport using polymorphic factory (handles all mode-specific setup)
	FTransportSetupResult SetupResult = IDMXTransport::CreateTransport(Config);
	
	if (!SetupResult.IsValid())
	{
		UE_LOG(LogProLighting, Error, TEXT("ProLightingController: Transport creation failed"));
		return false;
	}
	
	// Get transport pointer BEFORE moving (since moving will invalidate the pointers)
	ActiveTransport = SetupResult.GetTransport();
	
	// Store transports/managers from result (take ownership of raw pointers)
	if (SetupResult.USBDMXTransport)
	{
		USBDMXTransport = TUniquePtr<FUSBDMXTransport>(SetupResult.USBDMXTransport);
	}
	if (SetupResult.ArtNetManager)
	{
		ArtNetManager = TUniquePtr<FArtNetManager>(SetupResult.ArtNetManager);
	}
	
	// Run mode-specific setup callback (Art-Net discovery bridging, RDM init, etc.)
	if (SetupResult.SetupCallback)
	{
		if (!SetupResult.SetupCallback(this))
		{
			UE_LOG(LogProLighting, Error, TEXT("ProLightingController: Transport setup callback failed"));
			return false;
		}
	}
	
	bSuccess = true;
	bIsInitialized = true;
	bIsConnected = true;
	UE_LOG(LogProLighting, Log, TEXT("ProLightingController: Initialized (Mode: %d)"), (uint8)Config.DMXMode);
	
	// Re-bridge events in case services were created/updated during initialization
	BridgeServiceEvents();
	
	return bSuccess;
}

// ========================================
// EVENT BRIDGING
// ========================================

void UProLightingController::BridgeServiceEvents()
{
	// Use polymorphic interface to eliminate if-chain - each service handles its own bridging
	if (FixtureService)
	{
		FixtureService->BridgeEvents(this);
	}
	if (ArtNetManager)
	{
		ArtNetManager->BridgeEvents(this);
	}
	if (RDMService)
	{
		RDMService->BridgeEvents(this);
	}
}

// ========================================
// FIXTURE REGISTRATION
// ========================================

// Registration and control are available via GetFixtureService()

// ========================================
// FIXTURE CONTROL API
// ========================================

// Control methods removed; use FixtureService directly.

bool UProLightingController::IsDMXConnected() const
{
	return bIsConnected;
}

void UProLightingController::Shutdown()
{
	// Shutdown active transport (polymorphic - handles both USB DMX and Art-Net)
	if (ActiveTransport)
	{
		ActiveTransport->Shutdown();
		ActiveTransport = nullptr;
	}
	
	// Clean up transports/managers (ownership is separate from ActiveTransport pointer)
	if (USBDMXTransport)
	{
		USBDMXTransport.Reset();
	}
	if (ArtNetManager)
	{
		ArtNetManager.Reset();
	}

	// Clean up controller state
	bIsInitialized = false;
	bIsConnected = false;
    UniverseBuffer.Reset();
    // FixtureService owns Registry and FadeEngine; dropping the service will clean them up
    // Art-Net nodes are owned by ArtNetManager
    if (RDMService)
    {
        // No explicit reset API; drop the instance
        RDMService.Reset();
    }
	VirtualFixtureToRDMUIDMap.Empty();
	RDMUIDToVirtualFixtureMap.Empty();
	RDMPollTimer = 0.0f;
}

// ========================================
// USB DMX and Art-Net Implementation
// ========================================
// Note: Initialization and shutdown are now handled directly in InitializeDMX() and Shutdown()
// to avoid unnecessary wrapper methods. Transport/manager instances are created and their
// Initialize()/Shutdown() methods are called directly.

// ========================================
// DMX Data Management
// ========================================

void UProLightingController::UpdateDMXChannel(int32 Universe, int32 Channel, uint8 Value)
{
	if (Channel < 1 || Channel > 512)
	{
		return;
	}

    UniverseBuffer.SetChannel(Universe, Channel, Value);
}

uint8 UProLightingController::GetDMXChannel(int32 Universe, int32 Channel) const
{
	if (Channel < 1 || Channel > 512)
	{
		return 0;
	}

    return UniverseBuffer.GetChannel(Universe, Channel);
}

void UProLightingController::InitializeDMXUniverse(int32 Universe)
{
    UniverseBuffer.EnsureUniverse(Universe);
}

void UProLightingController::FlushDMXUniverse(int32 Universe)
{
	const TArray<uint8>* UniverseData = UniverseBuffer.GetUniverse(Universe);
	if (!UniverseData || UniverseData->Num() != 512)
	{
		return;
	}

	// Use polymorphic transport interface
	if (ActiveTransport && ActiveTransport->IsConnected())
	{
		ActiveTransport->SendDMX(Universe, *UniverseData);
	}
}

// Removed: GetFixtureUniverse, UpdateFixtureIntensity, UpdateFixtureColor, UpdateFixtureChannelRaw
// These methods have been removed as part of the refactor. Use FixtureService APIs directly:
// - FixtureService->SetIntensityById()
// - FixtureService->SetColorRGBWById()
// - FixtureService->SetChannelById()
// The service handles universe calculation and buffer updates internally.

// ========================================
// FIXTURE DISCOVERY API
// ========================================

void UProLightingController::DiscoverArtNetNodes()
{
	if (Config.DMXMode != EHoloCadeDMXMode::ArtNet || !ArtNetManager)
	{
		UE_LOG(LogProLighting, Warning, TEXT("ProLightingController: Art-Net discovery unavailable"));
		return;
	}
	ArtNetManager->SendArtPoll();
}

void UProLightingController::DiscoverRDMFixtures()
{
	if (!Config.bEnableRDM)
	{
		UE_LOG(LogProLighting, Warning, TEXT("ProLightingController: RDM is not enabled in configuration"));
		return;
	}

	if (!CheckRDMSupport())
	{
		UE_LOG(LogProLighting, Warning, TEXT("ProLightingController: RDM not supported by current DMX interface"));
		return;
	}

	UE_LOG(LogProLighting, Log, TEXT("ProLightingController: Starting RDM fixture discovery..."));

    // Discover fixtures on each universe
    for (int32 Universe : UniverseBuffer.GetUniverses())
    {
		TArray<FHoloCadeDiscoveredFixture> DiscoveredFixtures;
		
		if (SendRDMDiscoveryPacket(Universe))
		{
			if (ReceiveRDMDiscoveryResponse(Universe, DiscoveredFixtures))
			{
				// Process discovered fixtures
				for (FHoloCadeDiscoveredFixture& Fixture : DiscoveredFixtures)
				{
					// Check if we already know about this fixture (by RDM UID)
                    bool bNew = RDMService ? RDMService->AddOrUpdate(Fixture) : false;
                    if (bNew)
                    {
                        UE_LOG(LogProLighting, Log, TEXT("ProLightingController: Discovered RDM fixture: %s (%s) at DMX %d"), 
                            *Fixture.ModelName, *Fixture.RDMUID, Fixture.DMXAddress);
                    }
				}
			}
		}
	}

    if (RDMService)
    {
        UE_LOG(LogProLighting, Log, TEXT("ProLightingController: RDM discovery complete. Found %d fixtures"), RDMService->GetAll().Num());
    }
}

TArray<FHoloCadeArtNetNode> UProLightingController::GetDiscoveredArtNetNodes() const
{
    if (ArtNetManager)
    {
        TArray<FHoloCadeArtNetNode> Nodes;
        ArtNetManager->GetDiscoveredArtNetNodes().GenerateValueArray(Nodes);
        return Nodes;
    }
    return {};
}

TArray<FHoloCadeDiscoveredFixture> UProLightingController::GetDiscoveredRDMFixtures() const
{
    return RDMService ? RDMService->GetAll() : TArray<FHoloCadeDiscoveredFixture>();
}

int32 UProLightingController::AutoRegisterDiscoveredFixture(const FString& RDMUID)
{
    FHoloCadeDiscoveredFixture Temp;
    bool bFound = RDMService && RDMService->TryGet(RDMUID, Temp);
    if (!bFound)
	{
		UE_LOG(LogProLighting, Warning, TEXT("ProLightingController: RDM fixture %s not found in discovered fixtures"), *RDMUID);
		return -1;
	}

	// Check if already registered
	if (RDMUIDToVirtualFixtureMap.Contains(RDMUID))
	{
		int32 ExistingVirtualID = RDMUIDToVirtualFixtureMap[RDMUID];
		UE_LOG(LogProLighting, Log, TEXT("ProLightingController: RDM fixture %s already registered as virtual fixture %d"), *RDMUID, ExistingVirtualID);
		return ExistingVirtualID;
	}

	// Create fixture definition from discovered fixture
	FHoloCadeDMXFixture Fixture;
	Fixture.VirtualFixtureID = FixtureService ? FixtureService->GetNextVirtualFixtureID() : -1;
	if (Fixture.VirtualFixtureID < 0)
	{
		return -1;
	}
    Fixture.FixtureType = Temp.FixtureType;
    Fixture.DMXChannel = Temp.DMXAddress;
    Fixture.Universe = Temp.Universe;
    Fixture.ChannelCount = Temp.ChannelCount;
	Fixture.RDMUID = RDMUID;
	Fixture.bRDMCapable = true;

	// Register fixture via service
	if (FixtureService && FixtureService->ValidateAndRegister(Fixture))
	{
		// Create mappings
		VirtualFixtureToRDMUIDMap.Add(Fixture.VirtualFixtureID, RDMUID);
		RDMUIDToVirtualFixtureMap.Add(RDMUID, Fixture.VirtualFixtureID);

		// Update discovered fixture with virtual ID
        // Update cached entry
        // (Optional: we could add a SetVirtualID method to RDMService if needed)

		UE_LOG(LogProLighting, Log, TEXT("ProLightingController: Auto-registered RDM fixture %s as virtual fixture %d"), 
			*RDMUID, Fixture.VirtualFixtureID);

		return Fixture.VirtualFixtureID;
	}

	return -1;
}

bool UProLightingController::IsRDMSupported() const
{
	return Config.bEnableRDM && CheckRDMSupport();
}

bool UProLightingController::IsFixtureRDMCapable(int32 VirtualFixtureID) const
{
    if (const FFixtureService* Service = FixtureService.Get())
    {
        return Service->IsFixtureRDMCapable(VirtualFixtureID);
    }
    return false;
}

// ========================================
// Art-Net Node Discovery Implementation
// ========================================

// (ArtPoll discovery moved to FArtNetDiscovery)

// ========================================
// RDM Implementation
// ========================================

bool UProLightingController::CheckRDMSupport() const
{
	// Check if interface supports RDM
	// For USB DMX: ENTTEC DMX USB PRO supports RDM, Open DMX USB does not
	// For Art-Net: RDM runs over DMX cable from Art-Net nodes, so check if we can query DMX line
	
	if (Config.DMXMode == EHoloCadeDMXMode::USBDMX)
	{
		// TODO: Check COM port / interface type
		// For now, assume RDM is supported if bEnableRDM is true and we have a COM port
		return !Config.COMPort.IsEmpty();
	}
	else if (Config.DMXMode == EHoloCadeDMXMode::ArtNet)
	{
		// Art-Net itself doesn't determine RDM support - it's the DMX interface behind the Art-Net node
		// We can't determine this remotely, so assume it's possible if RDM is enabled
		// User must configure correctly
		return true;
	}

	return false;
}

bool UProLightingController::SendRDMDiscoveryPacket(int32 Universe)
{
	// RDM Discovery uses "Mute" command with Discovery Unique Branch (DUB)
	// This is complex - RDM packets are interleaved with DMX data
	// For now, stubbed - requires low-level DMX/RDM packet construction
	
	UE_LOG(LogProLighting, Verbose, TEXT("ProLightingController: RDM discovery packet (stubbed) for universe %d"), Universe);
	
	// TODO: Implement RDM discovery protocol
	// This requires:
	// 1. Constructing RDM packet (start code 0xCC)
	// 2. Sending on DMX line (requires USB DMX interface or Art-Net node with RDM support)
	// 3. Receiving RDM responses (on same DMX line, interleaved with DMX data)
	
	return false;  // Not yet implemented
}

bool UProLightingController::ReceiveRDMDiscoveryResponse(int32 Universe, TArray<FHoloCadeDiscoveredFixture>& OutFixtures)
{
	// TODO: Implement RDM discovery response parsing
	// Requires reading RDM packets from DMX line
	
	return false;  // Not yet implemented
}

bool UProLightingController::SendRDMGetRequest(int32 Universe, const FString& RDMUID, int32 PID, TArray<uint8>& OutResponse)
{
	// RDM GET request for querying fixture parameters
	// PID = Parameter ID (e.g., DMX_START_ADDRESS, DEVICE_INFO, etc.)
	
	UE_LOG(LogProLighting, Verbose, TEXT("ProLightingController: RDM GET request (stubbed) - Universe %d, UID %s, PID %d"), 
		Universe, *RDMUID, PID);
	
	// TODO: Implement RDM GET request
	
	return false;  // Not yet implemented
}

bool UProLightingController::ParseRDMResponse(const TArray<uint8>& ResponseData, FHoloCadeDiscoveredFixture& OutFixture)
{
	// TODO: Parse RDM response packet
	// Extract fixture information from RDM response
	
	return false;  // Not yet implemented
}

void UProLightingController::PollRDMFixtures(float DeltaTime)
{
	if (!Config.bEnableRDM)
	{
		return;
	}

	// Poll each registered RDM-capable fixture
	for (const auto& Pair : VirtualFixtureToRDMUIDMap)
	{
		int32 VirtualFixtureID = Pair.Key;
		FString RDMUID = Pair.Value;

        const FHoloCadeDMXFixture* Fixture = FixtureService ? FixtureService->FindFixture(VirtualFixtureID) : nullptr;
		if (!Fixture)
		{
			continue;
		}

		// Query fixture status via RDM GET request
    int32 Universe = (Config.DMXMode == EHoloCadeDMXMode::USBDMX) ? 0 : Fixture->Universe;
		TArray<uint8> Response;

		// Query DMX_START_ADDRESS to get current fixture state
		// PID 0x00F0 = DMX_START_ADDRESS
		if (SendRDMGetRequest(Universe, RDMUID, 0x00F0, Response))
		{
			FHoloCadeDiscoveredFixture DiscoveredFixture;
			if (ParseRDMResponse(Response, DiscoveredFixture))
			{
				// Update discovered fixture cache via service
				if (RDMService)
				{
					RDMService->AddOrUpdate(DiscoveredFixture);
				}

				// If DMX start address changed, update our registered mapping
				if (Fixture && DiscoveredFixture.DMXAddress != Fixture->DMXChannel)
				{
					UE_LOG(LogProLighting, Log, TEXT("ProLightingController: RDM fixture %s moved from DMX %d to %d"), 
						*RDMUID, Fixture->DMXChannel, DiscoveredFixture.DMXAddress);
					if (FHoloCadeDMXFixture* RegisteredFixture = FixtureService ? FixtureService->FindFixtureMutable(VirtualFixtureID) : nullptr)
					{
						RegisteredFixture->DMXChannel = DiscoveredFixture.DMXAddress;
					}
				}

				// Mark online via service
				if (RDMService)
				{
					RDMService->MarkOnline(RDMUID, VirtualFixtureID);
				}

				// TODO: Query actual DMX channel values if fixture supports it
				// Some RDM fixtures can report current parameter values
				// This would enable true bidirectional sync
			}
		}
		else
		{
			// RDM query failed - fixture might be offline
			if (RDMService)
			{
				RDMService->MarkOffline(RDMUID, VirtualFixtureID);
			}
		}
	}
}

// UpdateFixtureOnlineStatus moved to FFixtureService

void UProLightingController::PruneOfflineFixtures()
{
    // Handled in Tick() via RDMService->Prune
}

// GetNextVirtualFixtureID moved to FFixtureService

