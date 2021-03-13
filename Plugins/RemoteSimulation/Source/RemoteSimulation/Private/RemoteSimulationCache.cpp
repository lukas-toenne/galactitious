// Fill out your copyright notice in the Description page of Project Settings.

#include "RemoteSimulationCache.h"

#define LOCTEXT_NAMESPACE "RemoteSimulation"

URemoteSimulationCache::URemoteSimulationCache()
{
}

URemoteSimulationCache::~URemoteSimulationCache()
{
}

int32 URemoteSimulationCache::GetCapacity() const
{
	return Capacity;
}

void URemoteSimulationCache::SetCapacity(int32 InCapacity, ERemoteSimulationCacheResizeMode ResizeMode)
{
	using FrameArray = TArray<FRemoteSimulationFrame::ConstPtr>;

	FRWScopeLock Lock(FramesMutex, SLT_Write);

	Capacity = InCapacity;
	if (Frames.Num() > Capacity)
	{
		switch (ResizeMode)
		{
		case ERemoteSimulationCacheResizeMode::None:
			break;
		case ERemoteSimulationCacheResizeMode::PruneStart:
			Frames = FrameArray(Frames.GetData() + (Frames.Num() - Capacity), Capacity);
			break;
		case ERemoteSimulationCacheResizeMode::PruneEnd:
			Frames = FrameArray(Frames.GetData(), Capacity);
			break;
		case ERemoteSimulationCacheResizeMode::Empty:
			Frames.Empty();
			break;
		}

		FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady(
			[&]() { OnReset.Broadcast(this); }, TStatId(), nullptr, ENamedThreads::GameThread);
	}
}

int32 URemoteSimulationCache::GetNumFrames() const
{
	FRWScopeLock Lock(FramesMutex, SLT_ReadOnly);
	return Frames.Num();
}

FRemoteSimulationFrame::ConstPtr URemoteSimulationCache::GetFrame(int32 Step) const
{
	FRWScopeLock Lock(FramesMutex, SLT_ReadOnly);
	if (Frames.IsValidIndex(Step))
	{
		return Frames[Step];
	}
	return nullptr;
}

FRemoteSimulationFrame::ConstPtr URemoteSimulationCache::GetLastFrame() const
{
	FRWScopeLock Lock(FramesMutex, SLT_ReadOnly);
	if (Frames.Num() > 0)
	{
		return Frames.Last();
	}
	return nullptr;
}

void URemoteSimulationCache::Reset()
{
	{
		FRWScopeLock Lock(FramesMutex, SLT_Write);
		Frames.Empty();
	}

	FGraphEventRef Task =
		FFunctionGraphTask::CreateAndDispatchWhenReady([&]() { OnReset.Broadcast(this); }, TStatId(), nullptr, ENamedThreads::GameThread);
}

bool URemoteSimulationCache::AddFrame(const FRemoteSimulationFrame::ConstPtr& InFrame)
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
