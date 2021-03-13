// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RemoteSimulationFrame.h"

#include "RemoteSimulationCache.generated.h"

UENUM()
enum class ERemoteSimulationCacheResizeMode : uint8
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
class REMOTESIMULATION_API URemoteSimulationCache : public UObject
{
	GENERATED_BODY()

public:
	DECLARE_MULTICAST_DELEGATE_OneParam(FResetDelegate, URemoteSimulationCache*);
	DECLARE_MULTICAST_DELEGATE_OneParam(FFrameAddedDelegate, URemoteSimulationCache*);

	URemoteSimulationCache();
	virtual ~URemoteSimulationCache();

	UFUNCTION(BlueprintGetter)
	int32 GetCapacity() const;
	UFUNCTION(BlueprintCallable, Category = "Remote Simulation")
	void SetCapacity(int32 Capacity, ERemoteSimulationCacheResizeMode ResizeMode);

	UFUNCTION(BlueprintCallable, Category = "Remote Simulation")
	int32 GetNumFrames() const;

	FRemoteSimulationFrame::ConstPtr GetFrame(int32 Step) const;
	FRemoteSimulationFrame::ConstPtr GetLastFrame() const;

	UFUNCTION(BlueprintCallable, Category = "Remote Simulation")
	void Reset();

	bool AddFrame(const FRemoteSimulationFrame::ConstPtr& InFrame);

public:
	FResetDelegate OnReset;
	FFrameAddedDelegate OnFrameAdded;

private:
	UPROPERTY(EditAnywhere, BlueprintGetter = GetCapacity)
	int32 Capacity = 32;

	TArray<FRemoteSimulationFrame::ConstPtr> Frames;

	mutable FRWLock FramesMutex;
};
