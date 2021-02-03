// Fill out your copyright notice in the Description page of Project Settings.

#include "GalaxySimulationDebugComponent.h"

#include "FastMultipoleSimulationCache.h"
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
		if (UFastMultipoleSimulationCache* SimulationCache = SimActor->GetSimulationCache())
		{
			LogPoints(SimulationCache);
			SimulationCache->OnSimulationReset.AddDynamic(this, &UGalaxySimulationDebugComponent::OnSimulationReset);
		}
	}
}

void UGalaxySimulationDebugComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
}

void UGalaxySimulationDebugComponent::OnSimulationReset(UFastMultipoleSimulationCache* SimulationCache)
{
	LogPoints(SimulationCache);
}

void UGalaxySimulationDebugComponent::OnSimulationStep(UFastMultipoleSimulationCache* SimulationCache)
{
	LogPoints(SimulationCache);
}

void UGalaxySimulationDebugComponent::LogPoints(const UFastMultipoleSimulationCache* SimulationCache) const
{
	check(SimulationCache);

#if ENABLE_VISUAL_LOG
	if (FFastMultipoleSimulationFramePtr Frame = SimulationCache->GetLastFrame())
	{
		const TArray<FVector>& Positions = Frame->Positions;
		for (int32 i = 0; i < Positions.Num(); ++i)
		{
			const FVector& Point = Positions[i];
			UE_VLOG_LOCATION(this, LogGalaxySimulationDebug_Points, Log, Point, 1.0f, PointColor, TEXT(""));
		}
	}
#endif
}