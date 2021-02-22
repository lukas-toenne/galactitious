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

	SimulationCache = CreateDefaultSubobject<UFastMultipoleSimulationCache>(TEXT("FMM Simulation Cache"), true);
	SimulationCache->SetCapacity(100, EFastMultipoleCacheResizeMode::PruneStart);

	CachePlayer = CreateDefaultSubobject<UGalaxySimulationCachePlayer>(TEXT("Cache Player"), true);
	CachePlayer->SetSimulationCache(SimulationCache);

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
	StopSimulation();
	ThreadRunnable.Reset();
}

void AGalaxySimulationActor::Tick(float DeltaSeconds)
{
	// Store finished simulation frames in the cache
	FFastMultipoleSimulationStepResult StepResult;
	while (ThreadRunnable->PopCompletedStep(StepResult))
	{
		SimulationCache->AddFrame(StepResult.Frame);
	}

	// Schedule new simulation steps if the player reaches the end of the cache
	SchedulePrecomputeSteps();
}

void AGalaxySimulationActor::StartSimulation(EGalaxySimulationStartMode StartMode)
{
	if (ThreadRunnable->IsRunning())
	{
		ThreadRunnable->StopThread();
	}
	if (EnableDebugDrawing)
	{
		ThreadRunnable->SetDebugWorld(GetWorld());
	}
	ThreadRunnable->LaunchThread();

	FFastMultipoleSimulationInvariants::Ptr SimulationInvariants = nullptr;
	FFastMultipoleSimulationFrame::Ptr StartFrame = nullptr;
	switch (StartMode)
	{
	case EGalaxySimulationStartMode::DistributeStars:
	{
		SimulationCache->Reset();

		TArray<FVector> Positions;
		TArray<FVector> Velocities;
		DistributePoints(NumStars, Positions);
		ComputeVelocities(Positions, Velocities);

		SimulationInvariants = SetupInvariants(NumStars);

		StartFrame = MakeShared<FFastMultipoleSimulationFrame, ESPMode::ThreadSafe>(Positions, Velocities);
		SimulationCache->AddFrame(StartFrame);

		break;
	}

	case EGalaxySimulationStartMode::ContinueCache:
		if (SimulationCache->GetNumFrames() > 0)
		{
			StartFrame = MakeShared<FFastMultipoleSimulationFrame, ESPMode::ThreadSafe>(*SimulationCache->GetLastFrame());

			// TODO cache this
			SimulationInvariants = SetupInvariants(StartFrame->GetNumPoints());
		}
		else
		{
			UE_LOG(LogGalaxySimulationActor, Warning, TEXT("Simulation cache empty, initialization failed"));
		}
		break;
	}

	if (StartFrame)
	{
		float KineticEnergyAverage;
		FFastMultipoleSimulationUtils::ComputeStableForceFactor(
			SimulationInvariants->Masses, StartFrame->GetPositions(), StartFrame->GetVelocities(), SimulationInvariants->SofteningRadius,
			KineticEnergyAverage, SimulationInvariants->ForceFactor);

		ThreadRunnable->StartSimulation(SimulationInvariants, StartFrame, SimulationStepSize, SimulationIntegrator, SimulationForceMethod);

		SchedulePrecomputeSteps();
	}

	// Tick stores result frames in the cache
	SetActorTickEnabled(true);

	OnSimulationStarted.Broadcast(this);
}

void AGalaxySimulationActor::StopSimulation()
{
	OnSimulationStopped.Broadcast(this);

	ThreadRunnable->StopThread();

	SetActorTickEnabled(false);
}

FFastMultipoleSimulationInvariants::Ptr AGalaxySimulationActor::SetupInvariants(int32 NumPoints)
{
	// TODO cache this
	FFastMultipoleSimulationInvariants::Ptr SimulationInvariants = MakeShared<FFastMultipoleSimulationInvariants, ESPMode::ThreadSafe>();

	SimulationInvariants->SofteningRadius = GravitySofteningRadius;
	SimulationInvariants->ForceFactor = 1.0f; // Computed later based on positions

	DistributeMasses(NumPoints, SimulationInvariants->Masses);
	SimulationInvariants->InvMasses.SetNumUninitialized(NumPoints);
	for (int32 i = 0; i < NumPoints; ++i)
	{
		const float Mass = SimulationInvariants->Masses[i];
		SimulationInvariants->InvMasses[i] = FMath::IsNearlyZero(Mass) ? 0.0f : 1.0f / Mass;
	}

	return SimulationInvariants;
}

void AGalaxySimulationActor::DistributeMasses(int32 NumPoints, TArray<float>& OutMasses) const
{
	OutMasses.SetNumUninitialized(NumPoints);
	for (int32 i = 0; i < NumPoints; ++i)
	{
		OutMasses[i] = 1.0f;
	}
}

void AGalaxySimulationActor::DistributePoints(int32 NumPoints, TArray<FVector>& OutPositions) const
{
#if 1
	FRandomStream RadiusRng(98342);
	FRandomStream PhiRng(8349292);
	FRandomStream ThetaRng(285713);

	OutPositions.SetNumUninitialized(NumPoints);
	for (int32 i = 0; i < NumPoints; ++i)
	{
		float Radius = FMath::Pow(RadiusRng.GetFraction(), 1 / 3.0f) * Scale;

		float Phi = 2.0f * PI * PhiRng.GetFraction();
		float CosPhi = FMath::Cos(Phi);
		float SinPhi = FMath::Sin(Phi);

		float SinTheta = 2.0f * ThetaRng.GetFraction() - 1.0f;
		float CosTheta = FMath::Sqrt(FMath::Max(1.0f - SinTheta * SinTheta, 0.0f));

		OutPositions[i] = FVector(CosPhi * CosTheta, SinPhi * CosTheta, SinTheta) * Radius;
	}
#else
	OutPositions.SetNumUninitialized(NumPoints);
	for (int32 i = 0; i < NumPoints; ++i)
	{
		float Radius = Scale;

		float Phi = 2.0f * PI * i / NumPoints;
		float CosPhi = FMath::Cos(Phi);
		float SinPhi = FMath::Sin(Phi);

		float SinTheta = 0.0f;
		float CosTheta = 1.0f;

		OutPositions[i] = FVector(CosPhi * CosTheta, SinPhi * CosTheta, SinTheta) * Radius;
	}
#endif
}

void AGalaxySimulationActor::ComputeVelocities(const TArray<FVector>& InPositions, TArray<FVector>& OutVelocities)
{
	const int32 NumPoints = InPositions.Num();

	const float Speed = 0.2f;
	const float InvScale_3_2 = FMath::InvSqrt(Scale * Scale * Scale);
	const float TotalMass = NumPoints;

	const FVector LocalUp = FVector::UpVector;

	OutVelocities.SetNumUninitialized(NumPoints);
	for (int32 i = 0; i < NumPoints; ++i)
	{
		const FVector P = InPositions[i];
		const float R = P.Size();
		// Compute velocity such that centripetal force cancels out gravity according to shell theorem
		const FVector U = FVector::CrossProduct(LocalUp, P);
		const FVector V = U.GetSafeNormal() * R /* * InvScale_3_2*/;
		OutVelocities[i] = Speed * V;
	}
}

int32 AGalaxySimulationActor::SchedulePrecomputeSteps()
{
	// Check remaining number of cache frames after the end of the current interval.
	const int32 RemainingCacheFrames = SimulationCache->GetNumFrames() - (CachePlayer->GetCacheStep() + 1);
	check(RemainingCacheFrames >= 0);

	// Schedule more steps if needed.
	// Player interpolates between current cache step and the next, so have to add one frame.
	const int32 NumStepsToSchedule = FMath::Max(NumStepsPrecompute + 1 - ThreadRunnable->GetNumScheduledSteps() - RemainingCacheFrames, 0);
	for (int i = 0; i < NumStepsToSchedule; ++i)
	{
		ThreadRunnable->ScheduleStep();
	}
	return NumStepsToSchedule;
}
