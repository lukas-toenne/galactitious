// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "Containers/CircularQueue.h"

class FRemoteSimulationLocalSolver;

struct REMOTESIMULATIONLOCALSOLVER_API FRemoteSimulationThreadRunnable : public FRunnable
{
public:
	FRemoteSimulationThreadRunnable();
	virtual ~FRemoteSimulationThreadRunnable();

	void ScheduleStep();
	void CancelScheduledSteps();
	int32 GetNumScheduledSteps() const;

	void LaunchThread();
	void StopThread();
	inline bool IsRunning() { return bIsRunning; }

	void StartSimulation(
		const FRemoteSimulationSolverSettings& Settings, FRemoteSimulationInvariants::ConstPtr Invariants,
		FRemoteSimulationFrame::Ptr StartFrame, UWorld* DebugWorld = nullptr);
	bool PopCompletedStep(FRemoteSimulationStepResult& Result);

protected:
	// FRunnable interface.
	virtual bool Init() override;
	virtual void Exit() override;
	virtual uint32 Run() override;
	virtual void Stop() override;

private:
	TUniquePtr<FRemoteSimulationLocalSolver> Simulation;

	// Steps to compute
	TQueue<FRemoteSimulationSolverStepRequest, EQueueMode::Spsc> ScheduledSteps;
	FThreadSafeCounter NumScheduledSteps;
	// Queue of completed simulation steps.
	TQueue<FRemoteSimulationStepResult, EQueueMode::Spsc> CompletedSteps;

	TUniquePtr<FRunnableThread> Thread;
	// TUniquePtr<FQueuedThreadPool> WorkerThreadPool;

	FThreadSafeBool bIsRunning;
	FThreadSafeBool bStopRequested;

	/** Event signalling that simulation can continue. */
	FEvent* WorkEvent;
};
