// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FastMultipoleSimulationThreadRunnable.h"
#include "GalaxySimulationCachePlayer.h"

#include "Containers/Queue.h"
#include "GameFramework/Actor.h"

#include "GalaxySimulationActor.generated.h"

class UFastMultipoleSimulationCache;

UENUM(BlueprintType)
enum EGalaxySimulationStartMode
{
	DistributeStars,
	ContinueCache,
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGalaxySimulationStartedDelegate, AGalaxySimulationActor*, SimulationActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGalaxySimulationStoppedDelegate, AGalaxySimulationActor*, SimulationActor);

UCLASS(BlueprintType)
class GALACTITIOUS_API AGalaxySimulationActor : public AActor
{
	GENERATED_BODY()

public:
	AGalaxySimulationActor();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintGetter)
	UFastMultipoleSimulationCache* GetSimulationCache() const { return SimulationCache; }

	UFUNCTION(BlueprintGetter)
	UGalaxySimulationCachePlayer* GetCachePlayer() { return CachePlayer; }

	UFUNCTION(BlueprintCallable)
	void StartSimulation(EGalaxySimulationStartMode StartMode);

	UFUNCTION(BlueprintCallable)
	void StopSimulation();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 NumStars = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Scale = 100.0f;

	// Size of the simulation time step
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SimulationStepSize = 1.0f;

	// Number of steps to precompute in advance of the cache player.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 NumStepsPrecompute = 3;

	UPROPERTY(BlueprintAssignable)
	FGalaxySimulationStartedDelegate OnSimulationStarted;

	UPROPERTY(BlueprintAssignable)
	FGalaxySimulationStoppedDelegate OnSimulationStopped;

protected:
	void DistributePoints(uint32 NumPoints, TArray<FVector>& OutPositions, TArray<FVector>& OutVelocities) const;

	/** Schedule frames if the player reaches the end of the cache.
	 * @return Number of frames that have been scheduled.
	 */
	int32 SchedulePrecomputeSteps();

private:
	UPROPERTY(Transient, BlueprintGetter = GetSimulationCache)
	UFastMultipoleSimulationCache* SimulationCache;

	UPROPERTY(Transient, BlueprintGetter = GetCachePlayer)
	UGalaxySimulationCachePlayer* CachePlayer;

	/** Thread that runs the simulation. */
	class TUniquePtr<struct FFastMultipoleSimulationThreadRunnable> ThreadRunnable;
};
