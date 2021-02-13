// Fill out your copyright notice in the Description page of Project Settings.

#include "GalaxySimulationCachePlayer.h"

#include "FastMultipoleSimulationCache.h"

#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogGalaxySimulationCacheController, Log, All);

UGalaxySimulationCachePlayer::UGalaxySimulationCachePlayer() : AnimCacheStep(0), AnimationTime(0.0f)
{
}

UGalaxySimulationCachePlayer::~UGalaxySimulationCachePlayer()
{
	FWorldDelegates::OnWorldTickStart.RemoveAll(this);
	PlayDelegateHandle.Reset();
}

void UGalaxySimulationCachePlayer::SetSimulationCache(UFastMultipoleSimulationCache* InSimulationCache)
{
	if (UFastMultipoleSimulationCache* SimulationCache = SimulationCacheWeak.Get())
	{
		SimulationCache->OnReset.RemoveDynamic(this, &UGalaxySimulationCachePlayer::OnCacheReset);
		SimulationCache->OnFrameAdded.RemoveDynamic(this, &UGalaxySimulationCachePlayer::OnCacheFrameAdded);
	}

	SimulationCacheWeak = InSimulationCache;
	if (InSimulationCache)
	{
		InSimulationCache->OnReset.AddDynamic(this, &UGalaxySimulationCachePlayer::OnCacheReset);
		InSimulationCache->OnFrameAdded.AddDynamic(this, &UGalaxySimulationCachePlayer::OnCacheFrameAdded);
	}

	SetToFront();
}

void UGalaxySimulationCachePlayer::Play()
{
	PlayDelegateHandle = FWorldDelegates::OnWorldTickStart.AddUObject(this, &UGalaxySimulationCachePlayer::OnWorldTick_StepForward);
}

void UGalaxySimulationCachePlayer::Pause()
{
	FWorldDelegates::OnWorldTickStart.Remove(PlayDelegateHandle);
	PlayDelegateHandle.Reset();
}

bool UGalaxySimulationCachePlayer::IsPlaying() const
{
	return PlayDelegateHandle.IsValid();
}

void UGalaxySimulationCachePlayer::StepForward(float DeltaTime)
{
	UFastMultipoleSimulationCache* SimulationCache = SimulationCacheWeak.Get();
	if (!SimulationCache)
	{
		return;
	}

	const int32 NumFrames = SimulationCache->GetNumFrames();
	const int32 OldAnimCacheStep = AnimCacheStep;
	const float OldAnimationTime = AnimationTime;

	if (AnimCacheStep < NumFrames - 1)
	{
		AnimationTime += DeltaTime * AnimationSpeed;
		if (AnimationTime >= 1.0f)
		{
			AnimCacheStep += FMath::FloorToInt(AnimationTime);
			if (AnimCacheStep < NumFrames - 1)
			{
				AnimationTime = FMath::Frac(AnimationTime);
			}
			else
			{
				// Animation will stop until more frames are added
				AnimCacheStep = NumFrames - 1;
				AnimationTime = 0.0f;
			}
		}
	}
	else
	{
		AnimCacheStep = NumFrames - 1;
		AnimationTime = 0.0f;
	}

	if (AnimCacheStep != OldAnimCacheStep || AnimationTime != OldAnimationTime)
	{
		UpdateResultFrame(SimulationCache);
	}
}

void UGalaxySimulationCachePlayer::StepBack(float DeltaTime)
{
	UFastMultipoleSimulationCache* SimulationCache = SimulationCacheWeak.Get();
	if (!SimulationCache)
	{
		return;
	}

	const int32 NumFrames = SimulationCache->GetNumFrames();
	const int32 OldAnimCacheStep = AnimCacheStep;
	const float OldAnimationTime = AnimationTime;

	if (AnimCacheStep >= 0)
	{
		AnimationTime -= DeltaTime * AnimationSpeed;
		if (AnimationTime < 0.0f)
		{
			AnimCacheStep += FMath::FloorToInt(AnimationTime);
			if (AnimCacheStep >= 0)
			{
				AnimationTime = FMath::Frac(AnimationTime);
			}
			else
			{
				// Animation stop at front
				AnimCacheStep = 0;
				AnimationTime = 0.0f;
			}
		}
	}
	else
	{
		AnimCacheStep = 0;
		AnimationTime = 0.0f;
	}

	if (AnimCacheStep != OldAnimCacheStep || AnimationTime != OldAnimationTime)
	{
		UpdateResultFrame(SimulationCache);
	}
}

float UGalaxySimulationCachePlayer::GetTime() const
{
	return (float)AnimCacheStep + AnimationTime;
}

void UGalaxySimulationCachePlayer::SetTime(float Time)
{
	UFastMultipoleSimulationCache* SimulationCache = SimulationCacheWeak.Get();
	if (!SimulationCache)
	{
		return;
	}

	AnimCacheStep = FMath::FloorToInt(Time);
	AnimationTime = FMath::Frac(Time);

	UpdateResultFrame(SimulationCache);
}

void UGalaxySimulationCachePlayer::SetToFront()
{
	UFastMultipoleSimulationCache* SimulationCache = SimulationCacheWeak.Get();
	if (!SimulationCache)
	{
		return;
	}

	AnimCacheStep = 0;
	AnimationTime = 0.0f;

	UpdateResultFrame(SimulationCache);
}

void UGalaxySimulationCachePlayer::SetToBack()
{
	UFastMultipoleSimulationCache* SimulationCache = SimulationCacheWeak.Get();
	if (!SimulationCache)
	{
		return;
	}

	AnimCacheStep = SimulationCache->GetNumFrames() - 1;
	AnimationTime = 0.0f;

	UpdateResultFrame(SimulationCache);
}

void UGalaxySimulationCachePlayer::GetFrameInterval(
	const UFastMultipoleSimulationCache* SimulationCache, FFastMultipoleSimulationFrame::ConstPtr& OutStartFrame,
	FFastMultipoleSimulationFrame::ConstPtr& OutEndFrame) const
{
	check(SimulationCache);

	int32 NumFrames = SimulationCache->GetNumFrames();
	if (AnimCacheStep < 0)
	{
		OutStartFrame.Reset();
		OutEndFrame.Reset();
	}
	else if (AnimCacheStep < NumFrames - 1)
	{
		OutStartFrame = SimulationCache->GetFrame(AnimCacheStep);
		OutEndFrame = SimulationCache->GetFrame(AnimCacheStep + 1);
	}
	else if (AnimCacheStep < NumFrames)
	{
		OutStartFrame = SimulationCache->GetFrame(AnimCacheStep);
		OutEndFrame.Reset();
	}
	else
	{
		OutStartFrame.Reset();
		OutEndFrame.Reset();
	}
}

void UGalaxySimulationCachePlayer::UpdateResultFrame(const UFastMultipoleSimulationCache* SimulationCache)
{
	check(SimulationCache);

	FFastMultipoleSimulationFrame::ConstPtr StartFrame, EndFrame;
	GetFrameInterval(SimulationCache, StartFrame, EndFrame);
	if (StartFrame)
	{
		const TArray<FVector>& PosBegin = StartFrame->GetPositions();
		const TArray<FVector>& VelBegin = StartFrame->GetVelocities();
		const TArray<FVector>& ForceBegin = StartFrame->GetForces();

		if (EndFrame)
		{
			const TArray<FVector>& PosEnd = EndFrame->GetPositions();
			const TArray<FVector>& VelEnd = EndFrame->GetVelocities();
			const TArray<FVector>& ForceEnd = EndFrame->GetForces();

			int32 NumPoints = FMath::Min(StartFrame->GetNumPoints(), EndFrame->GetNumPoints());
			ResultFrame.SetNumPoints(NumPoints);
			for (int32 i = 0; i < NumPoints; ++i)
			{
				ResultFrame.SetPoint(
					i, FMath::Lerp(PosBegin[i], PosEnd[i], AnimationTime), FMath::Lerp(VelBegin[i], VelEnd[i], AnimationTime),
					FMath::Lerp(ForceBegin[i], ForceEnd[i], AnimationTime));
			}
		}
		else
		{
			int32 NumPoints = StartFrame->GetNumPoints();
			ResultFrame.SetNumPoints(NumPoints);
			for (int32 i = 0; i < NumPoints; ++i)
			{
				ResultFrame.SetPoint(i, PosBegin[i], VelBegin[i], ForceBegin[i]);
			}
		}
	}
	else
	{
		ResultFrame.Empty();
	}
}

void UGalaxySimulationCachePlayer::OnWorldTick_StepForward(UWorld* World, ELevelTick TickType, float DeltaTime)
{
	StepForward(DeltaTime);
}

void UGalaxySimulationCachePlayer::OnCacheReset(UFastMultipoleSimulationCache* InSimulationCache)
{
	SetToFront();
}

void UGalaxySimulationCachePlayer::OnCacheFrameAdded(UFastMultipoleSimulationCache* InSimulationCache)
{
}
