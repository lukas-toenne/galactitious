// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FastMultipoleSimulationFrame.h"
#include "FastMultipoleTypes.h"

#include "FastMultipoleCachePlayer.generated.h"

class UFastMultipoleSimulationCache;

USTRUCT()
struct FFastMultipoleCacheTime
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category = "Fast Multipole Cache Time")
	int32 CacheStep;

	UPROPERTY(EditAnywhere, Category = "Fast Multipole Cache Time")
	float AnimationTime;

	FFastMultipoleCacheTime() : CacheStep(0), AnimationTime(0.0f) {}

	FFastMultipoleCacheTime(int32 InCacheStep, float InAnimationTime) : CacheStep(InCacheStep), AnimationTime(InAnimationTime) {}

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

struct FASTMULTIPOLESIMULATION_API FFastMultipoleCachePlayer
{
public:
	FFastMultipoleCachePlayer();

	void SetSimulationCache(UFastMultipoleSimulationCache* SimulationCache);

	int32 GetCacheStep() const { return CacheStep; }
	float GetAnimationTime() const { return AnimationTime; }

	float GetTime() const;
	void SetTime(float Time);

	void StepForward(float DeltaTime);
	void StepBack(float DeltaTime);
	void SetToFront();
	void SetToBack();

	FFastMultipoleSimulationFrame::ConstPtr GetStartFrame() const { return StartFrame; }
	FFastMultipoleSimulationFrame::ConstPtr GetEndFrame() const { return EndFrame; }

	void UpdateCacheFrames();

public:
	float AnimationSpeed = 1.0f;

private:
	/** Cached ptr to the mesh so that we can make sure that we haven't been deleted. */
	TWeakObjectPtr<UFastMultipoleSimulationCache> SimulationCacheWeak;

	int32 CacheStep = 0;
	float AnimationTime = 0.0f;
	FFastMultipoleSimulationFrame::ConstPtr StartFrame;
	FFastMultipoleSimulationFrame::ConstPtr EndFrame;
};
