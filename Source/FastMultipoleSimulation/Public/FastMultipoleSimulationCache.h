// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FastMultipoleOpenVDBGuardEnter.h"
#include "FastMultipoleOpenVDBGuardLeave.h"
#include "FastMultipoleSimulationFrame.h"
#include "FastMultipoleTypes.h"

#include <openvdb/openvdb.h>
#include <openvdb/points/PointDataGrid.h>

#include "FastMultipoleSimulationCache.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogFastMultipoleSimulationCache, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFastMultipoleSimulationCacheResetDelegate, UFastMultipoleSimulationCache*, SimulationCache);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FFastMultipoleSimulationCacheFrameAddedDelegate, UFastMultipoleSimulationCache*, SimulationCache);

UENUM()
enum EFastMultipoleCacheResizeMode
{
	/** Don't remove any frames. */
	None,
	/** Remove frames from the beginning of the cache. */
	PruneStart,
	/** Remove frames from the end of the cache. */
	PruneEnd,
	/** Remove all frames. */
	Empty,
};

UCLASS(BlueprintType)
class FASTMULTIPOLESIMULATION_API UFastMultipoleSimulationCache : public UObject
{
	GENERATED_BODY()

public:
	UFastMultipoleSimulationCache();
	virtual ~UFastMultipoleSimulationCache();

	int32 GetCapacity() const;
	void SetCapacity(int32 Capacity, EFastMultipoleCacheResizeMode ResizeMode);

	int32 GetNumFrames() const;

	FFastMultipoleSimulationFrame::ConstPtr GetFrame(int32 Step) const;
	FFastMultipoleSimulationFrame::ConstPtr GetLastFrame() const;

	void Reset();
	bool AddFrame(const FFastMultipoleSimulationFrame::ConstPtr& InFrame);

public:
	UPROPERTY(BlueprintAssignable)
	FFastMultipoleSimulationCacheResetDelegate OnReset;

	UPROPERTY(BlueprintAssignable)
	FFastMultipoleSimulationCacheFrameAddedDelegate OnFrameAdded;

private:
	int32 Capacity = 32;

	TArray<FFastMultipoleSimulationFrame::ConstPtr> Frames;

	mutable FRWLock FramesMutex;
};
