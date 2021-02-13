// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FastMultipoleTypes.h"

#include "FastMultipoleOpenVDBGuardEnter.h"
#include <openvdb/openvdb.h>
#include <openvdb/points/PointDataGrid.h>
#include "FastMultipoleOpenVDBGuardLeave.h"

#include "FastMultipoleSimulationFrame.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogFastMultipoleSimulationFrame, Log, All);

USTRUCT(BlueprintType)
struct FASTMULTIPOLESIMULATION_API FFastMultipoleSimulationFrame
{
	GENERATED_BODY()

	using Ptr = TSharedPtr<FFastMultipoleSimulationFrame, ESPMode::ThreadSafe>;
	using ConstPtr = TSharedPtr<FFastMultipoleSimulationFrame const, ESPMode::ThreadSafe>;

	FFastMultipoleSimulationFrame();
	FFastMultipoleSimulationFrame(TArray<FVector>& Positions, TArray<FVector>& Velocities);
	FFastMultipoleSimulationFrame(const FFastMultipoleSimulationFrame& Other) = default;

	FFastMultipoleSimulationFrame& operator=(const FFastMultipoleSimulationFrame& Other) = default;

	int32 GetNumPoints() const;
	void SetNumPoints(int32 NumPoints);
	void Empty();

	float GetDeltaTime() const { return DeltaTime; }
	void SetDeltaTime(float InDeltaTime);

	const TArray<FVector>& GetPositions() const { return Positions; }
	const TArray<FVector>& GetVelocities() const { return Velocities; }
	const TArray<FVector>& GetForces() const { return Forces; }

	TArray<FVector>& GetPositions() { return Positions; }
	TArray<FVector>& GetVelocities() { return Velocities; }
	TArray<FVector>& GetForces() { return Forces; }

	FastMultipole::PointDataGridType::Ptr GetPointDataGrid() const { return PointDataGrid; }

	void SetPoint(int32 Index, const FVector& InPosition, const FVector& InVelocity, const FVector& InForce = FVector::ZeroVector);
	void SetPostion(int32 Index, const FVector& InPosition);
	void SetVelocity(int32 Index, const FVector& InVelocity);
	void SetForce(int32 Index, const FVector& InForce);
	void AddForce(int32 Index, const FVector& InForce);

private:
	float DeltaTime;
	TArray<FVector> Positions;
	TArray<FVector> Velocities;
	TArray<FVector> Forces;

	FastMultipole::PointDataGridType::Ptr PointDataGrid;
};
