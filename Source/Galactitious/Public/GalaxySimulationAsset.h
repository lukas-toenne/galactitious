// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "GalaxySimulationAsset.generated.h"

class UFastMultipoleSimulationCache;

UCLASS(BlueprintType, NotBlueprintable, AutoExpandCategories = "Fast Multipole Simulation")
class GALACTITIOUS_API UGalaxySimulationAsset : public UObject
{
	GENERATED_BODY()

public:
	void SetSimulationCache(UFastMultipoleSimulationCache* InSimulationCache);
	const UFastMultipoleSimulationCache* GetSimulationCache() const { return SimulationCache; }
	UFastMultipoleSimulationCache* GetSimulationCache() { return SimulationCache; }

	virtual void BeginDestroy() override;
#if WITH_EDITOR
	virtual void PostDuplicate(EDuplicateMode::Type DuplicateMode) override;
	virtual void PostLoad() override;
	virtual void PreEditUndo() override;
	virtual void PostEditUndo() override;
	virtual void PreEditChange(FProperty* PropertyAboutToChange) override;
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR

private:
#if WITH_EDITOR
	void OnSimulationReset(UFastMultipoleSimulationCache* InSimulation);
	void OnSimulationStep(UFastMultipoleSimulationCache* InSimulation);
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
	UPROPERTY(EditAnywhere, Instanced)
	UFastMultipoleSimulationCache* SimulationCache = nullptr;
};
