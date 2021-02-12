// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FastMultipoleSimulationFrame.h"

#include "GalaxySimulationCachePlayer.generated.h"

class UFastMultipoleSimulationCache;

USTRUCT()
struct GALACTITIOUS_API FGalaxySimulationCachePlayer
{
	GENERATED_BODY()

public:
	FGalaxySimulationCachePlayer();

	int32 GetCacheStep() const { return AnimCacheStep; }
	float GetAnimationTime() const { return AnimationTime; }
	const FFastMultipoleSimulationFrame& GetResultFrame() const { return ResultFrame; }

	void ResetAnimation(const UFastMultipoleSimulationCache* SimulationCache);
	void StepAnimation(const UFastMultipoleSimulationCache* SimulationCache, float DeltaTime);

public:
	UPROPERTY(EditAnywhere)
	float AnimationSpeed = 1.0f;

private:
	void GetFrameInterval(
		const UFastMultipoleSimulationCache* SimulationCache, FFastMultipoleSimulationFrame::ConstPtr& OutStartFrame,
		FFastMultipoleSimulationFrame::ConstPtr& OutEndFrame) const;
	void UpdateResultFrame(const UFastMultipoleSimulationCache* SimulationCache);

private:
	int32 AnimCacheStep;
	float AnimationTime;
	FFastMultipoleSimulationFrame ResultFrame;
};
