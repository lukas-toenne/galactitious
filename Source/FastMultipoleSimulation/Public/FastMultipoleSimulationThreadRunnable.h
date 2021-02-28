// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FastMultipoleSimulation.h"

#include "Containers/CircularQueue.h"

class FFastMultipoleSimulation;

struct FASTMULTIPOLESIMULATION_API FFastMultipoleSimulationThreadRunnable : public FRunnable
{
public:
	FFastMultipoleSimulationThreadRunnable();
	virtual ~FFastMultipoleSimulationThreadRunnable();

	void ScheduleStep();
	void CancelScheduledSteps();
	int32 GetNumScheduledSteps() const;

	void LaunchThread();
	void StopThread();
	inline bool IsRunning() { return bIsRunning; }

	void StartSimulation(
		const FFastMultipoleSimulationSettings& Settings, FFastMultipoleSimulationInvariants::ConstPtr Invariants,
		FFastMultipoleSimulationFrame::Ptr StartFrame, UWorld* DebugWorld = nullptr);
	bool PopCompletedStep(FFastMultipoleSimulationStepResult& Result);

protected:
	// FRunnable interface.
	virtual bool Init() override;
	virtual void Exit() override;
	virtual uint32 Run() override;
	virtual void Stop() override;

private:
	TUniquePtr<FFastMultipoleSimulation> Simulation;

	// Steps to compute
	TQueue<FFastMultipoleSimulationStepRequest, EQueueMode::Spsc> ScheduledSteps;
	FThreadSafeCounter NumScheduledSteps;
	// Queue of completed simulation steps.
	TQueue<FFastMultipoleSimulationStepResult, EQueueMode::Spsc> CompletedSteps;

	TUniquePtr<FRunnableThread> Thread;
	// TUniquePtr<FQueuedThreadPool> WorkerThreadPool;

	FThreadSafeBool bIsRunning;
	FThreadSafeBool bStopRequested;

	/** Event signalling that simulation can continue. */
	FEvent* WorkEvent;
};
