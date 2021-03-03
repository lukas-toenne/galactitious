// Fill out your copyright notice in the Description page of Project Settings.

#include "FastMultipoleCachePlayer.h"

#include "FastMultipoleSimulationCache.h"

DEFINE_LOG_CATEGORY_STATIC(LogFastMultipoleCachePlayer, Log, All);

void FFastMultipoleCachePlayer::GetCacheFrames(
	const UFastMultipoleSimulationCache* Cache, int32 CacheStep, FFastMultipoleSimulationFrame::ConstPtr& OutStartFrame,
	FFastMultipoleSimulationFrame::ConstPtr& OutEndFrame)
{
	if (Cache)
	{
		const int32 NumFrames = Cache->GetNumFrames();
		if (CacheStep < 0)
		{
			OutStartFrame.Reset();
			OutEndFrame.Reset();
		}
		else if (CacheStep < NumFrames - 1)
		{
			OutStartFrame = Cache->GetFrame(CacheStep);
			OutEndFrame = Cache->GetFrame(CacheStep + 1);
		}
		else if (CacheStep < NumFrames)
		{
			OutStartFrame = Cache->GetFrame(CacheStep);
			OutEndFrame.Reset();
		}
		else
		{
			OutStartFrame.Reset();
			OutEndFrame.Reset();
		}
	}
	else
	{
		OutStartFrame.Reset();
		OutEndFrame.Reset();
	}
}

bool FFastMultipoleCachePlayer::Clamp(int32 CacheStep, float CacheInterp, int32 NumCacheSteps, int32& OutCacheStep, float& OutCacheInterp)
{
	if (CacheStep < 0)
	{
		OutCacheStep = 0;
		OutCacheInterp = 0.0f;
		return true;
	}
	else if (CacheStep >= NumCacheSteps - 1 && CacheInterp > 0.0f)
	{
		OutCacheStep = NumCacheSteps - 1;
		OutCacheInterp = 0.0f;
		return true;
	}
	else
	{
		OutCacheStep = CacheStep;
		OutCacheInterp = CacheInterp;
		return false;
	}
}

#if 0
float FFastMultipoleCacheTime::GetTime() const
{
	return (float)CacheStep + AnimationTime;
}

void FFastMultipoleCacheTime::SetTime(float Time)
{
	CacheStep = FMath::FloorToInt(Time);
	AnimationTime = FMath::Frac(Time);
}

void FFastMultipoleCacheTime::StepForward(float DeltaTime)
{
	AnimationTime += DeltaTime;
	if (AnimationTime >= 1.0f)
	{
		CacheStep += FMath::FloorToInt(AnimationTime);
		AnimationTime = FMath::Frac(AnimationTime);
	}
}

void FFastMultipoleCacheTime::StepBack(float DeltaTime)
{
	AnimationTime -= DeltaTime;
	if (AnimationTime < 0.0f)
	{
		CacheStep += FMath::FloorToInt(AnimationTime);
		AnimationTime = FMath::Frac(AnimationTime);
	}
}

bool FFastMultipoleCacheTime::Clamp(int32 NumCacheSteps)
{
	const int32 OldCacheStep = CacheStep;
	const float OldAnimationTime = AnimationTime;

	if (CacheStep < 0)
	{
		CacheStep = 0;
		AnimationTime = 0.0f;
		return true;
	}
	else if (CacheStep >= NumCacheSteps - 1 && AnimationTime > 0.0f)
	{
		CacheStep = NumCacheSteps - 1;
		AnimationTime = 0.0f;
		return true;
	}

	return false;
}

void FFastMultipoleCacheTime::SetToFront()
{
	CacheStep = 0;
	AnimationTime = 0.0f;
}

void FFastMultipoleCacheTime::SetToBack(int32 NumCacheSteps)
{
	CacheStep = NumCacheSteps - 1;
	AnimationTime = 0.0f;
}

void FFastMultipoleCacheTime::GetCacheFrames(
	const UFastMultipoleSimulationCache* Cache, FFastMultipoleSimulationFrame::ConstPtr& OutStartFrame,
	FFastMultipoleSimulationFrame::ConstPtr& OutEndFrame) const
{
	if (Cache)
	{
		const int32 NumFrames = Cache->GetNumFrames();
		if (CacheStep < 0)
		{
			OutStartFrame.Reset();
			OutEndFrame.Reset();
		}
		else if (CacheStep < NumFrames - 1)
		{
			OutStartFrame = Cache->GetFrame(CacheStep);
			OutEndFrame = Cache->GetFrame(CacheStep + 1);
		}
		else if (CacheStep < NumFrames)
		{
			OutStartFrame = Cache->GetFrame(CacheStep);
			OutEndFrame.Reset();
		}
		else
		{
			OutStartFrame.Reset();
			OutEndFrame.Reset();
		}
	}
	else
	{
		OutStartFrame.Reset();
		OutEndFrame.Reset();
	}
}
#endif
