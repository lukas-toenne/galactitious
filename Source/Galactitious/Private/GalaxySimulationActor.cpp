// Fill out your copyright notice in the Description page of Project Settings.

#include "GalaxySimulationActor.h"

#include "FastMultipoleSimulationCache.h"

#include "Async/Async.h"
#include "Components/SceneComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogGalaxySimulationActor, Log, All);

AGalaxySimulationActor::AGalaxySimulationActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	SimulationCache = CreateDefaultSubobject<UFastMultipoleSimulationCache>(TEXT("FMM Simulation Cache"));
	SimulationCache->OnReset.AddDynamic(this, &AGalaxySimulationActor::OnCacheReset);
	SimulationCache->OnFrameAdded.AddDynamic(this, &AGalaxySimulationActor::OnCacheFrameAdded);

	USceneComponent* SceneRootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	AddOwnedComponent(SceneRootComponent);
	SetRootComponent(SceneRootComponent);
	SceneRootComponent->SetMobility(EComponentMobility::Movable);
}

void AGalaxySimulationActor::BeginPlay()
{
	ThreadRunnable = MakeUnique<FFastMultipoleSimulationThreadRunnable>();

	Super::BeginPlay();
}

void AGalaxySimulationActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CachePlayer.ResetAnimation(nullptr);
	StopSimulation();
	ThreadRunnable.Reset();
}

void AGalaxySimulationActor::Tick(float DeltaSeconds)
{
	CachePlayer.StepAnimation(SimulationCache, DeltaSeconds);

	//FFastMultipoleSimulationStepResult StepResult;
	//while (ThreadRunnable->PopCompletedStep(StepResult))
	//{
	//	SimulationCache->AddFrame(StepResult.Frame);
	//}
}

void AGalaxySimulationActor::StartSimulation(EGalaxySimulationStartMode StartMode)
{
	if (ThreadRunnable->IsRunning())
	{
		ThreadRunnable->StopThread();
	}

	ThreadRunnable->LaunchThread();

	OnSimulationStarted.Broadcast(this);

	SimulationCache->Reset();

	FFastMultipoleSimulationFrame::ConstPtr StartFrame = nullptr;
	int32 StepIndex = -1;
	switch (StartMode)
	{
	case EGalaxySimulationStartMode::DistributeStars:
	{
		TArray<FVector> Positions;
		TArray<FVector> Velocities;
		DistributePoints(NumStars, Positions, Velocities);
		StartFrame = MakeShared<FFastMultipoleSimulationFrame, ESPMode::ThreadSafe>(Positions, Velocities);
		SimulationCache->AddFrame(StartFrame);
		StepIndex = 0;
		break;
	}

	case EGalaxySimulationStartMode::ContinueCache:
		if (SimulationCache->GetNumFrames() > 0)
		{
			StartFrame = SimulationCache->GetLastFrame();
			StepIndex = SimulationCache->GetNumFrames() - 1;
		}
		else
		{
			UE_LOG(LogGalaxySimulationActor, Warning, TEXT("Simulation cache empty, initialization failed"));
		}
		break;
	}

	if (StartFrame)
	{
		ThreadRunnable->StartSimulation(StartFrame, 0, 1.0f);
	}

	// Tick stores result frames in the cache
	SetActorTickEnabled(true);
}

void AGalaxySimulationActor::StopSimulation()
{
	OnSimulationStopped.Broadcast(this);

	ThreadRunnable->StopThread();

	SetActorTickEnabled(false);
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
		float Radius = FMath::Pow(RadiusRng.GetFraction(), 1 / 3.0f) * Scale;

		float Phi = 2.0f * PI * PhiRng.GetFraction();
		float CosPhi = FMath::Cos(Phi);
		float SinPhi = FMath::Sin(Phi);

		float SinTheta = 2.0f * ThetaRng.GetFraction() - 1.0f;
		float CosTheta = FMath::Sqrt(FMath::Max(1.0f - SinTheta * SinTheta, 0.0f));

		const FVector P = OutPositions[i] = FVector(CosPhi * CosTheta, SinPhi * CosTheta, SinTheta) * Radius;

		const FVector Omega = FVector::UpVector * 1.0f;
		const FVector V = OutVelocities[i] = FVector::CrossProduct(Omega, P);
	}
}

void AGalaxySimulationActor::OnCacheReset(UFastMultipoleSimulationCache* InSimulationCache)
{
	CachePlayer.ResetAnimation(InSimulationCache);
}

void AGalaxySimulationActor::OnCacheFrameAdded(UFastMultipoleSimulationCache* InSimulationCache)
{
}
