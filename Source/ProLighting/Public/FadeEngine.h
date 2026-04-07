// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "ProLighting.h"

struct FFadeState
{
	float CurrentIntensity = 0.0f;
	float TargetIntensity = 0.0f;
	float FadeSpeed = 0.0f;
	bool bFading = false;
};

class PROLIGHTING_API FFadeEngine
{
public:
	void StartFade(int32 VirtualId, float Current, float Target, float DurationSec)
	{
		FFadeState& S = States.FindOrAdd(VirtualId);
		S.CurrentIntensity = Current;
		S.TargetIntensity = FMath::Clamp(Target, 0.0f, 1.0f);
		S.FadeSpeed = (DurationSec > 0.0f) ? (FMath::Abs(S.TargetIntensity - S.CurrentIntensity) / DurationSec) : 0.0f;
		S.bFading = DurationSec > 0.0f && S.FadeSpeed > 0.0f;
	}

	void Cancel(int32 VirtualId)
	{
		States.Remove(VirtualId);
	}

	void Tick(float DeltaTime, TFunctionRef<void(int32,float)> OnIntensity)
	{
		for (auto& Pair : States)
		{
			int32 Id = Pair.Key;
			FFadeState& S = Pair.Value;
			if (!S.bFading) continue;
			float Delta = S.FadeSpeed * DeltaTime;
			if (FMath::Abs(S.TargetIntensity - S.CurrentIntensity) <= Delta)
			{
				S.CurrentIntensity = S.TargetIntensity;
				S.bFading = false;
			}
			else
			{
				S.CurrentIntensity += (S.TargetIntensity > S.CurrentIntensity) ? Delta : -Delta;
			}
			OnIntensity(Id, S.CurrentIntensity);
		}
		// Remove finished fades
		for (auto It = States.CreateIterator(); It; ++It)
		{
			if (!It.Value().bFading) It.RemoveCurrent();
		}
	}

private:
	TMap<int32, FFadeState> States;
};




