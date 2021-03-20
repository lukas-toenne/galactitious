// Fill out your copyright notice in the Description page of Project Settings.

#include "RemoteSimulationLocalSolverModule.h"

#include "CoreMinimal.h"
#include "RemoteSimulationCache.h"
#include "RemoteSimulationThreadRunnable.h"

#include "Modules/ModuleManager.h"

FRemoteSimulationLocalSolverModule* FRemoteSimulationLocalSolverModule::Singleton = nullptr;

FRemoteSimulationLocalSolverModule& FRemoteSimulationLocalSolverModule::Get()
{
	if (Singleton == nullptr)
	{
		check(IsInGameThread());
		FModuleManager::LoadModuleChecked<FRemoteSimulationLocalSolverModule>("RemoteSimulationLocalSolver");
	}
	check(Singleton != nullptr);
	return *Singleton;
}

int32 FRemoteSimulationLocalSolverModule::CacheCompletedSteps(URemoteSimulationCache* SimulationCache)
{
	int32 NumCompletedSteps = 0;
	FRemoteSimulationStepResult StepResult;
	while (ThreadRunnable->PopCompletedStep(StepResult))
	{
		SimulationCache->AddFrame(StepResult.Frame);
		++NumCompletedSteps;
	}
	return NumCompletedSteps;
}

int32 FRemoteSimulationLocalSolverModule::GetNumScheduledSteps() const
{
	return ThreadRunnable->GetNumScheduledSteps();
}

int32 FRemoteSimulationLocalSolverModule::ScheduleSteps(int32 MaxStepsScheduled)
{
	const int32 NumStepsToSchedule = FMath::Max(MaxStepsScheduled - ThreadRunnable->GetNumScheduledSteps(), 0);
	for (int i = 0; i < NumStepsToSchedule; ++i)
	{
		ThreadRunnable->ScheduleStep();
	}
	return NumStepsToSchedule;
}

void FRemoteSimulationLocalSolverModule::CancelScheduledSteps()
{
	ThreadRunnable->CancelScheduledSteps();
}

void FRemoteSimulationLocalSolverModule::StartSimulation(
	const FRemoteSimulationSolverSettings& Settings, FRemoteSimulationInvariants::ConstPtr Invariants,
	FRemoteSimulationFrame::Ptr StartFrame, UWorld* DebugWorld)
{
	if (ThreadRunnable->IsRunning())
	{
		ThreadRunnable->StopThread();
	}
	ThreadRunnable->LaunchThread();

	if (Invariants && StartFrame)
	{
		ThreadRunnable->StartSimulation(Settings, Invariants, StartFrame, DebugWorld);
	}
}

void FRemoteSimulationLocalSolverModule::StopSimulation()
{
	ThreadRunnable->StopThread();
}

void FRemoteSimulationLocalSolverModule::StartupModule()
{
	Singleton = this;

	ThreadRunnable = MakeUnique<FRemoteSimulationThreadRunnable>();
}

void FRemoteSimulationLocalSolverModule::PostLoadCallback()
{
}

void FRemoteSimulationLocalSolverModule::PreUnloadCallback()
{
}

void FRemoteSimulationLocalSolverModule::ShutdownModule()
{
	ThreadRunnable.Reset();
	Singleton = nullptr;
}

IMPLEMENT_MODULE(FRemoteSimulationLocalSolverModule, RemoteSimulationLocalSolver);
