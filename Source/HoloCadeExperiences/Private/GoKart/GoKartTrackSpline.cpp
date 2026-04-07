// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "GoKart/GoKartTrackSpline.h"
#include "Components/SplineComponent.h"

AGoKartTrackSpline::AGoKartTrackSpline()
{
	PrimaryActorTick.bCanEverTick = false;

	SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponent"));
	RootComponent = SplineComponent;
	TrackName = TEXT("Unnamed Track");
}

FVector AGoKartTrackSpline::GetLocationAtDistance(float Distance) const
{
	if (SplineComponent)
	{
		float InputKey = SplineComponent->GetInputKeyAtDistanceAlongSpline(Distance);
		return SplineComponent->GetLocationAtSplineInputKey(InputKey, ESplineCoordinateSpace::World);
	}
	return FVector::ZeroVector;
}

FRotator AGoKartTrackSpline::GetRotationAtDistance(float Distance) const
{
	if (SplineComponent)
	{
		float InputKey = SplineComponent->GetInputKeyAtDistanceAlongSpline(Distance);
		FQuat RotationQuat = SplineComponent->GetQuaternionAtSplineInputKey(InputKey, ESplineCoordinateSpace::World);
		return RotationQuat.Rotator();
	}
	return FRotator::ZeroRotator;
}

float AGoKartTrackSpline::GetTrackLength() const
{
	if (SplineComponent)
	{
		return SplineComponent->GetSplineLength();
	}
	return 0.0f;
}

float AGoKartTrackSpline::GetProgressFromDistance(float Distance) const
{
	float Length = GetTrackLength();
	if (Length > 0.0f)
	{
		return FMath::Clamp(Distance / Length, 0.0f, 1.0f);
	}
	return 0.0f;
}

