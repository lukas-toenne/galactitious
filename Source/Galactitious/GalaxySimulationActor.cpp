// Fill out your copyright notice in the Description page of Project Settings.

#include "GalaxySimulationActor.h"

#include "FastMultipoleSimulation/FastMultipoleSimulation.h"

AGalaxySimulationActor::AGalaxySimulationActor()
{
	Simulation = CreateDefaultSubobject<UFastMultipoleSimulation>(TEXT("FMM Simulation"));
}

void AGalaxySimulationActor::BeginPlay()
{
	Super::BeginPlay();

	TSharedPtr<TArray<FVector>> Positions = MakeShared<TArray<FVector>>();
	TSharedPtr<TArray<FVector>> Velocities = MakeShared<TArray<FVector>>();
	DistributePoints(NumStars, *Positions, *Velocities);

	Simulation->Reset(Positions);
}

void AGalaxySimulationActor::DistributePoints(uint32 NumPoints, TArray<FVector>& OutPositions, TArray<FVector>& OutVelocities) const
{
	FRandomStream RadiusRng(98342);
	FRandomStream PhiRng(8349292);
	FRandomStream ThetaRng(285713);

	OutPositions.SetNum(NumPoints);
	OutVelocities.SetNum(NumPoints);
	for (uint32 i = 0; i < NumPoints; ++i)
	{
		float Radius = FMath::Pow(RadiusRng.GetFraction(), 1/3.0f) * Scale;

		float Phi = 2.0f * PI * PhiRng.GetFraction();
		float CosPhi = FMath::Cos(Phi);
		float SinPhi = FMath::Sin(Phi);

		float CosTheta = 2.0f * ThetaRng.GetFraction() - 1.0f;
		float SinTheta = FMath::Sqrt(FMath::Max(1.0f - CosTheta * CosTheta, 0.0f));

		const FVector P = OutPositions[i] = FVector(CosPhi * CosTheta, SinPhi * CosTheta, SinTheta) * Radius;

		const FVector Omega = FVector::UpVector * 1.0f;
		const FVector V = OutVelocities[i] = FVector::CrossProduct(Omega, P);
	}
}

