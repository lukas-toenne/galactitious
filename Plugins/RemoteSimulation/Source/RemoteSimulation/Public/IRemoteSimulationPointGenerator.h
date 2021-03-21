// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "IRemoteSimulationPointGenerator.generated.h"

USTRUCT(BlueprintType)
struct REMOTESIMULATION_API FRemoteSimulationPointResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Mass = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Position = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Velocity = FVector::ZeroVector;
};

UINTERFACE(MinimalAPI, Blueprintable)
class URemoteSimulationPointGenerator : public UInterface
{
	GENERATED_BODY()
};

class REMOTESIMULATION_API IRemoteSimulationPointGenerator
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Remote Simulation")
	bool GeneratePoint(int32 PointID, FRemoteSimulationPointResult& OutPoint);
};
