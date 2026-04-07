// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "OSCClient.h"  // Unreal's built-in OSC plugin
#include "OSCServer.h"  // Unreal's built-in OSC plugin
#include "OSCMessage.h"  // Unreal's built-in OSC plugin
#include "ProAudio.h"  // For LogProAudio
#include "ProAudioController.generated.h"

/**
 * Pro Audio Console Types
 */
UENUM(BlueprintType)
enum class EHoloCadeProAudioConsole : uint8
{
	BehringerX32       UMETA(DisplayName = "Behringer X32"),
	BehringerM32       UMETA(DisplayName = "Behringer M32"),
	BehringerWing       UMETA(DisplayName = "Behringer Wing"),
	YamahaQL           UMETA(DisplayName = "Yamaha QL"),
	YamahaCL           UMETA(DisplayName = "Yamaha CL"),
	YamahaTF           UMETA(DisplayName = "Yamaha TF"),
	YamahaDM7          UMETA(DisplayName = "Yamaha DM7"),
	AllenHeathSQ       UMETA(DisplayName = "Allen & Heath SQ"),
	AllenHeathDLive     UMETA(DisplayName = "Allen & Heath dLive"),
	SoundcraftSi        UMETA(DisplayName = "Soundcraft Si"),
	PresonusStudioLive  UMETA(DisplayName = "PreSonus StudioLive"),
	Other               UMETA(DisplayName = "Other (64 channels, no validation)"),
	Custom              UMETA(DisplayName = "Custom (OSC paths)")
};

/**
 * ProAudio Controller Configuration
 */
USTRUCT(BlueprintType)
struct PROAUDIO_API FHoloCadeProAudioConfig
{
	GENERATED_BODY()

	/** Console manufacturer/model */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProAudio")
	EHoloCadeProAudioConsole ConsoleType = EHoloCadeProAudioConsole::BehringerX32;

	/** Sound board IP address */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProAudio")
	FString BoardIPAddress = TEXT("192.168.1.100");

	/** OSC port (default: 10023 for X32, varies by manufacturer) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProAudio")
	int32 OSCPort = 10023;

	/** Enable receive mode (listen for OSC messages from board) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProAudio")
	bool bEnableReceive = false;

	/** OSC receive port (for bidirectional communication) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProAudio", meta = (EditCondition = "bEnableReceive"))
	int32 ReceivePort = 8000;

	/** 
	 * Channel number offset for OSC addressing
	 * Most consoles use 1-based indexing (Channel 1 = /ch/01/), so offset = 0 (default)
	 * Some consoles use 0-based indexing (Channel 1 = /ch/00/), so offset = -1
	 * This is applied when building OSC paths: OSCChannelNumber = VirtualChannelNumber + Offset
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProAudio", AdvancedDisplay)
	int32 ChannelOffset = 0;

	/** 
	 * Custom OSC path patterns (only used when ConsoleType = Custom)
	 * Use XX as placeholder for channel number (will be replaced with zero-padded channel number)
	 * Example: "/ch/XX/fader" becomes "/ch/05/fader" for channel 5
	 * Example: "/mix/input/XX/level" becomes "/mix/input/05/level" for channel 5
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProAudio|Custom", meta = (EditCondition = "ConsoleType == EHoloCadeProAudioConsole::Custom", EditConditionHides))
	FString CustomFaderPattern = TEXT("/ch/XX/fader");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProAudio|Custom", meta = (EditCondition = "ConsoleType == EHoloCadeProAudioConsole::Custom", EditConditionHides))
	FString CustomMutePattern = TEXT("/ch/XX/mute");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProAudio|Custom", meta = (EditCondition = "ConsoleType == EHoloCadeProAudioConsole::Custom", EditConditionHides))
	FString CustomBusSendPattern = TEXT("/ch/XX/bus/YY/level");  // YY will be replaced with bus number

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProAudio|Custom", meta = (EditCondition = "ConsoleType == EHoloCadeProAudioConsole::Custom", EditConditionHides))
	FString CustomMasterPattern = TEXT("/master/fader");
};

/**
 * HoloCade ProAudio Controller
 * 
 * Hardware-agnostic professional audio console control via OSC.
 * Supports all major manufacturers with a unified API.
 * 
 * Uses Unreal Engine's built-in OSC plugin (no external dependencies).
 */
UCLASS(ClassGroup=(HoloCade), meta=(BlueprintSpawnableComponent))
class PROAUDIO_API UProAudioController : public UActorComponent
{
	GENERATED_BODY()

public:
	UProAudioController(const FObjectInitializer& ObjectInitializer);

	/** Configuration */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProAudio")
	FHoloCadeProAudioConfig Config;

	/**
	 * Initialize connection to pro audio console
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|ProAudio")
	bool InitializeConsole(const FHoloCadeProAudioConfig& InConfig);

	/**
	 * Set channel fader level (0.0 to 1.0, where 1.0 = 0dB)
	 * @param Channel - Channel number (1-based: 1, 2, 3, etc.)
	 * @param Level - Fader level (0.0 = -inf, 1.0 = 0dB)
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|ProAudio")
	void SetChannelFader(int32 Channel, float Level);

	/**
	 * Mute/unmute a channel
	 * @param Channel - Channel number (1-based)
	 * @param bMute - true to mute, false to unmute
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|ProAudio")
	void SetChannelMute(int32 Channel, bool bMute);

	/**
	 * Set bus send level (e.g., reverb send, monitor send)
	 * @param Channel - Source channel number (1-based)
	 * @param Bus - Bus number (1-based)
	 * @param Level - Send level (0.0 to 1.0)
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|ProAudio")
	void SetChannelBusSend(int32 Channel, int32 Bus, float Level);

	/**
	 * Set master fader level
	 * @param Level - Fader level (0.0 to 1.0)
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|ProAudio")
	void SetMasterFader(float Level);

	/**
	 * Check if console is connected
	 */
	UFUNCTION(BlueprintPure, Category = "HoloCade|ProAudio")
	bool IsConsoleConnected() const;

	/**
	 * Shutdown connection
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|ProAudio")
	void Shutdown();

	// ========================================
	// UMG WIDGET AUTO-MAPPING (For Command Console)
	// ========================================

	/**
	 * Delegate fired when a channel fader value changes from the physical board
	 * UMG widgets can bind to this to auto-sync with physical console
	 * @param Channel - Channel number (1-based)
	 * @param Level - New fader level (0.0 to 1.0)
	 */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChannelFaderChanged, int32, Channel, float, Level);

	/**
	 * Delegate fired when a channel mute state changes from the physical board
	 * @param Channel - Channel number (1-based)
	 * @param bMute - New mute state (true = muted)
	 */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChannelMuteChanged, int32, Channel, bool, bMute);

	/**
	 * Delegate fired when master fader value changes from the physical board
	 * @param Level - New master fader level (0.0 to 1.0)
	 */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMasterFaderChanged, float, Level);

	/** Channel fader changed event (bind UMG widgets to this) */
	UPROPERTY(BlueprintAssignable, Category = "HoloCade|ProAudio|Events")
	FOnChannelFaderChanged OnChannelFaderChanged;

	/** Channel mute changed event (bind UMG widgets to this) */
	UPROPERTY(BlueprintAssignable, Category = "HoloCade|ProAudio|Events")
	FOnChannelMuteChanged OnChannelMuteChanged;

	/** Master fader changed event (bind UMG widgets to this) */
	UPROPERTY(BlueprintAssignable, Category = "HoloCade|ProAudio|Events")
	FOnMasterFaderChanged OnMasterFaderChanged;

	/**
	 * Register a channel for bidirectional sync
	 * When physical board sends updates, OnChannelFaderChanged/OnChannelMuteChanged will fire
	 * This prepares the controller to auto-map UMG widgets to physical mixer channels
	 * 
	 * @param Channel - Virtual channel number (1-based) used by UMG widgets
	 * @param PhysicalChannel - Hardware channel number (1-based). MUST be specified and valid for the console type.
	 *                         Returns false if PhysicalChannel is invalid (out of range for console).
	 * 
	 * Example: RegisterChannelForSync(1, 5) means "UMG channel 1 maps to hardware channel 5"
	 * 
	 * @return True if registration succeeded, false if PhysicalChannel is invalid for this console type
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|ProAudio|Widget Mapping")
	bool RegisterChannelForSync(int32 Channel, int32 PhysicalChannel);

	/**
	 * Get maximum number of channels supported by the configured console type
	 * @return Maximum channel number (e.g., 32 for X32, 48 for Wing, etc.)
	 */
	UFUNCTION(BlueprintPure, Category = "HoloCade|ProAudio|Widget Mapping")
	int32 GetMaxChannelsForConsole() const;

	/**
	 * Unregister a channel (stop syncing)
	 * @param Channel - Virtual channel number to unregister
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|ProAudio|Widget Mapping")
	void UnregisterChannelForSync(int32 Channel);

	/**
	 * Get the physical hardware channel number for a virtual channel
	 * @param VirtualChannel - Virtual channel number
	 * @return Physical hardware channel number, or -1 if not mapped
	 */
	UFUNCTION(BlueprintPure, Category = "HoloCade|ProAudio|Widget Mapping")
	int32 GetPhysicalChannel(int32 VirtualChannel) const;

	/**
	 * Check if bidirectional sync is enabled and ready
	 * Returns true if OSC receive is enabled and server is listening
	 */
	UFUNCTION(BlueprintPure, Category = "HoloCade|ProAudio|Widget Mapping")
	bool IsBidirectionalSyncEnabled() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	/** OSC Client for sending commands */
	UPROPERTY()
	UOSCClient* OSCClient = nullptr;

	/** OSC Server for receiving (if bidirectional) */
	UPROPERTY()
	UOSCServer* OSCServer = nullptr;

	bool bIsInitialized = false;

	/** Set of registered virtual channels for bidirectional sync (used by UMG templates) */
	TSet<int32> RegisteredChannelsForSync;

	/** Mapping from virtual channel (UMG) to physical channel (hardware) */
	TMap<int32, int32> VirtualToPhysicalChannelMap;

	/** Build OSC address path for console-specific commands */
	FString BuildOSCPath(const FString& Command, int32 Channel = -1, int32 Bus = -1) const;

	/** Convert normalized level (0-1) to console-specific format */
	float ConvertLevelToConsole(float NormalizedLevel) const;

	/** Extract channel number from OSC address (e.g., /ch/01/mix/fader -> 1) */
	int32 ExtractChannelFromOSCAddress(const FString& OSCAddress) const;

	/** Extract bus number from OSC address (e.g., /ch/01/mix/02/level -> bus 2) */
	int32 ExtractBusFromOSCAddress(const FString& OSCAddress) const;

	/** Find first available physical channel (not already mapped) */
	int32 FindFirstAvailablePhysicalChannel() const;

	/** Main OSC message handler (bound to OnOscMessageReceived) */
	UFUNCTION()
	void OnOSCMessageReceived(const FOSCMessage& Message, const FString& IPAddress, int32 Port);

	/** OSC event handlers (called by OnOSCMessageReceived based on address pattern) */
	void OnOSCFaderReceived(const FOSCAddress& AddressPattern, const FOSCMessage& Message);

	void OnOSCMuteReceived(const FOSCAddress& AddressPattern, const FOSCMessage& Message);

	void OnOSCMasterFaderReceived(const FOSCAddress& AddressPattern, const FOSCMessage& Message);

	void OnOSCBusSendReceived(const FOSCAddress& AddressPattern, const FOSCMessage& Message);
};

