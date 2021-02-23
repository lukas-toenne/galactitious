// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FastMultipoleSimulationFrame.h"
#include "FastMultipoleTypes.h"

class UFastMultipoleSimulationCache;

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

public:
	float AnimationSpeed = 1.0f;

private:
	void UpdateCacheFrames();

private:
	/** Cached ptr to the mesh so that we can make sure that we haven't been deleted. */
	TWeakObjectPtr<UFastMultipoleSimulationCache> SimulationCacheWeak;

	int32 CacheStep = 0;
	float AnimationTime = 0.0f;
	FFastMultipoleSimulationFrame::ConstPtr StartFrame;
	FFastMultipoleSimulationFrame::ConstPtr EndFrame;
};
