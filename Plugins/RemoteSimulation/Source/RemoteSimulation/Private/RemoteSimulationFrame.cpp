// Fill out your copyright notice in the Description page of Project Settings.

#include "RemoteSimulationFrame.h"

#define LOCTEXT_NAMESPACE "RemoteSimulation"

FRemoteSimulationFrame::FRemoteSimulationFrame()
{
}

FRemoteSimulationFrame::FRemoteSimulationFrame(TArray<FVector>& InPositions, TArray<FVector>& InVelocities)
{
	if (InPositions.Num() != InVelocities.Num())
	{
		UE_LOG(
			LogRemoteSimulation, Error, TEXT("Input arrays must have same size (positions: %d, velocities: %d)"),
			InPositions.Num(), InVelocities.Num());
		return;
	}

	Positions = MoveTemp(InPositions);
	Velocities = MoveTemp(InVelocities);
	Forces.SetNumZeroed(Positions.Num());
}

void FRemoteSimulationFrame::ContinueFrom(const FRemoteSimulationFrame& Other)
{
	const int32 NumPoints = Other.GetNumPoints();

	Positions = Other.Positions;
	Velocities = Other.Velocities;
	Forces.SetNumZeroed(NumPoints);
}

int32 FRemoteSimulationFrame::GetNumPoints() const
{
	return Positions.Num();
}

void FRemoteSimulationFrame::SetNumPoints(int32 NumPoints)
{
	Positions.SetNumUninitialized(NumPoints);
	Velocities.SetNumUninitialized(NumPoints);
	Forces.SetNumUninitialized(NumPoints);
}

void FRemoteSimulationFrame::Empty()
{
	Positions.Empty();
	Velocities.Empty();
	Forces.Empty();
}

bool FRemoteSimulationFrame::IsValid() const
{
	const int32 NumPoints = Positions.Num();
	return Velocities.Num() == NumPoints && Forces.Num() == NumPoints;
}

void FRemoteSimulationFrame::SetPoint(int32 Index, const FVector& InPosition, const FVector& InVelocity, const FVector& InForce)
{
	check(Index < Positions.Num());
	Positions[Index] = InPosition;
	Velocities[Index] = InVelocity;
	Forces[Index] = InForce;
}

void FRemoteSimulationFrame::SetPostion(int32 Index, const FVector& InPosition)
{
	Positions[Index] = InPosition;
}

void FRemoteSimulationFrame::SetVelocity(int32 Index, const FVector& InVelocity)
{
	Velocities[Index] = InVelocity;
}

void FRemoteSimulationFrame::SetForce(int32 Index, const FVector& InForce)
{
	Forces[Index] = InForce;
}

void FRemoteSimulationFrame::AddForce(int32 Index, const FVector& InForce)
{
	Forces[Index] += InForce;
}

#undef LOCTEXT_NAMESPACE
