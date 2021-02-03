// Fill out your copyright notice in the Description page of Project Settings.

#include "FastMultipoleSimulationThreadRunnable.h"

#include "FastMultipoleSimulation.h"

#include "Async/Async.h"

FFastMultipoleSimulationThreadRunnable::FFastMultipoleSimulationThreadRunnable() : bIsRunning(false), bStopRequested(false)
{
}

FFastMultipoleSimulationThreadRunnable::~FFastMultipoleSimulationThreadRunnable()
{
}

void FFastMultipoleSimulationThreadRunnable::Stop()
{
	bStopRequested = true;
}

bool FFastMultipoleSimulationThreadRunnable::Init()
{
	return true;
}

void FFastMultipoleSimulationThreadRunnable::Exit()
{
	bIsRunning = false;
}

static FQueuedThreadPool* CreateWorkerThreadPool()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(CreateWorkerThreadPool)
	const int32 NumThreads = FMath::Max<int32>(FPlatformMisc::NumberOfCoresIncludingHyperthreads() - 2, 1);
	FQueuedThreadPool* WorkerThreadPool = FQueuedThreadPool::Allocate();
	WorkerThreadPool->Create(NumThreads, 32 * 1024, TPri_BelowNormal);
	return WorkerThreadPool;
}

uint32 FFastMultipoleSimulationThreadRunnable::Run()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FFastMultipoleSimulationThreadRunnable::Run)

	while (!bStopRequested)
	{
		//// LIFO build order, since meshes actually visible in a map are typically loaded last
		//FAsyncDistanceFieldTask* Task = AsyncQueue.TaskQueue.Pop();

		FQueuedThreadPool* ThreadPool = nullptr;

#if WITH_EDITOR
		ThreadPool = GLargeThreadPool;
#endif

		//if (Task)
		{
			if (!ThreadPool)
			{
				if (!WorkerThreadPool)
				{
					WorkerThreadPool.Reset(CreateWorkerThreadPool());
				}

				ThreadPool = WorkerThreadPool.Get();
			}

			//AsyncQueue.Build(Task, *ThreadPool);
		}
	}

	WorkerThreadPool = nullptr;

	return 0;
}

void FFastMultipoleSimulationThreadRunnable::Launch()
{
	check(!bIsRunning);

	// Calling Reset will call Kill which in turn will call Stop and set bStopRequested to true.
	Thread.Reset();

	// Now we can set bStopRequested to false without being overwritten by the old thread shutting down.
	bStopRequested = false;
	Thread.Reset(
		FRunnableThread::Create(this, TEXT("Fast Multipole Simulation Thread"), 0, TPri_Normal, FPlatformAffinity::GetPoolThreadMask()));

	// Set this now before exiting so that IsRunning() returns true without having to wait on the thread to be completely started.
	bIsRunning = true;
}
