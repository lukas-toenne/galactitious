// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "Kismet/BlueprintFunctionLibrary.h"

#include "GalaxySimulationNiagaraFunctionLibrary.generated.h"

class UNiagaraComponent;

/**
 *
 */
UCLASS()
class GALACTITIOUSNIAGARA_API UGalaxySimulationNiagaraFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	///** Get the galaxy simulation data interface by name .*/
	//static class UNiagaraDataInterfaceGalaxySimulation* GetGalaxySimulationDataInterface(
	//	UNiagaraComponent* NiagaraSystem, const FString& OverrideName);

	///** Sets a Niagara simulation cache parameter by name, overriding locally if necessary.*/
	//UFUNCTION(BlueprintCallable, Category = "Galaxy Simulation", meta = (DisplayName = "Set Niagara Galaxy Simulation Cache"))
	//static void OverrideSystemUserVariableGalaxySimulationCache(
	//	UNiagaraComponent* NiagaraSystem, const FString& OverrideName, class UFastMultipoleSimulationCache* SimulationCache);
};
