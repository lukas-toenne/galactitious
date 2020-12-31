// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

THIRD_PARTY_INCLUDES_START
#include <openvdb/openvdb.h>
THIRD_PARTY_INCLUDES_END

#include "GravitySimulationActor.generated.h"

//class UVDBGridComponent;

UCLASS(BlueprintType)
class GALACTITIOUS_API AGravitySimulationActor : public AActor
{
	GENERATED_BODY()

public:
	AGravitySimulationActor();

	virtual void BeginPlay() override;

private:
	//UPROPERTY(Transient)
	//UVDBGridComponent* FMMGrid;
};
