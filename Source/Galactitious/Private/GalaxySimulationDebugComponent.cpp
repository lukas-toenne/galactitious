// Fill out your copyright notice in the Description page of Project Settings.

#include "GalaxySimulationDebugComponent.h"

#include "DrawDebugHelpers.h"
#include "FastMultipoleSimulationCache.h"
#include "GalaxySimulationActor.h"

#include "VisualLogger/VisualLogger.h"

DEFINE_LOG_CATEGORY_STATIC(LogGalaxySimulationDebug, Log, All);
DEFINE_LOG_CATEGORY_STATIC(LogGalaxySimulationDebug_Points, Log, All);

FGalaxySimulationSequencer::FGalaxySimulationSequencer()
{
	ResetAnimation(nullptr);
}

void FGalaxySimulationSequencer::ResetAnimation(const UFastMultipoleSimulationCache* SimulationCache)
{
	AnimCacheStep = 0;
	AnimationTime = 0.0f;

	UpdateResultFrame(SimulationCache);
}

void FGalaxySimulationSequencer::StepAnimation(const UFastMultipoleSimulationCache* SimulationCache, float DeltaTime)
{
	if (!SimulationCache)
	{
		ResetAnimation(nullptr);
		return;
	}

	const int32 NumFrames = SimulationCache->GetNumFrames();
	const int32 OldAnimCacheStep = AnimCacheStep;

	if (AnimCacheStep >= 0 && AnimCacheStep < NumFrames - 1)
	{
		AnimationTime += DeltaTime;
		if (AnimationTime >= 1.0f)
		{
			AnimCacheStep += (int32)AnimationTime;
			if (AnimCacheStep < NumFrames)
			{
				AnimationTime = FMath::Fmod(AnimationTime, 1.0f);
			}
			else
			{
				// Animation will stop until more frames are added
				AnimCacheStep = NumFrames - 1;
				AnimationTime = 1.0f;
			}
		}

		UpdateResultFrame(SimulationCache);
	}
}

void FGalaxySimulationSequencer::GetFrameInterval(
	const UFastMultipoleSimulationCache* SimulationCache, FFastMultipoleSimulationFrame::ConstPtr& OutStartFrame,
	FFastMultipoleSimulationFrame::ConstPtr& OutEndFrame) const
{
	check(SimulationCache);

	int32 NumFrames = SimulationCache->GetNumFrames();
	if (AnimCacheStep < 0)
	{
		OutStartFrame.Reset();
		OutEndFrame.Reset();
	}
	else if (AnimCacheStep < NumFrames - 1)
	{
		OutStartFrame = SimulationCache->GetFrame(AnimCacheStep);
		OutEndFrame = SimulationCache->GetFrame(AnimCacheStep + 1);
	}
	else if (AnimCacheStep < NumFrames)
	{
		OutStartFrame = SimulationCache->GetFrame(AnimCacheStep);
		OutEndFrame.Reset();
	}
	else
	{
		OutStartFrame.Reset();
		OutEndFrame.Reset();
	}
}

void FGalaxySimulationSequencer::UpdateResultFrame(const UFastMultipoleSimulationCache* SimulationCache)
{
	if (!SimulationCache)
	{
		ResultFrame.Empty();
		return;
	}

	FFastMultipoleSimulationFrame::ConstPtr StartFrame, EndFrame;
	GetFrameInterval(SimulationCache, StartFrame, EndFrame);
	if (StartFrame)
	{
		const TArray<FVector>& PosBegin = StartFrame->GetPositions();
		const TArray<FVector>& VelBegin = StartFrame->GetVelocities();
		const TArray<FVector>& ForceBegin = StartFrame->GetForces();

		if (EndFrame)
		{
			const TArray<FVector>& PosEnd = EndFrame->GetPositions();
			const TArray<FVector>& VelEnd = EndFrame->GetVelocities();
			const TArray<FVector>& ForceEnd = EndFrame->GetForces();

			int32 NumPoints = FMath::Min(StartFrame->GetNumPoints(), EndFrame->GetNumPoints());
			ResultFrame.SetNumPoints(NumPoints);
			for (int32 i = 0; i < NumPoints; ++i)
			{
				ResultFrame.SetPoint(
					i, FMath::Lerp(PosBegin[i], PosEnd[i], AnimationTime), FMath::Lerp(VelBegin[i], VelEnd[i], AnimationTime),
					FMath::Lerp(ForceBegin[i], ForceEnd[i], AnimationTime));
			}
		}
		else
		{
			int32 NumPoints = StartFrame->GetNumPoints();
			ResultFrame.SetNumPoints(NumPoints);
			for (int32 i = 0; i < NumPoints; ++i)
			{
				ResultFrame.SetPoint(i, PosBegin[i], VelBegin[i], ForceBegin[i]);
			}
		}
	}
	else
	{
		ResultFrame.Empty();
	}
}

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
	UFastMultipoleSimulationCache* SimulationCache = nullptr;
	AGalaxySimulationActor* SimActor = GetOwner<AGalaxySimulationActor>();
	if (SimActor)
	{
		SimulationCache = SimActor->GetSimulationCache();
	}

	// UE_LOG(LogGalaxySimulationDebug, Display, TEXT("Anim: %d | %.3f, %010x..%010x"), AnimCacheStep, AnimationTime, AnimFrameBegin.Get(),
	// AnimFrameEnd.Get());
	if (SimulationCache)
	{
		StepAnimation(SimulationCache, DeltaTime * AnimationSpeed);

		ShowAnimatedPoints();
	}
	else
	{
		ResetAnimation(nullptr);
	}
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

	ResetAnimation(SimulationCache);
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
	UWorld* World = GetWorld();

	const FFastMultipoleSimulationFrame& Frame = GetResultFrame();
	const int32 NumPoints = Frame.GetNumPoints();
	for (int32 i = 0; i < NumPoints; ++i)
	{
		DrawDebugPoint(World, Frame.GetPositions()[i], 3.0f, PointColor);
	}
}
