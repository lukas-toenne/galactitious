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
	ResetAnimation();
}

void FGalaxySimulationSequencer::GetIntervalFrames(
	FFastMultipoleSimulationFramePtr& OutStartFrame, FFastMultipoleSimulationFramePtr& OutEndFrame) const
{
	OutStartFrame = AnimFrameBegin;
	OutEndFrame = AnimFrameEnd;
}

int32 FGalaxySimulationSequencer::GetNumPoints() const
{
	if (AnimFrameBegin)
	{
		const TArray<FVector>& PosBegin = AnimFrameBegin->Positions;
		if (AnimFrameEnd)
		{
			const TArray<FVector>& PosEnd = AnimFrameEnd->Positions;
			return FMath::Min(PosBegin.Num(), PosEnd.Num());
		}
		else
		{
			return PosBegin.Num();
		}
	}

	return 0;
}

bool FGalaxySimulationSequencer::GetLerped(int32 Index, FVector& OutPosition, FVector& OutVelocity, FVector& OutForce) const
{
	if (AnimFrameBegin)
	{
		const TArray<FVector>& PosBegin = AnimFrameBegin->Positions;
		const TArray<FVector>& VelBegin = AnimFrameBegin->Velocities;
		const TArray<FVector>& ForceBegin = AnimFrameBegin->Forces;

		if (AnimFrameEnd)
		{
			const TArray<FVector>& PosEnd = AnimFrameEnd->Positions;
			const TArray<FVector>& VelEnd = AnimFrameEnd->Velocities;
			const TArray<FVector>& ForceEnd = AnimFrameEnd->Forces;

			OutPosition = FMath::Lerp(PosBegin[Index], PosEnd[Index], AnimationTime);
			OutVelocity = FMath::Lerp(VelBegin[Index], VelEnd[Index], AnimationTime);
			OutForce = FMath::Lerp(ForceBegin[Index], ForceEnd[Index], AnimationTime);
		}
		else
		{
			OutPosition = PosBegin[Index];
			OutVelocity= VelBegin[Index];
			OutForce = ForceBegin[Index];
		}

		return true;
	}

	return false;
}

void FGalaxySimulationSequencer::ResetAnimation()
{
	AnimCacheStep = -1;
	AnimationTime = 0.0f;
	AnimFrameBegin.Reset();
	AnimFrameEnd.Reset();
}

void FGalaxySimulationSequencer::StepAnimation(const UFastMultipoleSimulationCache* SimulationCache, float DeltaTime)
{
	check(SimulationCache);

	if (AnimCacheStep < 0)
	{
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

			UpdateAnimationFrames(SimulationCache);
		}
	}
}

void FGalaxySimulationSequencer::UpdateAnimationFrames(const UFastMultipoleSimulationCache* SimulationCache)
{
	int32 NumFrames = SimulationCache->GetNumFrames();
	UE_LOG(
		LogGalaxySimulationDebug, Display, TEXT("Change Anim Frames: [%d/%d]..[%d/%d]"), AnimCacheStep, NumFrames, AnimCacheStep + 1,
		NumFrames);
	if (AnimCacheStep < 0)
	{
		AnimFrameBegin.Reset();
		AnimFrameEnd.Reset();
	}
	else if (AnimCacheStep < NumFrames - 1)
	{
		AnimFrameBegin = SimulationCache->GetFrame(AnimCacheStep);
		AnimFrameEnd = SimulationCache->GetFrame(AnimCacheStep + 1);
	}
	else if (AnimCacheStep < NumFrames)
	{
		AnimFrameBegin = SimulationCache->GetFrame(AnimCacheStep);
		AnimFrameEnd.Reset();
	}
	else
	{
		AnimFrameBegin.Reset();
		AnimFrameEnd.Reset();
	}
}

UGalaxySimulationDebugComponent::UGalaxySimulationDebugComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	ResetAnimation();
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

	//UE_LOG(LogGalaxySimulationDebug, Display, TEXT("Anim: %d | %.3f, %010x..%010x"), AnimCacheStep, AnimationTime, AnimFrameBegin.Get(), AnimFrameEnd.Get());
	if (SimulationCache)
	{
		StepAnimation(SimulationCache, DeltaTime * AnimationSpeed);

		ShowAnimatedPoints();
	}
	else
	{
		ResetAnimation();
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

	ResetAnimation();
}

void UGalaxySimulationDebugComponent::OnCacheFrameAdded(UFastMultipoleSimulationCache* SimulationCache)
{
	UE_LOG(LogGalaxySimulationDebug, Display, TEXT("Cache Frame Added"));
	LogPoints(SimulationCache);

	UpdateAnimationFrames(SimulationCache);
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

void UGalaxySimulationDebugComponent::ShowAnimatedPoints() const
{
	UWorld* World = GetWorld();

	const int32 NumPoints = GetNumPoints();
	for (int32 i = 0; i < NumPoints; ++i)
	{
		FVector Pos, Vel, Force;
		GetLerped(i, Pos, Vel, Force);
		DrawDebugPoint(World, Pos, 3.0f, PointColor);
	}
}
