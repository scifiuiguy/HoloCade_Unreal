// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "ProLighting.h"
#include "ProLightingTypes.h"
#include "UniverseBuffer.h"
#include "FixtureRegistry.h"
#include "FadeEngine.h"
#include "FixtureDrivers.h"
#include "FixtureValidator.h"
#include "IBridgeEvents.h"

class FRDMService;

/**
 * FFixtureService
 * Encapsulates fixture registration, validation, driver application, fades, and buffer updates.
 */
class PROLIGHTING_API FFixtureService : public IBridgeEvents
{
public:
    FFixtureService(FUniverseBuffer& InBuffer);

    void SetRDMContext(FRDMService* InRDMService, TMap<int32, FString>* InVirtualToUID, TMap<FString, int32>* InUIDToVirtual);

	bool ValidateAndRegister(const FHoloCadeDMXFixture& Fixture);

	void Unregister(int32 VirtualFixtureID);

	void ApplyIntensity(const FHoloCadeDMXFixture& Fixture, float Intensity);

	void ApplyColor(const FHoloCadeDMXFixture& Fixture, float Red, float Green, float Blue, float White);

	void ApplyChannelRaw(const FHoloCadeDMXFixture& Fixture, int32 ChannelOffset, uint8 Value);

	void StartFade(int32 VirtualFixtureID, float Current, float Target, float DurationSec);

	void TickFades(float DeltaTime, TFunctionRef<void(int32,float)> OnIntensity);

	void AllOff(TFunctionRef<void(int32,float)> OnIntensity);

	void UpdateFixtureOnlineStatus(const FString& RDMUID, bool bIsOnline);

    // High-level helpers by VirtualFixtureID (controller delegates)
    int32 SetIntensityById(int32 VirtualFixtureID, float Intensity);
    int32 SetColorRGBWById(int32 VirtualFixtureID, float Red, float Green, float Blue, float White);
    int32 SetChannelById(int32 VirtualFixtureID, int32 ChannelOffset, float Value);
    void StartFadeById(int32 VirtualFixtureID, float TargetIntensity, float DurationSec);
    void AllOffAndNotify(TFunctionRef<void(int32,float)> OnIntensity);

    // Fixture query methods
    bool IsFixtureRDMCapable(int32 VirtualFixtureID) const;
    const FHoloCadeDMXFixture* FindFixture(int32 VirtualFixtureID) const;
    FHoloCadeDMXFixture* FindFixtureMutable(int32 VirtualFixtureID);

    // ID generation
    int32 GetNextVirtualFixtureID();

    // Notifications for controller/UI bridging
    DECLARE_MULTICAST_DELEGATE_TwoParams(FOnIntensityChangedNative, int32 /*VirtualFixtureID*/, float /*Intensity*/);
    DECLARE_MULTICAST_DELEGATE_FourParams(FOnColorChangedNative, int32 /*VirtualFixtureID*/, float /*R*/, float /*G*/, float /*B*/);
    FOnIntensityChangedNative& OnIntensityChanged() { return IntensityChanged; }
    FOnColorChangedNative& OnColorChanged() { return ColorChanged; }

    // Universe buffer access (for flushing)
    const FUniverseBuffer& GetUniverseBuffer() const { return Buffer; }

	// IBridgeEvents interface
	virtual void BridgeEvents(UProLightingController* Controller) override;

private:
    FUniverseBuffer& Buffer;
    FFixtureRegistry Registry;  // Owned by service
    FFadeEngine Fade;            // Owned by service
    FRDMService* RDMService = nullptr;
    TMap<int32, FString>* VirtualToUID = nullptr;
    TMap<FString, int32>* UIDToVirtual = nullptr;

    // ID generation counter
    int32 NextVirtualFixtureID = 1;

    FOnIntensityChangedNative IntensityChanged;
    FOnColorChangedNative ColorChanged;
};


