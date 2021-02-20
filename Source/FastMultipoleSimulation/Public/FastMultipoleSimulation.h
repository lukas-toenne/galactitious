// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FastMultipoleSimulationFrame.h"
#include "FastMultipoleTypes.h"

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

	FFastMultipoleSimulation(float StepSize, EFastMultipoleSimulationIntegrator Integrator, EFastMultipoleSimulationForceMethod ForceMethod);
	~FFastMultipoleSimulation();

	void SetDebugWorld(UWorld* InDebugWorld);

	void Reset(FFastMultipoleSimulationInvariants::ConstPtr Invariants, FFastMultipoleSimulationFrame::Ptr StartFrame);
	bool Step(FThreadSafeBool& bStopRequested, FFastMultipoleSimulationStepResult& Result);

	FFastMultipoleSimulationFrame::ConstPtr GetCurrentFrame() const { return CurrentFrame; }

protected:
	void Integrate();

	void ComputeForces(const TArray<FVector>& InPositions, TArray<FVector>& OutForces);

	/* Inefficient n^2 computation */
	void ComputeForces_Direct(const TArray<FVector>& InPositions, TArray<FVector>& OutForces);
	/* Fast Multipole Method */
	void ComputeForces_FastMultipole(const TArray<FVector>& InPositions, TArray<FVector>& OutForces);

	void BuildPointGrid(const TArray<FVector>& Positions, PointDataGridType::Ptr& PointDataGrid);
	void ClearPointGrid();

	// void BuildMomentsGrid();

private:
	FFastMultipoleSimulationInvariants::ConstPtr Invariants;

	FFastMultipoleSimulationFrame::ConstPtr CurrentFrame;
	FFastMultipoleSimulationFrame::Ptr NextFrame;

	float StepSize;
	EFastMultipoleSimulationIntegrator Integrator;
	EFastMultipoleSimulationForceMethod ForceMethod;

#if UE_BUILD_DEBUG
	UWorld* DebugWorld;
#endif
};

struct FASTMULTIPOLESIMULATION_API FFastMultipoleSimulationUtils
{
	/**
	 * Uses the Virial theorem to compute a force factor such that the overall system is stable.
	 */
	static void ComputeStableForceFactor(
		const TArray<float>& Masses, const TArray<FVector>& Positions, const TArray<FVector>& Velocities, float Softening,
		float& OutKineticEnergyAverage,
		float& OutStableForceFactor);
};
