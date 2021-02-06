// Fill out your copyright notice in the Description page of Project Settings.

#include "FastMultipoleSimulationFrame.h"

#define LOCTEXT_NAMESPACE "FastMultipole"
DEFINE_LOG_CATEGORY(LogFastMultipoleSimulationFrame)

FFastMultipoleSimulationFrame::FFastMultipoleSimulationFrame()
{
}

FFastMultipoleSimulationFrame::FFastMultipoleSimulationFrame(TArray<FVector>& InPositions, TArray<FVector>& InVelocities)
	: Positions(MoveTemp(InPositions))
	, Velocities(MoveTemp(InVelocities))
{
	if (InPositions.Num() != InVelocities.Num())
	{
		UE_LOG(
			LogFastMultipoleSimulationFrame, Error, TEXT("Input arrays must have same size (positions: %d, velocities: %d)"),
			InPositions.Num(), InVelocities.Num());
		return;
	}
}

#undef LOCTEXT_NAMESPACE
