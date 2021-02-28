// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FastMultipoleTypes.h"

#include "GalaxySimulationAsset.generated.h"

class UFastMultipoleSimulationCache;

UCLASS(BlueprintType, NotBlueprintable, AutoExpandCategories = "Fast Multipole Simulation")
class GALACTITIOUS_API UGalaxySimulationAsset : public UObject
{
	GENERATED_BODY()

public:
	UGalaxySimulationAsset();
	virtual ~UGalaxySimulationAsset();

	UFUNCTION(BlueprintGetter)
	UFastMultipoleSimulationCache* GetSimulationCache() const { return SimulationCache; }

	virtual void BeginDestroy() override;
#if WITH_EDITOR
	virtual void PostDuplicate(EDuplicateMode::Type DuplicateMode) override;
	virtual void PostLoad() override;
	virtual void PreEditUndo() override;
	virtual void PostEditUndo() override;
	virtual void PreEditChange(FProperty* PropertyAboutToChange) override;
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR

public:
	/** Softening radius of the gravitational potential to avoid instabilities at small distances. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FFastMultipoleSimulationSettings SimulationSettings;

private:
#if WITH_EDITOR
	void OnCacheReset(UFastMultipoleSimulationCache* InSimulation);
	void OnCacheFrameAdded(UFastMultipoleSimulationCache* InSimulation);
	void RegisterOnUpdateSimulation(UFastMultipoleSimulationCache* InSimulation, bool bRegister);
#endif // WITH_EDITOR

public:
#if WITH_EDITORONLY_DATA
	// Delegate called whenever the simulation is updated
	DECLARE_MULTICAST_DELEGATE_TwoParams(
		FOnUpdateSimulationAssetData, UGalaxySimulationAsset* /*SimulationAsset*/, EPropertyChangeType::Type /*ChangeType*/);
	FOnUpdateSimulationAssetData OnUpdateSimulationAssetData;
#endif // WITH_EDITORONLY_DATA

private:
	UPROPERTY(EditAnywhere, BlueprintGetter = "GetSimulationCache")
	UFastMultipoleSimulationCache* SimulationCache = nullptr;

	FDelegateHandle CacheResetHandle;
	FDelegateHandle CacheFrameAddedHandle;
};
