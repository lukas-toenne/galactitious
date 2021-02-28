// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FastMultipoleOpenVDBGuardEnter.h"
#include <openvdb/openvdb.h>
#include <openvdb/points/PointDataGrid.h>
#include "FastMultipoleOpenVDBGuardLeave.h"
#include "FastMultipoleOpenVDBTypes.h"
#include "FastMultipoleSimulationFrame.h"
#include "FastMultipoleTypes.h"

#include "FastMultipoleSimulationCache.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogFastMultipoleSimulationCache, Log, All);

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
	DECLARE_MULTICAST_DELEGATE_OneParam(FResetDelegate, UFastMultipoleSimulationCache*);
	DECLARE_MULTICAST_DELEGATE_OneParam(FFrameAddedDelegate, UFastMultipoleSimulationCache*);

	UFastMultipoleSimulationCache();
	virtual ~UFastMultipoleSimulationCache();

	UFUNCTION(BlueprintGetter)
	int32 GetCapacity() const;
	UFUNCTION(BlueprintCallable, Category = "Fast Multipole Simulation")
	void SetCapacity(int32 Capacity, EFastMultipoleCacheResizeMode ResizeMode);

	UFUNCTION(BlueprintCallable, Category = "Fast Multipole Simulation")
	int32 GetNumFrames() const;

	FFastMultipoleSimulationFrame::ConstPtr GetFrame(int32 Step) const;
	FFastMultipoleSimulationFrame::ConstPtr GetLastFrame() const;

	UFUNCTION(BlueprintCallable, Category = "Fast Multipole Simulation")
	void Reset();

	bool AddFrame(const FFastMultipoleSimulationFrame::ConstPtr& InFrame);

public:
	FResetDelegate OnReset;
	FFrameAddedDelegate OnFrameAdded;

private:
	UPROPERTY(EditAnywhere, BlueprintGetter = GetCapacity)
	int32 Capacity = 32;

	TArray<FFastMultipoleSimulationFrame::ConstPtr> Frames;

	mutable FRWLock FramesMutex;
};
