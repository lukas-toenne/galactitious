// Fill out your copyright notice in the Description page of Project Settings.

#include "FastMultipoleSimulationCache.h"

#define LOCTEXT_NAMESPACE "FastMultipole"
DEFINE_LOG_CATEGORY(LogFastMultipoleSimulationCache)

UFastMultipoleSimulationCache::UFastMultipoleSimulationCache()
{
}

UFastMultipoleSimulationCache::~UFastMultipoleSimulationCache()
{
}

void UFastMultipoleSimulationCache::Reset(TArray<FVector>& InInitialPositions, TArray<FVector>& InInitialVelocities)
{
	FRWScopeLock Lock(FramesMutex, SLT_Write);

	Frames.SetNum(1);

	if (InInitialPositions.Num() != InInitialVelocities.Num())
	{
		UE_LOG(
			LogFastMultipoleSimulationCache, Error, TEXT("Input arrays must have same size (positions: %d, velocities: %d)"),
			InInitialPositions.Num(), InInitialVelocities.Num());
		return;
	}

	Frames[0] = MakeShared<FFastMultipoleSimulationFrame, ESPMode::ThreadSafe>();
	Frames[0]->Positions = MoveTemp(InInitialPositions);
	Frames[0]->Velocities = MoveTemp(InInitialVelocities);

	FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady(
		[&]() { OnSimulationReset.Broadcast(this); }, TStatId(), nullptr, ENamedThreads::GameThread);
}

void UFastMultipoleSimulationCache::Clear()
{
	FRWScopeLock Lock(FramesMutex, SLT_Write);
	Frames.Empty();
}

int32 UFastMultipoleSimulationCache::GetNumFrames() const
{
	FRWScopeLock Lock(FramesMutex, SLT_ReadOnly);
	return Frames.Num();
}

FFastMultipoleSimulationFramePtr UFastMultipoleSimulationCache::GetFrame(int32 Step) const
{
	FRWScopeLock Lock(FramesMutex, SLT_ReadOnly);
	if (Frames.IsValidIndex(Step))
	{
		return Frames[Step];
	}
	return nullptr;
}

FFastMultipoleSimulationFramePtr UFastMultipoleSimulationCache::GetLastFrame() const
{
	FRWScopeLock Lock(FramesMutex, SLT_ReadOnly);
	if (Frames.Num() > 0)
	{
		return Frames.Last();
	}
	return nullptr;
}

#undef LOCTEXT_NAMESPACE
