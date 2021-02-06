// Fill out your copyright notice in the Description page of Project Settings.

#include "FastMultipoleSimulationFrame.h"

#define LOCTEXT_NAMESPACE "FastMultipole"
DEFINE_LOG_CATEGORY(LogFastMultipoleSimulationFrame)

FFastMultipoleSimulationFrame::FFastMultipoleSimulationFrame()
{
}

FFastMultipoleSimulationFrame::FFastMultipoleSimulationFrame(TArray<FVector>& Positions, TArray<FVector>& Velocities)
{
	if (Positions.Num() != Velocities.Num())
	{
		UE_LOG(
			LogFastMultipoleSimulationFrame, Error, TEXT("Input arrays must have same size (positions: %d, velocities: %d)"),
			Positions.Num(), Velocities.Num());
		return;
	}

	Positions = MoveTemp(Positions);
	Velocities = MoveTemp(Velocities);
}

#undef LOCTEXT_NAMESPACE
