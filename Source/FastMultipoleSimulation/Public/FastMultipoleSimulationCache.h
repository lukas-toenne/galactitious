// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FastMultipoleTypes.h"

#include "FastMultipoleOpenVDBGuardEnter.h"
#include <openvdb/openvdb.h>
#include <openvdb/points/PointDataGrid.h>
#include "FastMultipoleOpenVDBGuardLeave.h"

#include "FastMultipoleSimulationCache.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogFastMultipoleSimulationCache, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFastMultipoleSimulationCacheResetDelegate, UFastMultipoleSimulationCache*, SimulationCache);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFastMultipoleSimulationCacheStepDelegate, UFastMultipoleSimulationCache*, SimulationCache);

USTRUCT(BlueprintType)
struct FASTMULTIPOLESIMULATION_API FFastMultipoleSimulationFrame
{
	GENERATED_BODY()

public:
	float DeltaTime;
	TArray<FVector> Positions;
	TArray<FVector> Velocities;
	TArray<FVector> Forces;

	FastMultipole::PointDataGridType::Ptr PointDataGrid;
};

using FFastMultipoleSimulationFramePtr = TSharedPtr<FFastMultipoleSimulationFrame, ESPMode::ThreadSafe>;

UCLASS(BlueprintType)
class FASTMULTIPOLESIMULATION_API UFastMultipoleSimulationCache : public UObject
{
	GENERATED_BODY()

public:
	UFastMultipoleSimulationCache();
	virtual ~UFastMultipoleSimulationCache();

	void Reset(TArray<FVector>& InInitialPositions, TArray<FVector>& InInitialVelocities);
	void Clear();

	int32 GetNumFrames() const;

	FFastMultipoleSimulationFramePtr GetFrame(int32 Step) const;
	FFastMultipoleSimulationFramePtr GetLastFrame() const;

public:
	UPROPERTY(BlueprintAssignable)
	FFastMultipoleSimulationCacheResetDelegate OnSimulationReset;

	UPROPERTY(BlueprintAssignable)
	FFastMultipoleSimulationCacheStepDelegate OnSimulationStep;

private:
	TArray<FFastMultipoleSimulationFramePtr> Frames;

	mutable FRWLock FramesMutex;
};
