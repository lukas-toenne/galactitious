// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FastMultipoleSimulation.h"

#include "Containers/CircularQueue.h"

struct FASTMULTIPOLESIMULATION_API FFastMultipoleSimulationThreadRunnable : public FRunnable
{
public:
	FFastMultipoleSimulationThreadRunnable();
	virtual ~FFastMultipoleSimulationThreadRunnable();

	int32 GetMaxCompletedSteps() const { return MaxCompletedSteps; }
	void SetMaxCompletedSteps(int32 MaxCompletedSteps);

	void LaunchThread();
	void StopThread();
	inline bool IsRunning() { return bIsRunning; }

	void StartSimulation(FFastMultipoleSimulationFrame::ConstPtr StartFrame, int32 StepIndex, float DeltaTime);
	bool PopCompletedStep(FFastMultipoleSimulationStepResult& Result);

protected:
	// FRunnable interface.
	virtual bool Init() override;
	virtual void Exit() override;
	virtual uint32 Run() override;
	virtual void Stop() override;

private:
	TUniquePtr<class FFastMultipoleSimulation> Simulation;
	float DeltaTime;

	// Maximum number of steps that can be computed in advance.
	int32 MaxCompletedSteps;
	// Queue of completed simulation steps.
	// When capacity is reached completed steps need to be consumed by calling PopCompletedStep
	// for the simulation to continue.
	TQueue<FFastMultipoleSimulationStepResult> CompletedSteps;
	FThreadSafeCounter CompletedStepsCount;

	TUniquePtr<FRunnableThread> Thread;
	// TUniquePtr<FQueuedThreadPool> WorkerThreadPool;

	FThreadSafeBool bIsRunning;
	FThreadSafeBool bStopRequested;

	/** Event signalling that simulation can continue. */
	FEvent* WorkEvent;
};
