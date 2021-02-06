// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FastMultipoleTypes.h"
#include "FastMultipoleSimulationFrame.h"

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

	FFastMultipoleSimulation();
	~FFastMultipoleSimulation();

	void Reset(FFastMultipoleSimulationFramePtr Frame, int32 StepIndex);
	bool Step(FThreadSafeBool& bStopRequested, float DeltaTime, FFastMultipoleSimulationStepResult& Result);

	FFastMultipoleSimulationFramePtr GetCurrentFrame() const { return CurrentFrame; }

protected:

	void ComputeForces();
	void IntegratePositions(float DeltaTime);

	void BuildPointGrid(const TArray<FVector>& Positions, PointDataGridType::Ptr& PointDataGrid);
	void ClearPointGrid();

	// void BuildMomentsGrid();

	/* Inefficient n^2 computation */
	void ComputeForcesDirect();

private:
	FFastMultipoleSimulationFramePtr CurrentFrame;
	FFastMultipoleSimulationFramePtr NextFrame;

	int32 StepIndex;
};
