// Fill out your copyright notice in the Description page of Project Settings.

#include "FastMultipoleSimulationThreadRunnable.h"

#include "Async/Async.h"

DEFINE_LOG_CATEGORY_STATIC(LogFastMultipoleThread, Log, All);

FFastMultipoleSimulationThreadRunnable::FFastMultipoleSimulationThreadRunnable()
	: Simulation(nullptr)
	, DeltaTime(0.0f)
	, MaxCompletedSteps(3)
	, bIsRunning(false)
	, bStopRequested(false)
{
	WorkEvent = FPlatformProcess::GetSynchEventFromPool();
	check(WorkEvent != nullptr);
}

FFastMultipoleSimulationThreadRunnable::~FFastMultipoleSimulationThreadRunnable()
{
	FPlatformProcess::ReturnSynchEventToPool(WorkEvent);
	WorkEvent = nullptr;
}

void FFastMultipoleSimulationThreadRunnable::SetMaxCompletedSteps(int32 InMaxCompletedSteps)
{
	check(!bIsRunning);
	MaxCompletedSteps = InMaxCompletedSteps;

	int32 Count = CompletedStepsCount.GetValue();
	if (Count < MaxCompletedSteps)
	{
		WorkEvent->Trigger();
	}
	else if (Count > MaxCompletedSteps)
	{
		UE_LOG(LogFastMultipoleThread, Warning, TEXT("Completed steps count %d exceeds new max. count %d"), Count, MaxCompletedSteps);
	}
}

bool FFastMultipoleSimulationThreadRunnable::PopCompletedStep(FFastMultipoleSimulationStepResult& Result)
{
	if (CompletedSteps.Dequeue(Result))
	{
		int32 Count = CompletedStepsCount.Decrement();

		if (Count < MaxCompletedSteps)
		{
			WorkEvent->Trigger();
		}

		return true;
	}

	return false;
}

void FFastMultipoleSimulationThreadRunnable::Stop()
{
	bStopRequested = true;
	WorkEvent->Trigger();
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

	while (!Simulation)
	{
		WorkEvent->Wait();
	}

LoopStart:
	while (!bStopRequested)
	{
#if 0
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
#endif

		if (CompletedStepsCount.GetValue() >= MaxCompletedSteps)
		{
			WorkEvent->Wait();
			goto LoopStart;
		}

		FFastMultipoleSimulationStepResult Result;
		if (Simulation->Step(bStopRequested, DeltaTime, Result))
		{
			CompletedSteps.Enqueue(Result);
			CompletedStepsCount.Increment();
		}
	}

	// WorkerThreadPool = nullptr;

	return 0;
}

void FFastMultipoleSimulationThreadRunnable::LaunchThread()
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

void FFastMultipoleSimulationThreadRunnable::StopThread()
{
	if (bIsRunning)
	{
		// Calling Reset will call Kill which in turn will call Stop and set bStopRequested to true.
		Thread.Reset();
	}

	Simulation.Reset();
}

void FFastMultipoleSimulationThreadRunnable::StartSimulation(FFastMultipoleSimulationFrame::ConstPtr InStartFrame, int32 InStepIndex,
	float InDeltaTime)
{
	DeltaTime = InDeltaTime;

	Simulation = MakeUnique<FFastMultipoleSimulation>();
	Simulation->Reset(InStartFrame, 0);

	WorkEvent->Trigger();
}
