// Fill out your copyright notice in the Description page of Project Settings.

#include "GalaxySimulationNiagaraFunctionLibrary.h"

#include "FastMultipoleSimulationCache.h"
#include "NiagaraDataInterfaceGalaxySimulation.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogGalaxySimulationNiagara, Log, All);

//UNiagaraDataInterfaceGalaxySimulation* UGalaxySimulationNiagaraFunctionLibrary::GetGalaxySimulationDataInterface(
//	UNiagaraComponent* NiagaraSystem, const FString& OverrideName)
//{
//	if (!NiagaraSystem)
//	{
//		return nullptr;
//	}
//
//	const FNiagaraParameterStore& OverrideParameters = NiagaraSystem->GetOverrideParameters();
//	FNiagaraVariable Variable(FNiagaraTypeDefinition(UNiagaraDataInterfaceGalaxySimulation::StaticClass()), *OverrideName);
//
//	const int32 Index = OverrideParameters.IndexOf(Variable);
//	return Index != INDEX_NONE ? Cast<UNiagaraDataInterfaceGalaxySimulation>(OverrideParameters.GetDataInterface(Index)) : nullptr;
//}

//void UGalaxySimulationNiagaraFunctionLibrary::OverrideSystemUserVariableGalaxySimulationCache(
//	UNiagaraComponent* NiagaraSystem, const FString& OverrideName, UFastMultipoleSimulationCache* SimulationCache)
//{
//	if (!NiagaraSystem)
//	{
//		UE_LOG(
//			LogGalaxySimulationNiagara, Warning,
//			TEXT("NiagaraSystem in \"Set Niagara Galaxy Simulation Cache\" is NULL, OverrideName \"%s\" and SimulationCache \"%s\", "
//				 "skipping."),
//			*OverrideName, *GetFullNameSafe(SimulationCache));
//		return;
//	}
//
//	if (!SimulationCache)
//	{
//		UE_LOG(
//			LogGalaxySimulationNiagara, Warning,
//			TEXT("SimulationCache in \"Set Niagara Galaxy Simulation Cache\" is NULL, OverrideName \"%s\" and NiagaraSystem \"%s\", "
//				 "skipping."),
//			*OverrideName, *GetFullNameSafe(NiagaraSystem));
//		return;
//	}
//
//	UNiagaraDataInterfaceGalaxySimulation* DataInterface = GetGalaxySimulationDataInterface(NiagaraSystem, OverrideName);
//	if (!DataInterface)
//	{
//		UE_LOG(
//			LogGalaxySimulationNiagara, Warning,
//			TEXT("Did not find a matching Galaxy Simulation Data Interface variable named \"%s\" in the User variables of NiagaraSystem \"%s\" "
//				 "."),
//			*OverrideName, *GetFullNameSafe(NiagaraSystem));
//		return;
//	}
//
//	DataInterface->SetSimulationCache(SimulationCache);
//}
