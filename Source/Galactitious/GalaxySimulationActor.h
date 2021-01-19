// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "GameFramework/Actor.h"

#include "GalaxySimulationActor.generated.h"

UCLASS(BlueprintType)
class GALACTITIOUS_API AGalaxySimulationActor : public AActor
{
	GENERATED_BODY()

public:
	AGalaxySimulationActor();

	virtual void BeginPlay() override;

	UFUNCTION()
	class UFastMultipoleSimulation* GetSimulation() const { return Simulation; }

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 NumStars = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Scale = 100.0f;

protected:
	void DistributePoints(uint32 NumPoints, TArray<FVector>& OutPositions, TArray<FVector>& OutVelocities) const;

private:
	UPROPERTY(Transient)
	class UFastMultipoleSimulation* Simulation;
};
