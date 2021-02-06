// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FastMultipoleTypes.h"
#include "FastMultipoleSimulationCache.h"

DECLARE_LOG_CATEGORY_EXTERN(LogFastMultipole, Log, All);

enum class FASTMULTIPOLESIMULATION_API EFastMultipoleSimulationStatus : uint8
{
	NotInitialized,
	Stopped,
	Success,
};

struct FASTMULTIPOLESIMULATION_API FFastMultipoleSimulationStepResult
{
	EFastMultipoleSimulationStatus Status;
	int32 StepIndex;
};

class FASTMULTIPOLESIMULATION_API FFastMultipoleSimulation
{
public:
	using PointIndexGridType = FastMultipole::PointIndexGridType;
	using PointDataGridType = FastMultipole::PointDataGridType;

	static void Init();
	static void Shutdown();

	FFastMultipoleSimulation(UFastMultipoleSimulationCache* SimulationCache);
	~FFastMultipoleSimulation();

	void Reset(TArray<FVector>& InitialPositions, TArray<FVector>& InitialVelocities);
	void ResetToCache();
	bool Step(FThreadSafeBool& bStopRequested, float DeltaTime, FFastMultipoleSimulationStepResult& Result);

protected:

	void ComputeForces();
	void IntegratePositions(float DeltaTime);

	void BuildPointGrid(const TArray<FVector>& Positions, PointDataGridType::Ptr& PointDataGrid);
	void ClearPointGrid();

	// void BuildMomentsGrid();

	/* Inefficient n^2 computation */
	void ComputeForcesDirect();

private:
	int32 StepIndex;
	UFastMultipoleSimulationCache* SimulationCache;

	FFastMultipoleSimulationFramePtr CurrentFrame;
	FFastMultipoleSimulationFramePtr NextFrame;
};
