// Fill out your copyright notice in the Description page of Project Settings.

#include "GalaxySimulationActor.h"

#include "ProbabilityCurveFunctionLibrary.h"
#include "StellarSamplingData.h"
#include "TextureBakerFunctionLibrary.h"

#include "Components/StaticMeshComponent.h"
#include "FastMultipoleSimulation/FastMultipoleSimulation.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AGalaxySimulationActor::AGalaxySimulationActor()
{
}

void AGalaxySimulationActor::BeginPlay()
{
	Super::BeginPlay();
}

void AGalaxySimulationActor::DistributePoints(uint32 NumPoints, TArray<FVector>& OutPositions, TArray<FVector>& OutVelocities) const
{
	if (!ensureMsgf(RadialDensityCurve != nullptr, TEXT("Needs radial density curve")))
	{
		return;
	}
	if (!ensureMsgf(ThicknessCurve != nullptr, TEXT("Needs radial density curve")))
	{
		return;
	}

	FRichCurve RadialDensityNormalizedCurve, RadialSamplingCurve;
	UProbabilityCurveFunctionLibrary::ComputeQuantileRichCurve(
		RadialDensityCurve->FloatCurve, RadialDensityNormalizedCurve, RadialSamplingCurve);

	FRandomStream RadiusRng(98342);

	OutPositions.SetNum(NumPoints);
	OutVelocities.SetNum(NumPoints);
	for (uint32 i = 0; i < NumPoints; ++i)
	{
		float u1 = RadiusRng.GetFraction();

		float Radius = RadialSamplingCurve.Eval(u1, 0.5f);
	}
}

