// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "ProAudioController.h"
#include "OSCClient.h"
#include "OSCServer.h"
#include "OSCMessage.h"
#include "OSCAddress.h"
#include "OSCTypes.h"

UProAudioController::UProAudioController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true; // Enable tick for OSC receive processing
	bIsInitialized = false;
}

void UProAudioController::BeginPlay()
{
	Super::BeginPlay();

	if (Config.BoardIPAddress.Len() > 0)
	{
		InitializeConsole(Config);
	}
}

void UProAudioController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Shutdown();
	Super::EndPlay(EndPlayReason);
}

bool UProAudioController::InitializeConsole(const FHoloCadeProAudioConfig& InConfig)
{
	Config = InConfig;

	if (OSCClient != nullptr)
	{
		UE_LOG(LogProAudio, Warning, TEXT("ProAudioController: Already initialized"));
		return false;
	}

	// Create OSC client
	OSCClient = NewObject<UOSCClient>(this);
	if (!OSCClient)
	{
		UE_LOG(LogProAudio, Error, TEXT("ProAudioController: Failed to create OSC client"));
		return false;
	}

	// Configure client (SetSendIPAddress takes IP and Port as two parameters)
	if (!OSCClient->SetSendIPAddress(Config.BoardIPAddress, Config.OSCPort))
	{
		UE_LOG(LogProAudio, Error, TEXT("ProAudioController: Failed to set OSC client IP address and port"));
		return false;
	}
	
	// Connect client
	OSCClient->Connect();

	// Create OSC server for bidirectional communication (if enabled)
	if (Config.bEnableReceive)
	{
		OSCServer = NewObject<UOSCServer>(this);
		if (OSCServer)
		{
			// Set server address and port (SetAddress takes IP and Port as two parameters)
			if (!OSCServer->SetAddress(TEXT("0.0.0.0"), Config.ReceivePort)) // Listen on all interfaces
			{
				UE_LOG(LogProAudio, Error, TEXT("ProAudioController: Failed to set OSC server address and port"));
				OSCServer = nullptr;
				return false;
			}
			
			// Bind to OSC message received event
			OSCServer->OnOscMessageReceived.AddDynamic(this, &UProAudioController::OnOSCMessageReceived);
			
			// Start listening
			OSCServer->Listen();
			
			UE_LOG(LogProAudio, Log, TEXT("ProAudioController: OSC Server listening on port %d (bidirectional sync enabled)"), Config.ReceivePort);
		}
	}

	bIsInitialized = true;
	UE_LOG(LogProAudio, Log, TEXT("ProAudioController: Initialized (Console: %d, IP: %s:%d)"), 
		(uint8)Config.ConsoleType, *Config.BoardIPAddress, Config.OSCPort);

	return true;
}

void UProAudioController::SetChannelFader(int32 Channel, float Level)
{
	if (!bIsInitialized || !OSCClient)
	{
		UE_LOG(LogProAudio, Warning, TEXT("ProAudioController: Not initialized"));
		return;
	}

	// Map virtual channel to physical channel
	int32 PhysicalChannel = GetPhysicalChannel(Channel);
	if (PhysicalChannel <= 0)
	{
		// If not explicitly mapped, assume 1:1 (for backward compatibility)
		PhysicalChannel = Channel;
	}

	FString OSCPath = BuildOSCPath(TEXT("fader"), PhysicalChannel);
	float ConsoleLevel = ConvertLevelToConsole(Level);

	TArray<UE::OSC::FOSCData> Args;
	Args.Add(UE::OSC::FOSCData(ConsoleLevel));
	FOSCMessage Message(FOSCAddress(OSCPath), Args);

	OSCClient->SendOSCMessage(Message);
	
	UE_LOG(LogProAudio, Verbose, TEXT("ProAudioController: Set fader - Virtual CH %d -> Physical CH %d = %.3f"), 
		Channel, PhysicalChannel, ConsoleLevel);
}

void UProAudioController::SetChannelMute(int32 Channel, bool bMute)
{
	if (!bIsInitialized || !OSCClient)
	{
		UE_LOG(LogProAudio, Warning, TEXT("ProAudioController: Not initialized"));
		return;
	}

	// Map virtual channel to physical channel
	int32 PhysicalChannel = GetPhysicalChannel(Channel);
	if (PhysicalChannel <= 0)
	{
		// If not explicitly mapped, assume 1:1 (for backward compatibility)
		PhysicalChannel = Channel;
	}

	FString OSCPath = BuildOSCPath(TEXT("mute"), PhysicalChannel);
	int32 MuteValue = bMute ? 1 : 0;

	TArray<UE::OSC::FOSCData> Args;
	Args.Add(UE::OSC::FOSCData(MuteValue));
	FOSCMessage Message(FOSCAddress(OSCPath), Args);

	OSCClient->SendOSCMessage(Message);
	
	UE_LOG(LogProAudio, Verbose, TEXT("ProAudioController: Set mute - Virtual CH %d -> Physical CH %d = %s"), 
		Channel, PhysicalChannel, bMute ? TEXT("Muted") : TEXT("Unmuted"));
}

void UProAudioController::SetChannelBusSend(int32 Channel, int32 Bus, float Level)
{
	if (!bIsInitialized || !OSCClient)
	{
		UE_LOG(LogProAudio, Warning, TEXT("ProAudioController: Not initialized"));
		return;
	}

	// Map virtual channel to physical channel
	int32 PhysicalChannel = GetPhysicalChannel(Channel);
	if (PhysicalChannel <= 0)
	{
		PhysicalChannel = Channel; // Default to 1:1
	}

	FString OSCPath = BuildOSCPath(TEXT("bus"), PhysicalChannel, Bus);
	float ConsoleLevel = ConvertLevelToConsole(Level);

	TArray<UE::OSC::FOSCData> Args;
	Args.Add(UE::OSC::FOSCData(ConsoleLevel));
	FOSCMessage Message(FOSCAddress(OSCPath), Args);

	OSCClient->SendOSCMessage(Message);
	
	UE_LOG(LogProAudio, Verbose, TEXT("ProAudioController: Set bus send - Virtual CH %d -> Physical CH %d, Bus %d = %.3f"), 
		Channel, PhysicalChannel, Bus, ConsoleLevel);
}

void UProAudioController::SetMasterFader(float Level)
{
	if (!bIsInitialized || !OSCClient)
	{
		UE_LOG(LogProAudio, Warning, TEXT("ProAudioController: Not initialized"));
		return;
	}

	FString OSCPath = BuildOSCPath(TEXT("master"), -1);
	float ConsoleLevel = ConvertLevelToConsole(Level);

	TArray<UE::OSC::FOSCData> Args;
	Args.Add(UE::OSC::FOSCData(ConsoleLevel));
	FOSCMessage Message(FOSCAddress(OSCPath), Args);

	OSCClient->SendOSCMessage(Message);
}

bool UProAudioController::IsConsoleConnected() const
{
	return bIsInitialized && OSCClient != nullptr;
}

void UProAudioController::Shutdown()
{
	if (OSCClient)
	{
		OSCClient = nullptr;
	}

	if (OSCServer)
	{
		OSCServer = nullptr;
	}

	bIsInitialized = false;
	UE_LOG(LogProAudio, Log, TEXT("ProAudioController: Shutdown"));
}

FString UProAudioController::BuildOSCPath(const FString& Command, int32 Channel, int32 Bus) const
{
	FString Path;

	// Apply channel offset (for 0-based vs 1-based indexing)
	// Default: offset = 0 (1-based: Channel 1 → /ch/01/)
	// If offset = -1: 0-based (Channel 1 → /ch/00/)
	int32 OSCChannel = Channel + Config.ChannelOffset;
	int32 OSCBus = (Bus > 0) ? (Bus + Config.ChannelOffset) : -1;

	// Behringer X32/M32/Wing OSC paths (1-based by default)
	if (Config.ConsoleType == EHoloCadeProAudioConsole::BehringerX32 ||
		Config.ConsoleType == EHoloCadeProAudioConsole::BehringerM32 ||
		Config.ConsoleType == EHoloCadeProAudioConsole::BehringerWing)
	{
		if (Command == TEXT("fader"))
		{
			Path = FString::Printf(TEXT("/ch/%02d/mix/fader"), OSCChannel);
		}
		else if (Command == TEXT("mute"))
		{
			Path = FString::Printf(TEXT("/ch/%02d/mix/on"), OSCChannel);
		}
		else if (Command == TEXT("bus"))
		{
			Path = FString::Printf(TEXT("/ch/%02d/mix/%02d/level"), OSCChannel, OSCBus);
		}
		else if (Command == TEXT("master"))
		{
			Path = TEXT("/main/st/mix/fader");
		}
	}
	// Yamaha QL/CL/TF OSC paths (1-based by default)
	else if (Config.ConsoleType == EHoloCadeProAudioConsole::YamahaQL ||
			 Config.ConsoleType == EHoloCadeProAudioConsole::YamahaCL ||
			 Config.ConsoleType == EHoloCadeProAudioConsole::YamahaTF)
	{
		if (Command == TEXT("fader"))
		{
			Path = FString::Printf(TEXT("/ch/%02d/level"), OSCChannel);
		}
		else if (Command == TEXT("mute"))
		{
			Path = FString::Printf(TEXT("/ch/%02d/mute"), OSCChannel);
		}
		else if (Command == TEXT("bus"))
		{
			Path = FString::Printf(TEXT("/ch/%02d/mix/%02d/level"), OSCChannel, OSCBus);
		}
		else if (Command == TEXT("master"))
		{
			Path = TEXT("/main/st/level");
		}
	}
	// "Other" - use generic OSC path structure (assumes standard /ch/XX/ format)
	else if (Config.ConsoleType == EHoloCadeProAudioConsole::Other)
	{
		if (Command == TEXT("fader"))
		{
			Path = FString::Printf(TEXT("/ch/%02d/fader"), OSCChannel);
		}
		else if (Command == TEXT("mute"))
		{
			Path = FString::Printf(TEXT("/ch/%02d/mute"), OSCChannel);
		}
		else if (Command == TEXT("bus"))
		{
			Path = FString::Printf(TEXT("/ch/%02d/bus/%02d"), OSCChannel, OSCBus);
		}
		else if (Command == TEXT("master"))
		{
			Path = TEXT("/master/fader");
		}
	}
	// Custom - use user-provided patterns with XX/YY placeholders
	else if (Config.ConsoleType == EHoloCadeProAudioConsole::Custom)
	{
		FString Pattern;
		if (Command == TEXT("fader"))
		{
			Pattern = Config.CustomFaderPattern;
		}
		else if (Command == TEXT("mute"))
		{
			Pattern = Config.CustomMutePattern;
		}
		else if (Command == TEXT("bus"))
		{
			Pattern = Config.CustomBusSendPattern;
		}
		else if (Command == TEXT("master"))
		{
			Pattern = Config.CustomMasterPattern;
			// Master doesn't have channel number, so just return pattern as-is
			return Pattern;
		}
		else
		{
			UE_LOG(LogProAudio, Warning, TEXT("ProAudioController: Unknown command for Custom console: %s"), *Command);
			return TEXT("");
		}

		// Replace XX with zero-padded channel number (e.g., 5 -> 05, 15 -> 15)
		Path = Pattern.Replace(TEXT("XX"), *FString::Printf(TEXT("%02d"), OSCChannel));
		
		// Replace YY with zero-padded bus number if present
		if (OSCBus > 0)
		{
			Path = Path.Replace(TEXT("YY"), *FString::Printf(TEXT("%02d"), OSCBus));
		}
	}
	// Default fallback (generic OSC path structure)
	else
	{
		if (Command == TEXT("fader"))
		{
			Path = FString::Printf(TEXT("/ch/%02d/fader"), OSCChannel);
		}
		else if (Command == TEXT("mute"))
		{
			Path = FString::Printf(TEXT("/ch/%02d/mute"), OSCChannel);
		}
		else if (Command == TEXT("bus"))
		{
			Path = FString::Printf(TEXT("/ch/%02d/bus/%02d"), OSCChannel, OSCBus);
		}
		else if (Command == TEXT("master"))
		{
			Path = TEXT("/master/fader");
		}
	}

	return Path;
}

float UProAudioController::ConvertLevelToConsole(float NormalizedLevel) const
{
	// Most OSC-based consoles expect level in dB or normalized 0-1
	// For now, pass through as-is (0.0 to 1.0)
	// Can be extended to convert to dB if needed: return FMath::Lerp(-100.0f, 0.0f, NormalizedLevel);
	return FMath::Clamp(NormalizedLevel, 0.0f, 1.0f);
}

bool UProAudioController::RegisterChannelForSync(int32 Channel, int32 PhysicalChannel)
{
	if (Channel <= 0)
	{
		UE_LOG(LogProAudio, Error, TEXT("ProAudioController: Invalid virtual channel number %d"), Channel);
		return false;
	}

	if (PhysicalChannel <= 0)
	{
		UE_LOG(LogProAudio, Error, TEXT("ProAudioController: Physical channel number must be specified and greater than 0 (received %d)"), PhysicalChannel);
		return false;
	}

	// Validate physical channel is within console's supported range
	int32 MaxChannels = GetMaxChannelsForConsole();
	if (PhysicalChannel > MaxChannels)
	{
		UE_LOG(LogProAudio, Error, TEXT("ProAudioController: Physical channel %d exceeds maximum for %s (max: %d channels)"), 
			PhysicalChannel, 
			*UEnum::GetValueAsString(Config.ConsoleType), 
			MaxChannels);
		return false;
	}

	// Check if physical channel is already mapped (allow override, but warn)
	for (const auto& Pair : VirtualToPhysicalChannelMap)
	{
		if (Pair.Value == PhysicalChannel && Pair.Key != Channel)
		{
			UE_LOG(LogProAudio, Warning, TEXT("ProAudioController: Physical channel %d already mapped to virtual channel %d. Virtual channel %d will override."), 
				PhysicalChannel, Pair.Key, Channel);
			// Will overwrite below
			break;
		}
	}

	// Store mapping: virtual channel -> physical channel
	VirtualToPhysicalChannelMap.Add(Channel, PhysicalChannel);
	
	// Also track that this virtual channel is registered (for filtering)
	RegisteredChannelsForSync.Add(Channel);

	UE_LOG(LogProAudio, Log, TEXT("ProAudioController: Registered virtual channel %d -> physical channel %d for bidirectional sync"), 
		Channel, PhysicalChannel);
	
	return true;
}

int32 UProAudioController::GetMaxChannelsForConsole() const
{
	switch (Config.ConsoleType)
	{
	case EHoloCadeProAudioConsole::BehringerX32:
	case EHoloCadeProAudioConsole::BehringerM32:
		return 32;  // X32/M32 have 32 input channels
	
	case EHoloCadeProAudioConsole::BehringerWing:
		return 48;  // Wing has 48 input channels
	
	case EHoloCadeProAudioConsole::YamahaQL:
		// QL series: QL1 = 16, QL5 = 32, QL5 = 64 (depends on model, but most common is 32)
		return 64;  // Use max to allow for larger models
	
	case EHoloCadeProAudioConsole::YamahaCL:
		return 64;  // CL series can have up to 64 channels
	
	case EHoloCadeProAudioConsole::YamahaTF:
		// TF series: TF1 = 16, TF3 = 32, TF5 = 64
		return 64;  // Use max to allow for larger models
	
	case EHoloCadeProAudioConsole::YamahaDM7:
		return 96;  // DM7 can have up to 96 channels
	
	case EHoloCadeProAudioConsole::AllenHeathSQ:
		// SQ series: SQ5 = 32, SQ6 = 48, SQ7 = 64
		return 64;
	
	case EHoloCadeProAudioConsole::AllenHeathDLive:
		return 128;  // dLive can have up to 128 channels
	
	case EHoloCadeProAudioConsole::SoundcraftSi:
		// Si Expression/Impact: typically 32-64 channels
		return 64;
	
	case EHoloCadeProAudioConsole::PresonusStudioLive:
		// StudioLive Series III: typically 32 channels
		return 32;
	
	case EHoloCadeProAudioConsole::Other:
		// "Other" option - assume 64 channels, no validation
		// User is responsible for ensuring channel numbers are correct
		return 64;
	
	case EHoloCadeProAudioConsole::Custom:
	default:
		// Unknown/Custom console - use conservative default
		return 64;
	}
}

void UProAudioController::UnregisterChannelForSync(int32 Channel)
{
	RegisteredChannelsForSync.Remove(Channel);
	VirtualToPhysicalChannelMap.Remove(Channel);
	UE_LOG(LogProAudio, Log, TEXT("ProAudioController: Unregistered virtual channel %d from sync"), Channel);
}

int32 UProAudioController::GetPhysicalChannel(int32 VirtualChannel) const
{
	const int32* PhysicalChannelPtr = VirtualToPhysicalChannelMap.Find(VirtualChannel);
	return PhysicalChannelPtr ? *PhysicalChannelPtr : -1;
}

int32 UProAudioController::FindFirstAvailablePhysicalChannel() const
{
	// Collect all currently mapped physical channels
	TSet<int32> UsedPhysicalChannels;
	for (const auto& Pair : VirtualToPhysicalChannelMap)
	{
		UsedPhysicalChannels.Add(Pair.Value);
	}

	// Find first available physical channel starting from 1
	// Most boards support at least 32 channels, but we'll check up to 64
	const int32 MaxChannels = 64;
	for (int32 PhysicalCh = 1; PhysicalCh <= MaxChannels; ++PhysicalCh)
	{
		if (!UsedPhysicalChannels.Contains(PhysicalCh))
		{
			return PhysicalCh;
		}
	}

	// All channels 1-64 are taken - return -1 to indicate no available channels
	return -1;
}

bool UProAudioController::IsBidirectionalSyncEnabled() const
{
	return bIsInitialized && Config.bEnableReceive && OSCServer != nullptr;
}

void UProAudioController::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// OSC messages are handled automatically via event bindings (OnOSCFaderReceived, etc.)
	// Unreal's OSC plugin routes incoming messages to bound UFUNCTIONs
	// No polling needed - when physical board sends OSC update, handler fires -> delegate broadcasts -> UMG widgets update
}

int32 UProAudioController::ExtractChannelFromOSCAddress(const FString& OSCAddress) const
{
	// Parse channel number from various OSC address formats:
	// /ch/01/mix/fader -> 1 (if offset = 0, 1-based)
	// /ch/00/mix/fader -> 1 (if offset = -1, 0-based)
	// /mix/chan/5/fader -> 5
	
	FString LowerAddress = OSCAddress.ToLower();
	int32 OSCChannelNumber = -1;
	
	// Behringer X32/M32 format: /ch/XX/mix/...
	if (LowerAddress.Contains(TEXT("/ch/")))
	{
		int32 ChPos = LowerAddress.Find(TEXT("/ch/"));
		if (ChPos != INDEX_NONE)
		{
			FString AfterCh = LowerAddress.Mid(ChPos + 4); // After "/ch/"
			int32 NextSlash = AfterCh.Find(TEXT("/"));
			if (NextSlash != INDEX_NONE)
			{
				FString ChannelStr = AfterCh.Left(NextSlash);
				OSCChannelNumber = FCString::Atoi(*ChannelStr);
			}
		}
	}
	// Yamaha format: /mix/chan/XX/...
	else if (LowerAddress.Contains(TEXT("/mix/chan/")))
	{
		int32 ChanPos = LowerAddress.Find(TEXT("/mix/chan/"));
		if (ChanPos != INDEX_NONE)
		{
			FString AfterChan = LowerAddress.Mid(ChanPos + 10); // After "/mix/chan/"
			int32 NextSlash = AfterChan.Find(TEXT("/"));
			if (NextSlash != INDEX_NONE)
			{
				FString ChannelStr = AfterChan.Left(NextSlash);
				OSCChannelNumber = FCString::Atoi(*ChannelStr);
			}
		}
	}
	
	if (OSCChannelNumber < 0)
	{
		return -1; // Channel not found
	}
	
	// Convert from OSC channel number to virtual channel number (apply reverse offset)
	// If offset = 0 (1-based): OSC 1 → Virtual 1
	// If offset = -1 (0-based): OSC 0 → Virtual 1 (0 + 1 = 1)
	int32 VirtualChannel = OSCChannelNumber - Config.ChannelOffset;
	return VirtualChannel;
}

int32 UProAudioController::ExtractBusFromOSCAddress(const FString& OSCAddress) const
{
	// Parse bus number from OSC address formats:
	// /ch/01/mix/02/level -> bus 2
	// /ch/16/aux/03/level -> bus 3
	
	FString LowerAddress = OSCAddress.ToLower();
	
	// Look for pattern: .../mix/XX/level or .../aux/XX/level
	TArray<FString> Patterns = { TEXT("/mix/"), TEXT("/aux/") };
	
	for (const FString& Pattern : Patterns)
	{
		int32 PatternPos = LowerAddress.Find(Pattern);
		if (PatternPos != INDEX_NONE)
		{
			int32 AfterPattern = PatternPos + Pattern.Len();
			FString AfterPatternStr = LowerAddress.Mid(AfterPattern);
			int32 NextSlash = AfterPatternStr.Find(TEXT("/"));
			if (NextSlash != INDEX_NONE)
			{
				FString BusStr = AfterPatternStr.Left(NextSlash);
				return FCString::Atoi(*BusStr);
			}
		}
	}
	
	return -1; // Bus not found
}

void UProAudioController::OnOSCMessageReceived(const FOSCMessage& Message, const FString& IPAddress, int32 Port)
{
	// Get address from message
	const FOSCAddress& Address = Message.GetAddress();
	FString AddressStr = Address.GetFullPath();
	FString LowerAddress = AddressStr.ToLower();
	
	// Route based on address pattern
	if (LowerAddress.Contains(TEXT("/fader")) || LowerAddress.Contains(TEXT("/level")))
	{
		if (LowerAddress.Contains(TEXT("/master")) || LowerAddress.Contains(TEXT("/main")))
		{
			OnOSCMasterFaderReceived(Address, Message);
		}
		else
		{
			OnOSCFaderReceived(Address, Message);
		}
	}
	else if (LowerAddress.Contains(TEXT("/mute")) || LowerAddress.Contains(TEXT("/on")))
	{
		OnOSCMuteReceived(Address, Message);
	}
	else if (LowerAddress.Contains(TEXT("/bus")) || LowerAddress.Contains(TEXT("/aux")))
	{
		OnOSCBusSendReceived(Address, Message);
	}
}

void UProAudioController::OnOSCFaderReceived(const FOSCAddress& AddressPattern, const FOSCMessage& Message)
{
	FString Address = AddressPattern.GetFullPath();
	int32 PhysicalChannel = ExtractChannelFromOSCAddress(Address);
	
	if (PhysicalChannel <= 0)
	{
		return; // Invalid channel number extracted from OSC address
	}

	// Find which virtual channel(s) map to this physical channel
	// (Reverse lookup: physical -> virtual)
	TArray<int32> MatchingVirtualChannels;
	for (const auto& Pair : VirtualToPhysicalChannelMap)
	{
		if (Pair.Value == PhysicalChannel)
		{
			MatchingVirtualChannels.Add(Pair.Key);
		}
	}

	if (MatchingVirtualChannels.Num() == 0)
	{
		return; // No virtual channels registered for this physical channel
	}
	
	// Extract float value from OSC message
	float FaderValue = 0.0f;
	const TArray<UE::OSC::FOSCData>& Args = Message.GetArgumentsChecked();
	if (Args.Num() > 0)
	{
		const UE::OSC::FOSCData& FirstArg = Args[0];
		if (FirstArg.IsFloat())
		{
			FaderValue = FirstArg.GetFloat();
		}
		else if (FirstArg.IsInt32())
		{
			FaderValue = static_cast<float>(FirstArg.GetInt32());
		}
	}
	
	// Convert from console-specific format to normalized 0-1
	float NormalizedValue = FMath::Clamp(FaderValue, 0.0f, 1.0f);
	
	// Fire delegate for each virtual channel that maps to this physical channel
	// (Supports multiple UMG widgets mapping to same hardware channel, if needed)
	for (int32 VirtualChannel : MatchingVirtualChannels)
	{
		if (RegisteredChannelsForSync.Contains(VirtualChannel))
		{
			OnChannelFaderChanged.Broadcast(VirtualChannel, NormalizedValue);
			UE_LOG(LogProAudio, Verbose, TEXT("ProAudioController: Received fader update - Physical CH %d -> Virtual CH %d = %.3f"), 
				PhysicalChannel, VirtualChannel, NormalizedValue);
		}
	}
}

void UProAudioController::OnOSCMuteReceived(const FOSCAddress& AddressPattern, const FOSCMessage& Message)
{
	FString Address = AddressPattern.GetFullPath();
	int32 PhysicalChannel = ExtractChannelFromOSCAddress(Address);
	
	if (PhysicalChannel <= 0)
	{
		return;
	}

	// Find which virtual channel(s) map to this physical channel
	TArray<int32> MatchingVirtualChannels;
	for (const auto& Pair : VirtualToPhysicalChannelMap)
	{
		if (Pair.Value == PhysicalChannel)
		{
			MatchingVirtualChannels.Add(Pair.Key);
		}
	}

	if (MatchingVirtualChannels.Num() == 0)
	{
		return;
	}
	
	// Extract mute state (typically 0 = unmuted, 1 = muted)
	bool bMuted = false;
	const TArray<UE::OSC::FOSCData>& Args = Message.GetArgumentsChecked();
	if (Args.Num() > 0)
	{
		const UE::OSC::FOSCData& FirstArg = Args[0];
		if (FirstArg.IsInt32())
		{
			bMuted = (FirstArg.GetInt32() != 0);
		}
		else if (FirstArg.IsFloat())
		{
			bMuted = (FirstArg.GetFloat() != 0.0f);
		}
		else if (FirstArg.IsBool())
		{
			bMuted = FirstArg.GetBool();
		}
	}
	
	// Fire delegate for each matching virtual channel
	for (int32 VirtualChannel : MatchingVirtualChannels)
	{
		if (RegisteredChannelsForSync.Contains(VirtualChannel))
		{
			OnChannelMuteChanged.Broadcast(VirtualChannel, bMuted);
			UE_LOG(LogProAudio, Verbose, TEXT("ProAudioController: Received mute update - Physical CH %d -> Virtual CH %d = %s"), 
				PhysicalChannel, VirtualChannel, bMuted ? TEXT("Muted") : TEXT("Unmuted"));
		}
	}
}

void UProAudioController::OnOSCMasterFaderReceived(const FOSCAddress& AddressPattern, const FOSCMessage& Message)
{
	// Master fader doesn't have a channel number
	float FaderValue = 0.0f;
	const TArray<UE::OSC::FOSCData>& Args = Message.GetArgumentsChecked();
	if (Args.Num() > 0)
	{
		const UE::OSC::FOSCData& FirstArg = Args[0];
		if (FirstArg.IsFloat())
		{
			FaderValue = FirstArg.GetFloat();
		}
		else if (FirstArg.IsInt32())
		{
			FaderValue = static_cast<float>(FirstArg.GetInt32());
		}
	}
	
	float NormalizedValue = FMath::Clamp(FaderValue, 0.0f, 1.0f);
	OnMasterFaderChanged.Broadcast(NormalizedValue);
	
	UE_LOG(LogProAudio, Verbose, TEXT("ProAudioController: Received master fader update = %.3f"), NormalizedValue);
}

void UProAudioController::OnOSCBusSendReceived(const FOSCAddress& AddressPattern, const FOSCMessage& Message)
{
	FString Address = AddressPattern.GetFullPath();
	int32 Channel = ExtractChannelFromOSCAddress(Address);
	int32 Bus = ExtractBusFromOSCAddress(Address);
	
	if (Channel <= 0 || Bus <= 0 || !RegisteredChannelsForSync.Contains(Channel))
	{
		return;
	}
	
	// Bus sends don't have their own delegate currently, but could be added if needed
	// For now, we just log it
	float BusLevel = 0.0f;
	const TArray<UE::OSC::FOSCData>& Args = Message.GetArgumentsChecked();
	if (Args.Num() > 0)
	{
		const UE::OSC::FOSCData& FirstArg = Args[0];
		if (FirstArg.IsFloat())
		{
			BusLevel = FirstArg.GetFloat();
		}
		else if (FirstArg.IsInt32())
		{
			BusLevel = static_cast<float>(FirstArg.GetInt32());
		}
	}
	
	UE_LOG(LogProAudio, Verbose, TEXT("ProAudioController: Received bus send update - Channel %d Bus %d = %.3f"), Channel, Bus, BusLevel);
}

