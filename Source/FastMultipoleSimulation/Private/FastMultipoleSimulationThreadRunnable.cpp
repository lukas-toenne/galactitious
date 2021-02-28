// Fill out your copyright notice in the Description page of Project Settings.

#include "FastMultipoleSimulationThreadRunnable.h"

#include "Async/Async.h"

DEFINE_LOG_CATEGORY_STATIC(LogFastMultipoleThread, Log, All);

FFastMultipoleSimulationThreadRunnable::FFastMultipoleSimulationThreadRunnable()
	: Simulation(nullptr)
	, NumScheduledSteps(0)
	, bIsRunning(false)
	, bStopRequested(false)
{
	WorkEvent = FPlatformProcess::GetSynchEventFromPool();
	check(WorkEvent != nullptr);
}

FFastMultipoleSimulationThreadRunnable::~FFastMultipoleSimulationThreadRunnable()
{
	Thread.Reset();
	FPlatformProcess::ReturnSynchEventToPool(WorkEvent);
	WorkEvent = nullptr;
}

void FFastMultipoleSimulationThreadRunnable::ScheduleStep()
{
	FFastMultipoleSimulationStepRequest Request;
	ScheduledSteps.Enqueue(MoveTemp(Request));
	NumScheduledSteps.Increment();

	WorkEvent->Trigger();
}

void FFastMultipoleSimulationThreadRunnable::CancelScheduledSteps()
{
	if (!ScheduledSteps.IsEmpty())
	{
		ScheduledSteps.Empty();
		NumScheduledSteps.Reset();

		WorkEvent->Trigger();
	}
}

int32 FFastMultipoleSimulationThreadRunnable::GetNumScheduledSteps() const
{
	return NumScheduledSteps.GetValue();
}

bool FFastMultipoleSimulationThreadRunnable::PopCompletedStep(FFastMultipoleSimulationStepResult& Result)
{
	return CompletedSteps.Dequeue(Result);
}

void FFastMultipoleSimulationThreadRunnable::Stop()
{
	bStopRequested = true;
	CancelScheduledSteps();
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

	// LoopStart:
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

		FFastMultipoleSimulationStepRequest Request;
		if (ScheduledSteps.Dequeue(Request))
		{
			NumScheduledSteps.Decrement();

			FFastMultipoleSimulationStepResult Result;
			if (Simulation->Step(bStopRequested, Result))
			{
				CompletedSteps.Enqueue(Result);
			}
		}
		else
		{
			WorkEvent->Wait();
			// goto LoopStart;
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

void FFastMultipoleSimulationThreadRunnable::StartSimulation(
	const FFastMultipoleSimulationSettings& Settings, FFastMultipoleSimulationInvariants::ConstPtr Invariants,
	FFastMultipoleSimulationFrame::Ptr StartFrame, UWorld* InDebugWorld)
{
	Simulation = MakeUnique<FFastMultipoleSimulation>(Settings);
	Simulation->SetDebugWorld(InDebugWorld);
	Simulation->Reset(Invariants, StartFrame);

	WorkEvent->Trigger();
}
