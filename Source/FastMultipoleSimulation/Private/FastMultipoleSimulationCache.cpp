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

int32 UFastMultipoleSimulationCache::GetCapacity() const
{
	return Capacity;
}

void UFastMultipoleSimulationCache::SetCapacity(int32 InCapacity, EFastMultipoleCacheResizeMode ResizeMode)
{
	using FrameArray = TArray<FFastMultipoleSimulationFrame::ConstPtr>;

	FRWScopeLock Lock(FramesMutex, SLT_Write);

	Capacity = InCapacity;
	if (Frames.Num() > Capacity)
	{
		switch (ResizeMode)
		{
		case EFastMultipoleCacheResizeMode::None:
			break;
		case EFastMultipoleCacheResizeMode::PruneStart:
			Frames = FrameArray(Frames.GetData() + (Frames.Num() - Capacity), Capacity);
			break;
		case EFastMultipoleCacheResizeMode::PruneEnd:
			Frames = FrameArray(Frames.GetData(), Capacity);
			break;
		case EFastMultipoleCacheResizeMode::Empty:
			Frames.Empty();
			break;
		}

		FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady(
			[&]() { OnReset.Broadcast(this); }, TStatId(), nullptr, ENamedThreads::GameThread);
	}
}

int32 UFastMultipoleSimulationCache::GetNumFrames() const
{
	FRWScopeLock Lock(FramesMutex, SLT_ReadOnly);
	return Frames.Num();
}

FFastMultipoleSimulationFrame::ConstPtr UFastMultipoleSimulationCache::GetFrame(int32 Step) const
{
	FRWScopeLock Lock(FramesMutex, SLT_ReadOnly);
	if (Frames.IsValidIndex(Step))
	{
		return Frames[Step];
	}
	return nullptr;
}

FFastMultipoleSimulationFrame::ConstPtr UFastMultipoleSimulationCache::GetLastFrame() const
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

	FGraphEventRef Task =
		FFunctionGraphTask::CreateAndDispatchWhenReady([&]() { OnReset.Broadcast(this); }, TStatId(), nullptr, ENamedThreads::GameThread);
}

bool UFastMultipoleSimulationCache::AddFrame(const FFastMultipoleSimulationFrame::ConstPtr& InFrame)
{
	FRWScopeLock Lock(FramesMutex, SLT_Write);

	if (Frames.Num() >= Capacity)
	{
		return false;
	}

	Frames.Add(InFrame);

	FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady(
		[&]() { OnFrameAdded.Broadcast(this); }, TStatId(), nullptr, ENamedThreads::GameThread);

	return true;
}

#undef LOCTEXT_NAMESPACE
