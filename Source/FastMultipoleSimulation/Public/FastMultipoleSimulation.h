// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FastMultipoleTypes.h"

DECLARE_LOG_CATEGORY_EXTERN(LogFastMultipole, Log, All);

class FASTMULTIPOLESIMULATION_API FFastMultipoleSimulation
{
public:
	using PointIndexGridType = FastMultipole::PointIndexGridType;
	using PointDataGridType = FastMultipole::PointDataGridType;

	static void Init();
	static void Shutdown();

	void ComputeForces();
	void IntegratePositions(float DeltaTime);

protected:

	void BuildPointGrid(const TArray<FVector>& Positions, PointDataGridType::Ptr& PointDataGrid);
	void ClearPointGrid();

	// void BuildMomentsGrid();

	/* Inefficient n^2 computation */
	void ComputeForcesDirect();
};
