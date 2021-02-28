// Fill out your copyright notice in the Description page of Project Settings.

#include "FastMultipoleCachePlayer.h"

#include "FastMultipoleSimulationCache.h"

DEFINE_LOG_CATEGORY_STATIC(LogFastMultipoleCachePlayer, Log, All);

float FFastMultipoleCacheTime::GetTime() const
{
	return (float)CacheStep + AnimationTime;
}

void FFastMultipoleCacheTime::SetTime(float Time)
{
	CacheStep = FMath::FloorToInt(Time);
	AnimationTime = FMath::Frac(Time);
}

void FFastMultipoleCacheTime::StepForward(float DeltaTime, int32 NumCacheSteps)
{
	const int32 OldCacheStep = CacheStep;
	const float OldAnimationTime = AnimationTime;

	if (NumCacheSteps < 0)
	{
		AnimationTime += DeltaTime;
		if (AnimationTime >= 1.0f)
		{
			CacheStep += FMath::FloorToInt(AnimationTime);
			AnimationTime = FMath::Frac(AnimationTime);
		}
	}
	else if (CacheStep < NumCacheSteps - 1)
	{
		AnimationTime += DeltaTime;
		if (AnimationTime >= 1.0f)
		{
			CacheStep += FMath::FloorToInt(AnimationTime);
			if (CacheStep < NumCacheSteps - 1)
			{
				AnimationTime = FMath::Frac(AnimationTime);
			}
			else
			{
				// Animation will stop until more frames are added
				CacheStep = NumCacheSteps - 1;
				AnimationTime = 0.0f;
			}
		}
	}
	else
	{
		CacheStep = NumCacheSteps - 1;
		AnimationTime = 0.0f;
	}
}

void FFastMultipoleCacheTime::StepBack(float DeltaTime)
{
	const int32 OldCacheStep = CacheStep;
	const float OldAnimationTime = AnimationTime;

	if (CacheStep >= 0)
	{
		AnimationTime -= DeltaTime;
		if (AnimationTime < 0.0f)
		{
			CacheStep += FMath::FloorToInt(AnimationTime);
			if (CacheStep >= 0)
			{
				AnimationTime = FMath::Frac(AnimationTime);
			}
			else
			{
				// Animation stop at front
				CacheStep = 0;
				AnimationTime = 0.0f;
			}
		}
	}
	else
	{
		CacheStep = 0;
		AnimationTime = 0.0f;
	}
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
