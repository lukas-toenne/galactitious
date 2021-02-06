// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FastMultipoleSimulationFrame.h"
#include "FastMultipoleTypes.h"

#include "FastMultipoleOpenVDBGuardEnter.h"
#include <openvdb/openvdb.h>
#include <openvdb/points/PointDataGrid.h>
#include "FastMultipoleOpenVDBGuardLeave.h"

#include "FastMultipoleSimulationCache.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogFastMultipoleSimulationCache, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFastMultipoleSimulationCacheResetDelegate, UFastMultipoleSimulationCache*, SimulationCache);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFastMultipoleSimulationCacheStepDelegate, UFastMultipoleSimulationCache*, SimulationCache);

UCLASS(BlueprintType)
class FASTMULTIPOLESIMULATION_API UFastMultipoleSimulationCache : public UObject
{
	GENERATED_BODY()

public:
	UFastMultipoleSimulationCache();
	virtual ~UFastMultipoleSimulationCache();

	int32 GetNumFrames() const;

	FFastMultipoleSimulationFramePtr GetFrame(int32 Step) const;
	FFastMultipoleSimulationFramePtr GetLastFrame() const;

	void Reset();
	bool AddFrame(const FFastMultipoleSimulationFramePtr& InFrame);

public:
	UPROPERTY(BlueprintAssignable)
	FFastMultipoleSimulationCacheResetDelegate OnSimulationReset;

	UPROPERTY(BlueprintAssignable)
	FFastMultipoleSimulationCacheStepDelegate OnSimulationStep;

private:
	TArray<FFastMultipoleSimulationFramePtr> Frames;

	mutable FRWLock FramesMutex;
};
