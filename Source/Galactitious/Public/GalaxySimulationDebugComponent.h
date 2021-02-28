// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FastMultipoleSimulationFrame.h"

#include "Components/ActorComponent.h"

#include "GalaxySimulationDebugComponent.generated.h"

class AGalaxySimulationActor;
class UFastMultipoleSimulationCache;

UCLASS(meta = (BlueprintSpawnableComponent))
class GALACTITIOUS_API UGalaxySimulationDebugComponent
	: public UActorComponent
{
	GENERATED_BODY()

public:
	UGalaxySimulationDebugComponent(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	UPROPERTY(EditAnywhere)
	FColor PointColor = FColor::Silver;

protected:
	UFUNCTION()
	void OnSimulationStarted(AGalaxySimulationActor* SimulationActor);
	UFUNCTION()
	void OnSimulationStopped(AGalaxySimulationActor* SimulationActor);

	void OnCacheReset(UFastMultipoleSimulationCache* SimulationCache);
	void OnCacheFrameAdded(UFastMultipoleSimulationCache* SimulationCache);

	void LogPoints(const UFastMultipoleSimulationCache* SimulationCache) const;
	void ShowAnimatedPoints() const;

private:
	FDelegateHandle CacheResetHandle;
	FDelegateHandle CacheFrameAddedHandle;
};
