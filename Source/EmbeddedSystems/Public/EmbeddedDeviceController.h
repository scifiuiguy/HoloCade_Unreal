// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Networking/HoloCadeUDPTransport.h"
#include "HoloCadeEmbeddedDeviceInterface.h"
#include "EmbeddedDeviceController.generated.h"

/**
 * Supported microcontroller types
 */
UENUM(BlueprintType)
enum class EHoloCadeMicrocontrollerType : uint8
{
	Arduino UMETA(DisplayName = "Arduino"),
	ESP32 UMETA(DisplayName = "ESP32"),
	STM32 UMETA(DisplayName = "STM32"),
	RaspberryPi UMETA(DisplayName = "Raspberry Pi"),
	Jetson UMETA(DisplayName = "NVIDIA Jetson"),
	Custom UMETA(DisplayName = "Custom")
};

/**
 * Communication protocol type
 */
UENUM(BlueprintType)
enum class EHoloCadeCommProtocol : uint8
{
	Serial UMETA(DisplayName = "Serial (USB/UART)"),
	WiFi UMETA(DisplayName = "WiFi (UDP/TCP)"),
	Bluetooth UMETA(DisplayName = "Bluetooth"),
	Ethernet UMETA(DisplayName = "Ethernet")
};

/**
 * Input type from embedded device
 */
UENUM(BlueprintType)
enum class EHoloCadeInputType : uint8
{
	Discrete UMETA(DisplayName = "Discrete (Button Press)"),
	Continuous UMETA(DisplayName = "Continuous (Analog)")
};

/**
 * Output type to embedded device
 */
UENUM(BlueprintType)
enum class EHoloCadeOutputType : uint8
{
	Discrete UMETA(DisplayName = "Discrete (On/Off)"),
	Continuous UMETA(DisplayName = "Continuous (PWM/Analog)")
};

/**
 * Security level for embedded communication
 */
UENUM(BlueprintType)
enum class EHoloCadeSecurityLevel : uint8
{
	None UMETA(DisplayName = "None (Development Only)"),
	HMAC UMETA(DisplayName = "HMAC Authentication"),
	Encrypted UMETA(DisplayName = "AES-128 + HMAC (Recommended)"),
	DTLS UMETA(DisplayName = "DTLS (Future)")
};

/**
 * Configuration for embedded device
 */
USTRUCT(BlueprintType)
struct FEmbeddedDeviceConfig
{
	GENERATED_BODY()

	/** Type of microcontroller */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Embedded")
	EHoloCadeMicrocontrollerType DeviceType = EHoloCadeMicrocontrollerType::ESP32;

	/** Communication protocol */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Embedded")
	EHoloCadeCommProtocol Protocol = EHoloCadeCommProtocol::WiFi;

	/** Device address (COM port, IP address, MAC address, etc.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Embedded")
	FString DeviceAddress = TEXT("192.168.1.50");

	/** Port number (for network protocols) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Embedded")
	int32 Port = 8888;

	/** Baud rate (for serial communication) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Embedded")
	int32 BaudRate = 115200;

	/** Number of input pins/channels */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Embedded")
	int32 InputChannelCount = 8;

	/** Number of output pins/channels */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Embedded")
	int32 OutputChannelCount = 8;

	/** Enable debug mode (uses JSON instead of binary, easier to debug with Wireshark) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Embedded")
	bool bDebugMode = false;

	/** Security level for packet encryption and authentication */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Embedded|Security")
	EHoloCadeSecurityLevel SecurityLevel = EHoloCadeSecurityLevel::Encrypted;

	/** Shared secret key for HMAC/AES (must match device firmware) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Embedded|Security", meta = (PasswordField = true))
	FString SharedSecret = TEXT("CHANGE_ME_IN_PRODUCTION_2025");

	/** AES encryption key (auto-derived from SharedSecret if empty) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Embedded|Security|Advanced")
	FString AESKey128;

	/** HMAC key (auto-derived from SharedSecret if empty) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Embedded|Security|Advanced")
	FString HMACKey;
};

/**
 * Input data from embedded device
 */
USTRUCT(BlueprintType)
struct FEmbeddedInputData
{
	GENERATED_BODY()

	/** Channel/pin number */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Embedded")
	int32 Channel = 0;

	/** Input type */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Embedded")
	EHoloCadeInputType InputType = EHoloCadeInputType::Discrete;

	/** Value (0-1 for analog, 0 or 1 for digital) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Embedded")
	float Value = 0.0f;

	/** Timestamp */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Embedded")
	float Timestamp = 0.0f;
};

/**
 * Output command to embedded device
 */
USTRUCT(BlueprintType)
struct FEmbeddedOutputCommand
{
	GENERATED_BODY()

	/** Channel/pin number */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Embedded")
	int32 Channel = 0;

	/** Output type */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Embedded")
	EHoloCadeOutputType OutputType = EHoloCadeOutputType::Discrete;

	/** Value (0-1 for PWM/analog, 0 or 1 for digital) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Embedded")
	float Value = 0.0f;

	/** Duration for timed outputs (0 = continuous) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Embedded")
	float Duration = 0.0f;
};

// Delegate for input events
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEmbeddedInputReceived, FEmbeddedInputData, InputData);

// Note: FOnBoolReceived, FOnInt32Received, FOnFloatReceived, FOnStringReceived, FOnBytesReceived
// are inherited from UHoloCadeUDPTransport base class

/**
 * Data type enum for binary protocol
 */
UENUM(BlueprintType)
enum class EHoloCadeDataType : uint8
{
	Bool = 0 UMETA(DisplayName = "Boolean"),
	Int32 = 1 UMETA(DisplayName = "Integer"),
	Float = 2 UMETA(DisplayName = "Float"),
	String = 3 UMETA(DisplayName = "String"),
	Bytes = 4 UMETA(DisplayName = "Raw Bytes"),
	Struct = 5 UMETA(DisplayName = "Struct")
};

/**
 * Embedded Device Controller Component
 * 
 * Manages communication with embedded microcontrollers for:
 * - Button/trigger input from costume-mounted or prop-mounted sensors
 * - Haptic output to vibrators/kickers in costumes or props
 * - Integration with narrative state machines
 * - Wireless and wired communication protocols
 */
UCLASS(ClassGroup=(HoloCade), meta=(BlueprintSpawnableComponent))
class EMBEDDEDSYSTEMS_API UEmbeddedDeviceController : public UHoloCadeUDPTransport, public IHoloCadeEmbeddedDeviceInterface
{
	GENERATED_BODY()

public:	
	UEmbeddedDeviceController();

	/** Device configuration */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoloCade|Embedded")
	FEmbeddedDeviceConfig Config;

	/** Event fired when input is received from device (legacy) */
	UPROPERTY(BlueprintAssignable, Category = "HoloCade|Embedded")
	FOnEmbeddedInputReceived OnInputReceived;

	// Note: OnBoolReceived, OnInt32Received, OnFloatReceived, OnStringReceived, OnBytesReceived
	// are inherited from UHoloCadeUDPTransport base class

	/**
	 * Initialize connection to embedded device
	 * @param InConfig - Configuration settings
	 * @return true if initialization was successful
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Embedded")
	bool InitializeDevice(const FEmbeddedDeviceConfig& InConfig);

	/**
	 * Send output command to device
	 * @param Command - The output command to send
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Embedded")
	void SendOutputCommand(const FEmbeddedOutputCommand& Command);

	/**
	 * Trigger a haptic pulse on a specific channel
	 * @param Channel - Output channel number
	 * @param Intensity - Intensity (0-1)
	 * @param Duration - Duration in seconds
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Embedded")
	void TriggerHapticPulse(int32 Channel, float Intensity, float Duration);

	/**
	 * Set continuous output on a channel
	 * @param Channel - Output channel number
	 * @param Value - Output value (0-1)
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Embedded")
	void SetContinuousOutput(int32 Channel, float Value);

	/**
	 * Get the most recent input value for a channel
	 * @param Channel - Input channel number
	 * @return The most recent input value
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Embedded")
	float GetInputValue(int32 Channel) const;

	/**
	 * Check if device is connected and responding
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Embedded")
	bool IsDeviceConnected() const;

	/**
	 * Disconnect from device
	 */
	UFUNCTION(BlueprintCallable, Category = "HoloCade|Embedded")
	void DisconnectDevice();

	// =====================================
	// Binary Protocol - Primitive Send API
	// =====================================
	// Note: These methods shadow base class methods to add encryption/HMAC and JSON debug mode support
	// They are NOT UFUNCTIONs (cannot override UFUNCTIONs in Unreal), but work in C++ code
	// Blueprint code will use the base class methods (without encryption/HMAC)

	/**
	 * Send a boolean value to device (with encryption/HMAC support)
	 * @param Channel - Channel/pin number
	 * @param Value - Boolean value to send
	 */
	void SendBool(int32 Channel, bool Value);

	/**
	 * Send an integer value to device (with encryption/HMAC support)
	 * @param Channel - Channel/pin number
	 * @param Value - Integer value to send
	 */
	void SendInt32(int32 Channel, int32 Value);

	/**
	 * Send a float value to device (with encryption/HMAC support)
	 * @param Channel - Channel/pin number
	 * @param Value - Float value to send
	 */
	void SendFloat(int32 Channel, float Value);

	/**
	 * Send a string value to device (with encryption/HMAC support)
	 * @param Channel - Channel/pin number
	 * @param Value - String value to send (max 255 bytes)
	 */
	void SendString(int32 Channel, const FString& Value);

	/**
	 * Send raw bytes to device (with encryption/HMAC support)
	 * @param Channel - Channel/pin number
	 * @param Data - Raw byte array
	 */
	void SendBytes(int32 Channel, const TArray<uint8>& Data);

	/**
	 * Send a POD struct to device (template, C++ only)
	 * @param Channel - Channel/pin number
	 * @param Data - Struct data (must be POD type)
	 */
	template<typename T>
	void SendStruct(int32 Channel, const T& Data)
	{
		static_assert(TIsPODType<T>::Value, "SendStruct requires POD (Plain Old Data) type");
		TArray<uint8> Bytes;
		Bytes.SetNumUninitialized(sizeof(T));
		FMemory::Memcpy(Bytes.GetData(), &Data, sizeof(T));
		SendBytes(Channel, Bytes);
	}

	/**
	 * Get digital input state (button press)
	 * @param Channel - Channel/pin number
	 * @return true if button is pressed, false otherwise
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|Embedded|Read")
	bool GetDigitalInput(int32 Channel) const;

	/**
	 * Get analog input value (0.0 to 1.0)
	 * @param Channel - Channel/pin number
	 * @return Analog value from 0.0 to 1.0
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HoloCade|Embedded|Read")
	float GetAnalogInput(int32 Channel) const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	/** Whether device is initialized and connected */
	bool bIsConnected = false;

	/** Cache of most recent input values per channel */
	TMap<int32, float> InputValueCache;

	/** Timestamp of last successful communication */
	float LastCommTimestamp = 0.0f;

	// UDP socket management now handled by base class (UHoloCadeUDPTransport)

	/** Receive buffer for incoming packets */
	TArray<uint8> ReceiveBuffer;

	// Protocol start marker (0xAA) now in base class (UHoloCadeUDPTransport)

	/** Derived AES key (16 bytes for AES-128) */
	uint8 DerivedAESKey[16];

	/** Derived HMAC key (32 bytes for SHA-256) */
	uint8 DerivedHMACKey[32];

	/** Random number generator state */
	uint32 RandomState;

	/**
	 * Process incoming data from device
	 */
	void ProcessIncomingData();

	/**
	 * Send data to device (protocol-agnostic)
	 */
	void SendDataToDevice(const TArray<uint8>& Data);

	/**
	 * Check connection health
	 */
	void CheckConnectionHealth();

	/**
	 * Initialize WiFi/Ethernet connection (UDP) - uses base class
	 */
	bool InitializeWiFiConnection();

	/**
	 * Initialize Serial connection (COM port)
	 */
	bool InitializeSerialConnection();

	/**
	 * Send data via UDP - uses base class
	 */
	void SendWiFiData(const TArray<uint8>& Data);

	/**
	 * Receive data via UDP - uses base class
	 */
	void ReceiveWiFiData();

	/**
	 * Build binary packet for transmission (with encryption/HMAC support)
	 * Overrides base class to add security features
	 */
	TArray<uint8> BuildBinaryPacket(EHoloCadeDataType Type, int32 Channel, const TArray<uint8>& Payload);

	/**
	 * Build JSON packet for transmission (debug mode)
	 */
	TArray<uint8> BuildJSONPacket(EHoloCadeDataType Type, int32 Channel, const FString& ValueString);

	/**
	 * Parse incoming binary packet (with encryption/HMAC support)
	 * Overrides base class to add security features
	 */
	void ParseBinaryPacket(const TArray<uint8>& Data, int32 Length);

	/**
	 * Parse incoming JSON packet (debug mode)
	 */
	void ParseJSONPacket(const TArray<uint8>& Data, int32 Length);

	/**
	 * Calculate CRC checksum (XOR-based) - same as base class, but kept for encryption/HMAC
	 */
	uint8 CalculateCRC(const TArray<uint8>& Data, int32 Length) const;

	/**
	 * Validate CRC checksum - same as base class, but kept for encryption/HMAC
	 */
	bool ValidateCRC(const TArray<uint8>& Data, int32 Length, uint8 ExpectedCRC) const;

	/**
	 * Derive encryption keys from shared secret
	 */
	void DeriveKeysFromSecret();

	/**
	 * Encrypt payload using AES-128-CTR
	 * @param Plaintext - Data to encrypt
	 * @param IV - 4-byte initialization vector (random per packet)
	 * @return Encrypted data (same length as plaintext)
	 */
	TArray<uint8> EncryptAES128(const TArray<uint8>& Plaintext, uint32 IV) const;

	/**
	 * Decrypt payload using AES-128-CTR
	 * @param Ciphertext - Encrypted data
	 * @param IV - 4-byte initialization vector from packet
	 * @return Decrypted data
	 */
	TArray<uint8> DecryptAES128(const TArray<uint8>& Ciphertext, uint32 IV) const;

	/**
	 * Calculate HMAC-SHA256 (truncated to 8 bytes)
	 * @param Data - Data to authenticate
	 * @return 8-byte HMAC tag
	 */
	TArray<uint8> CalculateHMAC(const TArray<uint8>& Data) const;

	/**
	 * Validate HMAC-SHA256
	 * @param Data - Data (without HMAC)
	 * @param ExpectedHMAC - 8-byte HMAC from packet
	 * @return true if HMAC matches
	 */
	bool ValidateHMAC(const TArray<uint8>& Data, const TArray<uint8>& ExpectedHMAC) const;

	/**
	 * Generate random 32-bit value for IV
	 */
	uint32 GenerateRandomIV();
};



