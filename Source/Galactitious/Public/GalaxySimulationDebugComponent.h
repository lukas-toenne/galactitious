// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FastMultipoleSimulationFrame.h"

#include "Components/ActorComponent.h"

#include "GalaxySimulationDebugComponent.generated.h"

class AGalaxySimulationActor;
class UFastMultipoleSimulationCache;

USTRUCT()
struct GALACTITIOUS_API FGalaxySimulationSequencer
{
	GENERATED_BODY()

public:
	FGalaxySimulationSequencer();

	void ResetAnimation(const UFastMultipoleSimulationCache* SimulationCache);
	void StepAnimation(const UFastMultipoleSimulationCache* SimulationCache, float DeltaTime);

	const FFastMultipoleSimulationFrame& GetResultFrame() const { return ResultFrame; }

public:
	UPROPERTY(EditAnywhere)
	float AnimationSpeed = 1.0f;

private:
	void GetFrameInterval(
		const UFastMultipoleSimulationCache* SimulationCache, FFastMultipoleSimulationFramePtr& OutStartFrame,
		FFastMultipoleSimulationFramePtr& OutEndFrame) const;
	void UpdateResultFrame(const UFastMultipoleSimulationCache* SimulationCache);

private:
	int32 AnimCacheStep;
	float AnimationTime;
	FFastMultipoleSimulationFrame ResultFrame;
};

UCLASS(meta = (BlueprintSpawnableComponent))
class GALACTITIOUS_API UGalaxySimulationDebugComponent
	: public UActorComponent
	, public FGalaxySimulationSequencer
{
	GENERATED_BODY()

public:
	UGalaxySimulationDebugComponent(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	UPROPERTY(EditAnywhere)
	FColor PointColor = FColor::Silver;

protected:
	UFUNCTION()
	void OnSimulationStarted(AGalaxySimulationActor* SimulationActor);
	UFUNCTION()
	void OnSimulationStopped(AGalaxySimulationActor* SimulationActor);
	UFUNCTION()
	void OnCacheReset(UFastMultipoleSimulationCache* SimulationCache);
	UFUNCTION()
	void OnCacheFrameAdded(UFastMultipoleSimulationCache* SimulationCache);

	void LogPoints(const UFastMultipoleSimulationCache* SimulationCache) const;
	void ShowAnimatedPoints() const;
};
