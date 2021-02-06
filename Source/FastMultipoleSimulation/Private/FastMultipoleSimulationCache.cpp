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

void UFastMultipoleSimulationCache::Reset()
{
	{
		FRWScopeLock Lock(FramesMutex, SLT_Write);
		Frames.Empty();
	}

	FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady(
		[&]() { OnSimulationReset.Broadcast(this); }, TStatId(), nullptr, ENamedThreads::GameThread);
}

bool UFastMultipoleSimulationCache::AddFrame(const FFastMultipoleSimulationFramePtr& InFrame)
{
	{
		FRWScopeLock Lock(FramesMutex, SLT_Write);
		Frames.Add(InFrame);
	}

	FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady(
		[&]() { OnSimulationStep.Broadcast(this); }, TStatId(), nullptr, ENamedThreads::GameThread);

	return true;
}

#undef LOCTEXT_NAMESPACE
