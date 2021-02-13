// Fill out your copyright notice in the Description page of Project Settings.

#include "GalaxySimulationDebugComponent.h"

#include "DrawDebugHelpers.h"
#include "FastMultipoleSimulationCache.h"
#include "GalaxySimulationActor.h"
#include "GalaxySimulationCachePlayer.h"

#include "VisualLogger/VisualLogger.h"

DEFINE_LOG_CATEGORY_STATIC(LogGalaxySimulationDebug, Log, All);
DEFINE_LOG_CATEGORY_STATIC(LogGalaxySimulationDebug_Points, Log, All);

UGalaxySimulationDebugComponent::UGalaxySimulationDebugComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UGalaxySimulationDebugComponent::BeginPlay()
{
	Super::BeginPlay();

	AGalaxySimulationActor* SimActor = GetOwner<AGalaxySimulationActor>();
	if (SimActor)
	{
		SimActor->OnSimulationStarted.AddDynamic(this, &UGalaxySimulationDebugComponent::OnSimulationStarted);
		SimActor->OnSimulationStopped.AddDynamic(this, &UGalaxySimulationDebugComponent::OnSimulationStopped);

		if (UFastMultipoleSimulationCache* SimulationCache = SimActor->GetSimulationCache())
		{
			SimulationCache->OnReset.AddDynamic(this, &UGalaxySimulationDebugComponent::OnCacheReset);
			SimulationCache->OnFrameAdded.AddDynamic(this, &UGalaxySimulationDebugComponent::OnCacheFrameAdded);
		}
	}
}

void UGalaxySimulationDebugComponent::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	AGalaxySimulationActor* SimActor = GetOwner<AGalaxySimulationActor>();
	if (SimActor)
	{
		SimActor->OnSimulationStarted.RemoveAll(this);
		SimActor->OnSimulationStopped.RemoveAll(this);

		if (UFastMultipoleSimulationCache* SimulationCache = SimActor->GetSimulationCache())
		{
			SimulationCache->OnReset.RemoveAll(this);
			SimulationCache->OnFrameAdded.RemoveAll(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void UGalaxySimulationDebugComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	ShowAnimatedPoints();
}

void UGalaxySimulationDebugComponent::OnSimulationStarted(AGalaxySimulationActor* SimulationActor)
{
}

void UGalaxySimulationDebugComponent::OnSimulationStopped(AGalaxySimulationActor* SimulationActor)
{
}

void UGalaxySimulationDebugComponent::OnCacheReset(UFastMultipoleSimulationCache* SimulationCache)
{
	UE_LOG(LogGalaxySimulationDebug, Display, TEXT("Cache Reset"));
}

void UGalaxySimulationDebugComponent::OnCacheFrameAdded(UFastMultipoleSimulationCache* SimulationCache)
{
	UE_LOG(LogGalaxySimulationDebug, Display, TEXT("Cache Frame Added"));
	LogPoints(SimulationCache);
}

void UGalaxySimulationDebugComponent::LogPoints(const UFastMultipoleSimulationCache* SimulationCache) const
{
	check(SimulationCache);

#if ENABLE_VISUAL_LOG
	if (FFastMultipoleSimulationFrame::ConstPtr Frame = SimulationCache->GetLastFrame())
	{
		const TArray<FVector>& Positions = Frame->GetPositions();
		for (int32 i = 0; i < Positions.Num(); ++i)
		{
			const FVector& Point = Positions[i];
			UE_VLOG_LOCATION(this, LogGalaxySimulationDebug_Points, Log, Point, 1.0f, PointColor, TEXT(""));
		}
	}
#endif
}

void UGalaxySimulationDebugComponent::ShowAnimatedPoints() const
{
	AGalaxySimulationActor* SimActor = GetOwner<AGalaxySimulationActor>();
	if (SimActor)
	{
		UWorld* World = GetWorld();

		const FFastMultipoleSimulationFrame& Frame = SimActor->GetCachePlayer()->GetResultFrame();
		const int32 NumPoints = Frame.GetNumPoints();
		for (int32 i = 0; i < NumPoints; ++i)
		{
			DrawDebugPoint(World, Frame.GetPositions()[i], 3.0f, PointColor);
		}
	}
}
