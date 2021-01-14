// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "GameFramework/Actor.h"

THIRD_PARTY_INCLUDES_START
#include <openvdb/openvdb.h>
THIRD_PARTY_INCLUDES_END

#include "GravitySimulationActor.generated.h"

UCLASS(BlueprintType)
class GALACTITIOUS_API AGravitySimulationActor : public AActor
{
	GENERATED_BODY()

public:
	using TreeType = openvdb::tree::Tree4<float, 5, 4, 3>::Type;
	using GridType = openvdb::Grid<TreeType>;

	AGravitySimulationActor();

	virtual void BeginPlay() override;

	GridType::Ptr GetGrid() const { return Grid; }

protected:
	void DistributePoints(TArray<FVector>& OutPositions, TArray<FVector>& OutVelocities) const;

private:
	GridType::Ptr Grid;
};
