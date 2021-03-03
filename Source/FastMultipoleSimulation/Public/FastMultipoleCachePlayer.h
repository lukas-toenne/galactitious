// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FastMultipoleSimulationFrame.h"
#include "FastMultipoleTypes.h"

class UFastMultipoleSimulationCache;

struct FASTMULTIPOLESIMULATION_API FFastMultipoleCachePlayer
{
	static void GetCacheFrames(
		const UFastMultipoleSimulationCache* Cache, int32 CacheStep, FFastMultipoleSimulationFrame::ConstPtr& OutStartFrame,
		FFastMultipoleSimulationFrame::ConstPtr& OutEndFrame);

	static bool Clamp(int32 CacheStep, float CacheInterp, int32 NumCacheSteps, int32& OutCacheStep, float& OutCacheInterp);
};
