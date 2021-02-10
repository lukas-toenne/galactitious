// Fill out your copyright notice in the Description page of Project Settings.

#include "GalaxySimulationCachePlayer.h"

#include "FastMultipoleSimulationCache.h"

DEFINE_LOG_CATEGORY_STATIC(LogGalaxySimulationCacheController, Log, All);

FGalaxySimulationCachePlayer::FGalaxySimulationCachePlayer()
{
	ResetAnimation(nullptr);
}

void FGalaxySimulationCachePlayer::ResetAnimation(const UFastMultipoleSimulationCache* SimulationCache)
{
	AnimCacheStep = 0;
	AnimationTime = 0.0f;

	UpdateResultFrame(SimulationCache);
}

void FGalaxySimulationCachePlayer::StepAnimation(const UFastMultipoleSimulationCache* SimulationCache, float DeltaTime)
{
	if (!SimulationCache)
	{
		ResetAnimation(nullptr);
		return;
	}

	const int32 NumFrames = SimulationCache->GetNumFrames();
	const int32 OldAnimCacheStep = AnimCacheStep;

	if (AnimCacheStep >= 0 && AnimCacheStep < NumFrames - 1)
	{
		AnimationTime += DeltaTime * AnimationSpeed;
		if (AnimationTime >= 1.0f)
		{
			AnimCacheStep += (int32)AnimationTime;
			if (AnimCacheStep < NumFrames)
			{
				AnimationTime = FMath::Fmod(AnimationTime, 1.0f);
			}
			else
			{
				// Animation will stop until more frames are added
				AnimCacheStep = NumFrames - 1;
				AnimationTime = 1.0f;
			}
		}

		UpdateResultFrame(SimulationCache);
	}
}

void FGalaxySimulationCachePlayer::GetFrameInterval(
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

void FGalaxySimulationCachePlayer::UpdateResultFrame(const UFastMultipoleSimulationCache* SimulationCache)
{
	if (!SimulationCache)
	{
		ResultFrame.Empty();
		return;
	}

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
