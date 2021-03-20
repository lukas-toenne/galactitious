// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RemoteSimulationFrame.h"
#include "RemoteSImulationLocalSolverTypes.h"

struct REMOTESIMULATIONLOCALSOLVER_API FRemoteSimulationSolverStepRequest
{
	int32 Dummy;
};

struct REMOTESIMULATIONLOCALSOLVER_API FRemoteSimulationStepResult
{
	ERemoteSimulationResultStatus Status;
	FRemoteSimulationFrame::Ptr Frame;
};

class REMOTESIMULATIONLOCALSOLVER_API FRemoteSimulationLocalSolver
{
public:
	static void Init();
	static void Shutdown();

	FRemoteSimulationLocalSolver(const FRemoteSimulationSolverSettings& Settings);
	~FRemoteSimulationLocalSolver();

	void SetDebugWorld(UWorld* InDebugWorld);

	void Reset(FRemoteSimulationInvariants::ConstPtr Invariants, FRemoteSimulationFrame::Ptr StartFrame);
	bool Step(FThreadSafeBool& bStopRequested, FRemoteSimulationStepResult& Result);

	FRemoteSimulationFrame::ConstPtr GetCurrentFrame() const { return CurrentFrame; }

protected:
	void Integrate();

	void ComputeForces(const TArray<FVector>& InPositions, TArray<FVector>& OutForces);

	/* Inefficient n^2 computation */
	void ComputeForces_Direct(const TArray<FVector>& InPositions, TArray<FVector>& OutForces);
	/* Fast Multipole Method */
	void ComputeForces_FastMultipole(const TArray<FVector>& InPositions, TArray<FVector>& OutForces);

private:
	FRemoteSimulationInvariants::ConstPtr Invariants;

	FRemoteSimulationFrame::ConstPtr CurrentFrame;
	FRemoteSimulationFrame::Ptr NextFrame;

	FRemoteSimulationSolverSettings Settings;

#if UE_BUILD_DEBUG
	UWorld* DebugWorld;
#endif
};
