// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FastMultipoleSimulationFrame.h"
#include "FastMultipoleTypes.h"

#include "FastMultipoleCachePlayer.generated.h"

class UFastMultipoleSimulationCache;

USTRUCT()
struct FASTMULTIPOLESIMULATION_API FFastMultipoleCacheTime
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category = "Fast Multipole Cache Time")
	int32 CacheStep;

	UPROPERTY(EditAnywhere, Category = "Fast Multipole Cache Time")
	float AnimationTime;

	FFastMultipoleCacheTime() : CacheStep(0), AnimationTime(0.0f) {}

	FFastMultipoleCacheTime(int32 InCacheStep, float InAnimationTime) : CacheStep(InCacheStep), AnimationTime(InAnimationTime) {}

	inline float GetTime() const;
	void SetTime(float Time);

	void StepForward(float DeltaTime, int32 NumCacheSteps = -1);
	void StepBack(float DeltaTime);
	void SetToFront();
	void SetToBack(int32 NumCacheSteps);

	void GetCacheFrames(
		const UFastMultipoleSimulationCache* Cache, FFastMultipoleSimulationFrame::ConstPtr& OutStartFrame,
		FFastMultipoleSimulationFrame::ConstPtr& OutEndFrame) const;

	inline bool operator==(const FFastMultipoleCacheTime& Other) const
	{
		if ((Other.CacheStep != CacheStep) || (Other.AnimationTime != AnimationTime))
		{
			return false;
		}

		return true;
	}

	inline bool operator!=(const FFastMultipoleCacheTime& Other) const { return !(*this == Other); }
};
