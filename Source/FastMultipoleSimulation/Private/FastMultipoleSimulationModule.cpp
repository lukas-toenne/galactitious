// Fill out your copyright notice in the Description page of Project Settings.

#include "FastMultipoleSimulationModule.h"

#include "CoreMinimal.h"
#include "FastMultipoleSimulationCache.h"
#include "FastMultipoleSimulationThreadRunnable.h"

#include "Modules/ModuleManager.h"

FFastMultipoleSimulationModule* FFastMultipoleSimulationModule::Singleton = nullptr;

FFastMultipoleSimulationModule& FFastMultipoleSimulationModule::Get()
{
	if (Singleton == nullptr)
	{
		check(IsInGameThread());
		FModuleManager::LoadModuleChecked<FFastMultipoleSimulationModule>("FastMultipoleSimulation");
	}
	check(Singleton != nullptr);
	return *Singleton;
}

int32 FFastMultipoleSimulationModule::CacheCompletedSteps(UFastMultipoleSimulationCache* SimulationCache)
{
	int32 NumCompletedSteps = 0;
	FFastMultipoleSimulationStepResult StepResult;
	while (ThreadRunnable->PopCompletedStep(StepResult))
	{
		SimulationCache->AddFrame(StepResult.Frame);
		++NumCompletedSteps;
	}
	return NumCompletedSteps;
}

int32 FFastMultipoleSimulationModule::GetNumScheduledSteps() const
{
	return ThreadRunnable->GetNumScheduledSteps();
}

int32 FFastMultipoleSimulationModule::ScheduleSteps(int32 MaxStepsScheduled)
{
	const int32 NumStepsToSchedule = FMath::Max(MaxStepsScheduled - ThreadRunnable->GetNumScheduledSteps(), 0);
	for (int i = 0; i < NumStepsToSchedule; ++i)
	{
		ThreadRunnable->ScheduleStep();
	}
	return NumStepsToSchedule;
}

void FFastMultipoleSimulationModule::CancelScheduledSteps()
{
	ThreadRunnable->CancelScheduledSteps();
}

void FFastMultipoleSimulationModule::StartSimulation(
	const FFastMultipoleSimulationSettings& Settings, FFastMultipoleSimulationInvariants::ConstPtr Invariants,
	FFastMultipoleSimulationFrame::Ptr StartFrame, UWorld* DebugWorld)
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

void FFastMultipoleSimulationModule::StopSimulation()
{
	ThreadRunnable->StopThread();
}

void FFastMultipoleSimulationModule::StartupModule()
{
	Singleton = this;

	ThreadRunnable = MakeUnique<FFastMultipoleSimulationThreadRunnable>();
}

void FFastMultipoleSimulationModule::PostLoadCallback()
{
}

void FFastMultipoleSimulationModule::PreUnloadCallback()
{
}

void FFastMultipoleSimulationModule::ShutdownModule()
{
	ThreadRunnable.Reset();
	Singleton = nullptr;
}

IMPLEMENT_MODULE(FFastMultipoleSimulationModule, FastMultipoleSimulation);
