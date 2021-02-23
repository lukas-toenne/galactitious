// Fill out your copyright notice in the Description page of Project Settings.

#include "FastMultipoleCachePlayer.h"

#include "FastMultipoleSimulationCache.h"

DEFINE_LOG_CATEGORY_STATIC(LogFastMultipoleCachePlayer, Log, All);

//------------------------------------------------------------------------------------------------------------

FFastMultipoleCachePlayer::FFastMultipoleCachePlayer()
{
}

void FFastMultipoleCachePlayer::SetSimulationCache(UFastMultipoleSimulationCache* InSimulationCache)
{
	SimulationCacheWeak = InSimulationCache;
	SetToFront();
}

float FFastMultipoleCachePlayer::GetTime() const
{
	return (float)CacheStep + AnimationTime;
}

void FFastMultipoleCachePlayer::SetTime(float Time)
{
	UFastMultipoleSimulationCache* SimulationCache = SimulationCacheWeak.Get();
	if (!SimulationCache)
	{
		return;
	}

	CacheStep = FMath::FloorToInt(Time);
	AnimationTime = FMath::Frac(Time);

	UpdateCacheFrames();
}

void FFastMultipoleCachePlayer::StepForward(float DeltaTime)
{
	UFastMultipoleSimulationCache* SimulationCache = SimulationCacheWeak.Get();
	if (!SimulationCache)
	{
		return;
	}

	const int32 NumFrames = SimulationCache->GetNumFrames();
	const int32 OldCacheStep = CacheStep;
	const float OldAnimationTime = AnimationTime;

	if (CacheStep < NumFrames - 1)
	{
		AnimationTime += DeltaTime * AnimationSpeed;
		if (AnimationTime >= 1.0f)
		{
			CacheStep += FMath::FloorToInt(AnimationTime);
			if (CacheStep < NumFrames - 1)
			{
				AnimationTime = FMath::Frac(AnimationTime);
			}
			else
			{
				// Animation will stop until more frames are added
				CacheStep = NumFrames - 1;
				AnimationTime = 0.0f;
			}
		}
	}
	else
	{
		CacheStep = NumFrames - 1;
		AnimationTime = 0.0f;
	}

	if (CacheStep != OldCacheStep || AnimationTime != OldAnimationTime)
	{
		UpdateCacheFrames();
	}
}

void FFastMultipoleCachePlayer::StepBack(float DeltaTime)
{
	UFastMultipoleSimulationCache* SimulationCache = SimulationCacheWeak.Get();
	if (!SimulationCache)
	{
		return;
	}

	const int32 NumFrames = SimulationCache->GetNumFrames();
	const int32 OldCacheStep = CacheStep;
	const float OldAnimationTime = AnimationTime;

	if (CacheStep >= 0)
	{
		AnimationTime -= DeltaTime * AnimationSpeed;
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

	if (CacheStep != OldCacheStep || AnimationTime != OldAnimationTime)
	{
		UpdateCacheFrames();
	}
}

void FFastMultipoleCachePlayer::SetToFront()
{
	UFastMultipoleSimulationCache* SimulationCache = SimulationCacheWeak.Get();
	if (!SimulationCache)
	{
		return;
	}

	CacheStep = 0;
	AnimationTime = 0.0f;

	UpdateCacheFrames();
}

void FFastMultipoleCachePlayer::SetToBack()
{
	UFastMultipoleSimulationCache* SimulationCache = SimulationCacheWeak.Get();
	if (!SimulationCache)
	{
		return;
	}

	CacheStep = SimulationCache->GetNumFrames() - 1;
	AnimationTime = 0.0f;

	UpdateCacheFrames();
}

void FFastMultipoleCachePlayer::UpdateCacheFrames()
{
	if (const UFastMultipoleSimulationCache* SimulationCache = SimulationCacheWeak.Get())
	{
		const int32 NumFrames = SimulationCache->GetNumFrames();
		if (CacheStep < 0)
		{
			StartFrame.Reset();
			EndFrame.Reset();
		}
		else if (CacheStep < NumFrames - 1)
		{
			StartFrame = SimulationCache->GetFrame(CacheStep);
			EndFrame = SimulationCache->GetFrame(CacheStep + 1);
		}
		else if (CacheStep < NumFrames)
		{
			StartFrame = SimulationCache->GetFrame(CacheStep);
			EndFrame.Reset();
		}
		else
		{
			StartFrame.Reset();
			EndFrame.Reset();
		}
	}
	else
	{
		StartFrame.Reset();
		EndFrame.Reset();
	}
}
