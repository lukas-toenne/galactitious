// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "Components/ActorComponent.h"

#include "GalaxySimulationDebugComponent.generated.h"

class UFastMultipoleSimulationCache;

UCLASS(meta = (BlueprintSpawnableComponent))
class GALACTITIOUS_API UGalaxySimulationDebugComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGalaxySimulationDebugComponent(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	UPROPERTY(EditAnywhere)
	FColor PointColor = FColor::Silver;

protected:
	UFUNCTION()
	void OnSimulationReset(UFastMultipoleSimulationCache* SimulationCache);
	UFUNCTION()
	void OnSimulationStep(UFastMultipoleSimulationCache* SimulationCache);

	void LogPoints(const UFastMultipoleSimulationCache* SimulationCache) const;
};
