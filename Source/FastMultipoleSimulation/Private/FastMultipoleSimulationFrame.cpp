// Fill out your copyright notice in the Description page of Project Settings.

#include "FastMultipoleSimulationFrame.h"

#define LOCTEXT_NAMESPACE "FastMultipole"
DEFINE_LOG_CATEGORY(LogFastMultipoleSimulationFrame)

FFastMultipoleSimulationFrame::FFastMultipoleSimulationFrame()
{
}

FFastMultipoleSimulationFrame::FFastMultipoleSimulationFrame(TArray<FVector>& InPositions, TArray<FVector>& InVelocities)
{
	if (InPositions.Num() != InVelocities.Num())
	{
		UE_LOG(
			LogFastMultipoleSimulationFrame, Error, TEXT("Input arrays must have same size (positions: %d, velocities: %d)"),
			InPositions.Num(), InVelocities.Num());
		return;
	}

	Positions = MoveTemp(InPositions);
	Velocities = MoveTemp(InVelocities);
	Forces.SetNumZeroed(Positions.Num());
}

void FFastMultipoleSimulationFrame::SetDeltaTime(float InDeltaTime)
{
	DeltaTime = InDeltaTime;
}

int32 FFastMultipoleSimulationFrame::GetNumPoints() const
{
	return Positions.Num();
}

void FFastMultipoleSimulationFrame::SetNumPoints(int32 NumPoints)
{
	Positions.SetNumUninitialized(NumPoints);
	Velocities.SetNumUninitialized(NumPoints);
	Forces.SetNumUninitialized(NumPoints);
}

void FFastMultipoleSimulationFrame::Empty()
{
	Positions.Empty();
	Velocities.Empty();
	Forces.Empty();
}

void FFastMultipoleSimulationFrame::SetPoint(int32 Index, const FVector& InPosition, const FVector& InVelocity, const FVector& InForce)
{
	check(Index < Positions.Num());
	Positions[Index] = InPosition;
	Velocities[Index] = InVelocity;
	Forces[Index] = InForce;
}

void FFastMultipoleSimulationFrame::SetPostion(int32 Index, const FVector& InPosition)
{
	Positions[Index] = InPosition;
}

void FFastMultipoleSimulationFrame::SetVelocity(int32 Index, const FVector& InVelocity)
{
	Velocities[Index] = InVelocity;
}

void FFastMultipoleSimulationFrame::SetForce(int32 Index, const FVector& InForce)
{
	Forces[Index] = InForce;
}

void FFastMultipoleSimulationFrame::AddForce(int32 Index, const FVector& InForce)
{
	Forces[Index] += InForce;
}

#undef LOCTEXT_NAMESPACE
