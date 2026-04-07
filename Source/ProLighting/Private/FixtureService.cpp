// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "ProLighting/Public/FixtureService.h"
#include "ProLighting/Public/RDMService.h"
#include "ProLighting/Public/ProLightingController.h"

FFixtureService::FFixtureService(FUniverseBuffer& InBuffer)
	: Buffer(InBuffer) {}

void FFixtureService::SetRDMContext(FRDMService* InRDMService, TMap<int32, FString>* InVirtualToUID, TMap<FString, int32>* InUIDToVirtual)
{
	RDMService = InRDMService;
	VirtualToUID = InVirtualToUID;
	UIDToVirtual = InUIDToVirtual;
}

bool FFixtureService::ValidateAndRegister(const FHoloCadeDMXFixture& Fixture)
{
	FString Err;
	if (!FFixtureValidator::ValidateRegister(Fixture, Registry, Err))
	{
		UE_LOG(LogProLighting, Error, TEXT("FixtureService: Register failed - %s"), *Err);
		return false;
	}
	Buffer.EnsureUniverse(Fixture.Universe);
	FHoloCadeDMXFixture Valid = Fixture;
	if (Valid.ChannelCount <= 0)
	{
		Valid.ChannelCount = (Fixture.FixtureType == EHoloCadeDMXFixtureType::Custom)
			? FMath::Max(1, Fixture.CustomChannelMapping.Num())
			: (Fixture.FixtureType == EHoloCadeDMXFixtureType::Dimmable ? 1 : (Fixture.FixtureType == EHoloCadeDMXFixtureType::RGB ? 3 : (Fixture.FixtureType == EHoloCadeDMXFixtureType::RGBW ? 4 : 8)));
	}
	return Registry.Register(Valid);
}

void FFixtureService::Unregister(int32 VirtualFixtureID)
{
	Registry.Unregister(VirtualFixtureID);
	Fade.Cancel(VirtualFixtureID);
}

void FFixtureService::ApplyIntensity(const FHoloCadeDMXFixture& Fixture, float Intensity)
{
	TUniquePtr<IFixtureDriver> Driver = FFixtureDriverFactory::Create(Fixture.FixtureType);
	Driver->ApplyIntensity(Fixture, Intensity, Buffer);
}

void FFixtureService::ApplyColor(const FHoloCadeDMXFixture& Fixture, float Red, float Green, float Blue, float White)
{
	TUniquePtr<IFixtureDriver> Driver = FFixtureDriverFactory::Create(Fixture.FixtureType);
	Driver->ApplyColor(Fixture, Red, Green, Blue, White, Buffer);
}

void FFixtureService::ApplyChannelRaw(const FHoloCadeDMXFixture& Fixture, int32 ChannelOffset, uint8 Value)
{
	Buffer.SetChannel(Fixture.Universe, Fixture.DMXChannel + ChannelOffset, Value);
}

void FFixtureService::StartFade(int32 VirtualFixtureID, float Current, float Target, float DurationSec)
{
	Fade.StartFade(VirtualFixtureID, Current, Target, DurationSec);
}

void FFixtureService::TickFades(float DeltaTime, TFunctionRef<void(int32,float)> OnIntensity)
{
	Fade.Tick(DeltaTime, OnIntensity);
}

void FFixtureService::AllOff(TFunctionRef<void(int32,float)> OnIntensity)
{
    for (int32 Id : Registry.GetIDs())
    {
        if (const FHoloCadeDMXFixture* Fx = Registry.Find(Id))
        {
            ApplyIntensity(*Fx, 0.0f);
            OnIntensity(Id, 0.0f);
            IntensityChanged.Broadcast(Id, 0.0f);
        }
    }
}

void FFixtureService::UpdateFixtureOnlineStatus(const FString& RDMUID, bool bIsOnline)
{
	if (!RDMService || !UIDToVirtual) return;
	if (int32* VirtualFixtureID = UIDToVirtual->Find(RDMUID))
	{
		if (bIsOnline)
		{
			RDMService->MarkOnline(RDMUID, *VirtualFixtureID);
		}
		else
		{
			RDMService->MarkOffline(RDMUID, *VirtualFixtureID);
		}
	}
}

int32 FFixtureService::SetIntensityById(int32 VirtualFixtureID, float Intensity)
{
    const FHoloCadeDMXFixture* Fixture = Registry.Find(VirtualFixtureID);
    if (!Fixture) return -1;
    ApplyIntensity(*Fixture, FMath::Clamp(Intensity, 0.0f, 1.0f));
    IntensityChanged.Broadcast(VirtualFixtureID, FMath::Clamp(Intensity, 0.0f, 1.0f));
    return Fixture->Universe;
}

int32 FFixtureService::SetColorRGBWById(int32 VirtualFixtureID, float Red, float Green, float Blue, float White)
{
    const FHoloCadeDMXFixture* Fixture = Registry.Find(VirtualFixtureID);
    if (!Fixture) return -1;
    switch (Fixture->FixtureType)
    {
        case EHoloCadeDMXFixtureType::RGB:
        case EHoloCadeDMXFixtureType::RGBW:
        case EHoloCadeDMXFixtureType::MovingHead:
        case EHoloCadeDMXFixtureType::Custom:
            break;
        default:
            UE_LOG(LogProLighting, Warning, TEXT("FixtureService: Fixture %d does not support color"), VirtualFixtureID);
            return -1;
    }
    ApplyColor(*Fixture,
        FMath::Clamp(Red, 0.0f, 1.0f),
        FMath::Clamp(Green, 0.0f, 1.0f),
        FMath::Clamp(Blue, 0.0f, 1.0f),
        (White >= 0.0f) ? FMath::Clamp(White, 0.0f, 1.0f) : White);
    ColorChanged.Broadcast(VirtualFixtureID,
        FMath::Clamp(Red, 0.0f, 1.0f),
        FMath::Clamp(Green, 0.0f, 1.0f),
        FMath::Clamp(Blue, 0.0f, 1.0f));
    return Fixture->Universe;
}

int32 FFixtureService::SetChannelById(int32 VirtualFixtureID, int32 ChannelOffset, float Value)
{
    const FHoloCadeDMXFixture* Fixture = Registry.Find(VirtualFixtureID);
    if (!Fixture) return -1;
    if (ChannelOffset < 0 || ChannelOffset >= Fixture->ChannelCount)
    {
        UE_LOG(LogProLighting, Warning, TEXT("FixtureService: Invalid channel offset %d for fixture %d"), ChannelOffset, VirtualFixtureID);
        return -1;
    }
    uint8 DMXValue = 0;
    if (Value >= 0.0f && Value <= 1.0f)
    {
        DMXValue = (uint8)(Value * 255.0f);
    }
    else
    {
        DMXValue = (uint8)FMath::Clamp((int32)Value, 0, 255);
    }
    ApplyChannelRaw(*Fixture, ChannelOffset, DMXValue);
    return Fixture->Universe;
}

void FFixtureService::StartFadeById(int32 VirtualFixtureID, float TargetIntensity, float DurationSec)
{
    const FHoloCadeDMXFixture* Fixture = Registry.Find(VirtualFixtureID);
    if (!Fixture) return;
    float Current = Buffer.GetChannel(Fixture->Universe, Fixture->DMXChannel) / 255.0f;
    StartFade(VirtualFixtureID, Current, FMath::Clamp(TargetIntensity, 0.0f, 1.0f), FMath::Max(0.01f, DurationSec));
}

void FFixtureService::AllOffAndNotify(TFunctionRef<void(int32,float)> OnIntensity)
{
    AllOff(OnIntensity);
}

bool FFixtureService::IsFixtureRDMCapable(int32 VirtualFixtureID) const
{
    const FHoloCadeDMXFixture* Fixture = Registry.Find(VirtualFixtureID);
    if (!Fixture)
    {
        return false;
    }
    return Fixture->bRDMCapable;
}

const FHoloCadeDMXFixture* FFixtureService::FindFixture(int32 VirtualFixtureID) const
{
    return Registry.Find(VirtualFixtureID);
}

FHoloCadeDMXFixture* FFixtureService::FindFixtureMutable(int32 VirtualFixtureID)
{
    return Registry.FindMutable(VirtualFixtureID);
}

int32 FFixtureService::GetNextVirtualFixtureID()
{
    return NextVirtualFixtureID++;
}

void FFixtureService::BridgeEvents(UProLightingController* Controller)
{
	if (!Controller)
	{
		return;
	}

	// Bridge intensity changed events
	OnIntensityChanged().AddLambda([Controller](int32 Id, float Intensity)
	{
		Controller->OnFixtureIntensityChanged.Broadcast(Id, Intensity);
	});

	// Bridge color changed events
	OnColorChanged().AddLambda([Controller](int32 Id, float R, float G, float B)
	{
		Controller->OnFixtureColorChanged.Broadcast(Id, R, G, B);
	});
}


