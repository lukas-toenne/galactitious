// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FastMultipoleTypes.h"
#include "FastMultipoleSimulationFrame.h"

DECLARE_LOG_CATEGORY_EXTERN(LogFastMultipole, Log, All);

struct FASTMULTIPOLESIMULATION_API FFastMultipoleSimulationStepRequest
{
	int32 Dummy;
};

struct FASTMULTIPOLESIMULATION_API FFastMultipoleSimulationStepResult
{
	EFastMultipoleSimulationStatus Status;
	FFastMultipoleSimulationFrame::Ptr Frame;
};

class FASTMULTIPOLESIMULATION_API FFastMultipoleSimulation
{
public:
	using PointIndexGridType = FastMultipole::PointIndexGridType;
	using PointDataGridType = FastMultipole::PointDataGridType;

	static void Init();
	static void Shutdown();

	FFastMultipoleSimulation(float StepSize, EFastMultipoleSimulationIntegrator Integrator);
	~FFastMultipoleSimulation();

	void Reset(FFastMultipoleSimulationFrame::ConstPtr Frame);
	bool Step(FThreadSafeBool& bStopRequested, FFastMultipoleSimulationStepResult& Result);

	FFastMultipoleSimulationFrame::ConstPtr GetCurrentFrame() const { return CurrentFrame; }

protected:

	void Integrate();

	void ComputeForces(const TArray<FVector>& InPositions, TArray<FVector>& OutForces);

	/* Inefficient n^2 computation */
	void ComputeForcesDirect(const TArray<FVector>& InPositions, TArray<FVector>& OutForces);

	void BuildPointGrid(const TArray<FVector>& Positions, PointDataGridType::Ptr& PointDataGrid);
	void ClearPointGrid();

	// void BuildMomentsGrid();

private:
	FFastMultipoleSimulationFrame::ConstPtr CurrentFrame;
	FFastMultipoleSimulationFrame::Ptr NextFrame;

	float StepSize;
	EFastMultipoleSimulationIntegrator Integrator;
};
