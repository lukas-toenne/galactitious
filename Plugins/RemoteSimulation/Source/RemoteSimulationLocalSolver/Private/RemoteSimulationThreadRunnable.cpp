// Fill out your copyright notice in the Description page of Project Settings.

#include "RemoteSimulationThreadRunnable.h"

#include "Async/Async.h"

DEFINE_LOG_CATEGORY_STATIC(LogRemoteSimulationLocalSolverThread, Log, All);

FRemoteSimulationThreadRunnable::FRemoteSimulationThreadRunnable()
	: Simulation(nullptr)
	, NumScheduledSteps(0)
	, bIsRunning(false)
	, bStopRequested(false)
{
	WorkEvent = FPlatformProcess::GetSynchEventFromPool();
	check(WorkEvent != nullptr);
}

FRemoteSimulationThreadRunnable::~FRemoteSimulationThreadRunnable()
{
	Thread.Reset();
	FPlatformProcess::ReturnSynchEventToPool(WorkEvent);
	WorkEvent = nullptr;
}

void FRemoteSimulationThreadRunnable::ScheduleStep()
{
	FRemoteSimulationSolverStepRequest Request;
	ScheduledSteps.Enqueue(MoveTemp(Request));
	NumScheduledSteps.Increment();

	WorkEvent->Trigger();
}

void FRemoteSimulationThreadRunnable::CancelScheduledSteps()
{
	if (!ScheduledSteps.IsEmpty())
	{
		ScheduledSteps.Empty();
		NumScheduledSteps.Reset();

		WorkEvent->Trigger();
	}
}

int32 FRemoteSimulationThreadRunnable::GetNumScheduledSteps() const
{
	return NumScheduledSteps.GetValue();
}

bool FRemoteSimulationThreadRunnable::PopCompletedStep(FRemoteSimulationStepResult& Result)
{
	return CompletedSteps.Dequeue(Result);
}

void FRemoteSimulationThreadRunnable::Stop()
{
	bStopRequested = true;
	CancelScheduledSteps();
	WorkEvent->Trigger();
}

bool FRemoteSimulationThreadRunnable::Init()
{
	return true;
}

void FRemoteSimulationThreadRunnable::Exit()
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

uint32 FRemoteSimulationThreadRunnable::Run()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FRemoteSimulationThreadRunnable::Run)

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

		FRemoteSimulationSolverStepRequest Request;
		if (ScheduledSteps.Dequeue(Request))
		{
			NumScheduledSteps.Decrement();

			FRemoteSimulationStepResult Result;
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

void FRemoteSimulationThreadRunnable::LaunchThread()
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

void FRemoteSimulationThreadRunnable::StopThread()
{
	if (bIsRunning)
	{
		// Calling Reset will call Kill which in turn will call Stop and set bStopRequested to true.
		Thread.Reset();
	}

	Simulation.Reset();
}

void FRemoteSimulationThreadRunnable::StartSimulation(
	const FRemoteSimulationSolverSettings& Settings, FRemoteSimulationInvariants::ConstPtr Invariants,
	FRemoteSimulationFrame::Ptr StartFrame, UWorld* InDebugWorld)
{
	Simulation = MakeUnique<FRemoteSimulationLocalSolver>(Settings);
	Simulation->SetDebugWorld(InDebugWorld);
	Simulation->Reset(Invariants, StartFrame);

	WorkEvent->Trigger();
}
