// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "Containers/Queue.h"

struct FASTMULTIPOLESIMULATION_API FGalaxySimulationStepResult
{
	int32 StepIndex;
};

struct FASTMULTIPOLESIMULATION_API FFastMultipoleSimulationThreadRunnable : public FRunnable
{
public:
	FFastMultipoleSimulationThreadRunnable();
	virtual ~FFastMultipoleSimulationThreadRunnable();

	void Launch();

	inline bool IsRunning() { return bIsRunning; }

	virtual void Stop() override;

protected:
	// FRunnable interface.
	virtual bool Init() override;
	virtual void Exit() override;
	virtual uint32 Run() override;

private:
	float DeltaTime;
	TQueue<FGalaxySimulationStepResult> CompletedSteps;

	TUniquePtr<FRunnableThread> Thread;
	TUniquePtr<FQueuedThreadPool> WorkerThreadPool;

	volatile bool bIsRunning;
	volatile bool bStopRequested;
};
