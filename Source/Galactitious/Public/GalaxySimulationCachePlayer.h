// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FastMultipoleSimulationFrame.h"

#include "Engine/EngineBaseTypes.h"

#include "GalaxySimulationCachePlayer.generated.h"

class UFastMultipoleSimulationCache;

UCLASS(BlueprintType)
class GALACTITIOUS_API UGalaxySimulationCachePlayer : public UObject
{
	GENERATED_BODY()

public:
	UGalaxySimulationCachePlayer();
	virtual ~UGalaxySimulationCachePlayer();

	void SetSimulationCache(UFastMultipoleSimulationCache* SimulationCache);

	UFUNCTION(BlueprintGetter)
	int32 GetCacheStep() const { return AnimCacheStep; }

	UFUNCTION(BlueprintGetter)
	float GetAnimationTime() const { return AnimationTime; }

	const FFastMultipoleSimulationFrame& GetResultFrame() const { return ResultFrame; }

	UFUNCTION(BlueprintCallable, Category = "Fast Multipole Cache Player")
	void Play();

	UFUNCTION(BlueprintCallable, Category = "Fast Multipole Cache Player")
	bool IsPlaying() const;

	UFUNCTION(BlueprintCallable, Category = "Fast Multipole Cache Player")
	void Pause();

	UFUNCTION(BlueprintCallable, Category = "Fast Multipole Cache Player")
	void StepForward(float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = "Fast Multipole Cache Player")
	void StepBack(float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = "Fast Multipole Cache Player")
	float GetTime() const;

	UFUNCTION(BlueprintCallable, Category = "Fast Multipole Cache Player")
	void SetTime(float Time);

	UFUNCTION(BlueprintCallable, Category = "Fast Multipole Cache Player")
	void SetToFront();

	UFUNCTION(BlueprintCallable, Category = "Fast Multipole Cache Player")
	void SetToBack();

public:
	UPROPERTY(EditAnywhere)
	float AnimationSpeed = 1.0f;

private:
	void GetFrameInterval(
		const UFastMultipoleSimulationCache* SimulationCache, FFastMultipoleSimulationFrame::ConstPtr& OutStartFrame,
		FFastMultipoleSimulationFrame::ConstPtr& OutEndFrame) const;
	void UpdateResultFrame(const UFastMultipoleSimulationCache* SimulationCache);

	void OnWorldTick_StepForward(UWorld* World, ELevelTick TickType, float DeltaTime);
	UFUNCTION()
	void OnCacheReset(UFastMultipoleSimulationCache* SimulationCache);
	UFUNCTION()
	void OnCacheFrameAdded(UFastMultipoleSimulationCache* SimulationCache);

private:
	TWeakObjectPtr<UFastMultipoleSimulationCache> SimulationCacheWeak;

	UPROPERTY(BlueprintGetter = GetCacheStep, Category = "Fast Multipole Cache Player")
	int32 AnimCacheStep;

	UPROPERTY(BlueprintGetter = GetAnimationTime, Category = "Fast Multipole Cache Player")
	float AnimationTime;

	FFastMultipoleSimulationFrame ResultFrame;

	FDelegateHandle PlayDelegateHandle;
};
