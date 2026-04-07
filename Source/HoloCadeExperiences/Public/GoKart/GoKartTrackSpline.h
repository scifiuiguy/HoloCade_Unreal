// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SplineComponent.h"
#include "GoKartTrackSpline.generated.h"

/**
 * GoKart Track Spline Actor
 * 
 * Blueprint-editable spline for defining go-kart track paths.
 * Used by GoKartTrackGenerator to procedurally generate track geometry and barriers.
 * 
 * Supports multiple splines per experience for easy track switching during debugging.
 */
UCLASS(BlueprintType, ClassGroup=(HoloCade))
class HOLOCADEEXPERIENCES_API AGoKartTrackSpline : public AActor
{
	GENERATED_BODY()
	
public:
	AGoKartTrackSpline();

	/** Spline component for track path */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GoKart|Track")
	TObjectPtr<USplineComponent> SplineComponent;

	/** Track name for debugging/UI */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GoKart|Track")
	FString TrackName = TEXT("Unnamed Track");

	/**
	 * Get distance along spline at a given point
	 * @param Distance - Distance along spline in cm
	 * @return World position at that distance
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GoKart|Track")
	FVector GetLocationAtDistance(float Distance) const;

	/**
	 * Get rotation along spline at a given distance
	 * @param Distance - Distance along spline in cm
	 * @return World rotation at that distance
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GoKart|Track")
	FRotator GetRotationAtDistance(float Distance) const;

	/**
	 * Get total length of track in cm
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GoKart|Track")
	float GetTrackLength() const;

	/**
	 * Get progress (0.0-1.0) from distance along track
	 * @param Distance - Distance along spline in cm
	 * @return Progress value (0.0 = start, 1.0 = end)
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GoKart|Track")
	float GetProgressFromDistance(float Distance) const;
};

