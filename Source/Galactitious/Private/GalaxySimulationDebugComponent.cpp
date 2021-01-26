// Fill out your copyright notice in the Description page of Project Settings.

#include "GalaxySimulationDebugComponent.h"

#include "FastMultipoleSimulation.h"
#include "GalaxySimulationActor.h"

#include "VisualLogger/VisualLogger.h"

DEFINE_LOG_CATEGORY_STATIC(LogGalaxySimulationDebug_Points, Log, All);

UGalaxySimulationDebugComponent::UGalaxySimulationDebugComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UGalaxySimulationDebugComponent::BeginPlay()
{
	Super::BeginPlay();

	AGalaxySimulationActor* SimActor = GetOwner<AGalaxySimulationActor>();
	if (SimActor)
	{
		if (UFastMultipoleSimulation* Simulation = SimActor->GetSimulation())
		{
			LogPoints(Simulation);
			Simulation->OnSimulationReset.AddDynamic(this, &UGalaxySimulationDebugComponent::OnSimulationReset);
		}
	}
}

void UGalaxySimulationDebugComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
}

void UGalaxySimulationDebugComponent::OnSimulationReset(UFastMultipoleSimulation* Simulation)
{
	LogPoints(Simulation);
}

void UGalaxySimulationDebugComponent::OnSimulationStep(UFastMultipoleSimulation* Simulation)
{
	LogPoints(Simulation);
}

void UGalaxySimulationDebugComponent::LogPoints(const UFastMultipoleSimulation* Simulation) const
{
	check(Simulation);

#if ENABLE_VISUAL_LOG
	const TArray<FVector> Positions = Simulation->GetPositionData();
	for (int32 i = 0; i < Positions.Num(); ++i)
	{
		const FVector& Point = Positions[i];
		UE_VLOG_LOCATION(this, LogGalaxySimulationDebug_Points, Log, Point, 1.0f, PointColor, TEXT(""));
	}
#endif
}