// Fill out your copyright notice in the Description page of Project Settings.

#include "GalaxySimulationAsset.h"

#include "FastMultipoleSimulation.h"

void UGalaxySimulationAsset::SetSimulation(UFastMultipoleSimulation* InSimulation)
{
	if (InSimulation != Simulation)
	{
#if WITH_EDITOR
		RegisterOnUpdateSimulation(Simulation, /*bRegister = */ false);
#endif // WITH_EDITOR

		Simulation = InSimulation;

#if WITH_EDITOR
		RegisterOnUpdateSimulation(Simulation, /*bRegister = */ true);
		OnUpdateSimulationAssetData.Broadcast(this, EPropertyChangeType::ValueSet);
#endif // WITH_EDITOR
	}
}

void UGalaxySimulationAsset::BeginDestroy()
{
	Super::BeginDestroy();

#if WITH_EDITOR
	RegisterOnUpdateSimulation(Simulation, /*bRegister = */ false);
#endif // WITH_EDITOR
}

#if WITH_EDITOR
void UGalaxySimulationAsset::PostDuplicate(EDuplicateMode::Type DuplicateMode)
{
	Super::PostDuplicate(DuplicateMode);

	RegisterOnUpdateSimulation(Simulation, /*bRegister = */ true);
}

void UGalaxySimulationAsset::PostLoad()
{
	Super::PostLoad();

	RegisterOnUpdateSimulation(Simulation, /*bRegister = */ true);
}

void UGalaxySimulationAsset::PreEditUndo()
{
	Super::PreEditUndo();

	// On undo, when PreEditChange is called, PropertyAboutToChange is nullptr so we need to unregister from the previous object here :
	RegisterOnUpdateSimulation(Simulation, /*bRegister = */ false);
}

void UGalaxySimulationAsset::PostEditUndo()
{
	Super::PostEditUndo();

	// On undo, when PostEditChangeProperty is called, PropertyChangedEvent is fake so we need to register to the new object here :
	RegisterOnUpdateSimulation(Simulation, /*bRegister = */ true);
}

void UGalaxySimulationAsset::PreEditChange(FProperty* PropertyAboutToChange)
{
	Super::PreEditChange(PropertyAboutToChange);

	const FName PropertyName = PropertyAboutToChange ? PropertyAboutToChange->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UGalaxySimulationAsset, Simulation))
	{
		RegisterOnUpdateSimulation(Simulation, /*bRegister = */ false);
	}
}

void UGalaxySimulationAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UGalaxySimulationAsset, Simulation))
	{
		RegisterOnUpdateSimulation(Simulation, /*bRegister = */ true);
	}

	OnUpdateSimulationAssetData.Broadcast(this, PropertyChangedEvent.ChangeType);
}

void UGalaxySimulationAsset::OnSimulationReset(UFastMultipoleSimulation* InSimulation)
{
	// There was a data change on our internal data, just forward the event :
	OnUpdateSimulationAssetData.Broadcast(this, EPropertyChangeType::Unspecified);
}

void UGalaxySimulationAsset::OnSimulationStep(UFastMultipoleSimulation* InSimulation)
{
	// There was a data change on our internal data, just forward the event :
	OnUpdateSimulationAssetData.Broadcast(this, EPropertyChangeType::Unspecified);
}

void UGalaxySimulationAsset::RegisterOnUpdateSimulation(UFastMultipoleSimulation* InSimulation, bool bRegister)
{
	if (InSimulation != nullptr)
	{
		if (bRegister)
		{
			InSimulation->OnSimulationReset.AddDynamic(this, &UGalaxySimulationAsset::OnSimulationReset);
			InSimulation->OnSimulationStep.AddDynamic(this, &UGalaxySimulationAsset::OnSimulationStep);
		}
		else
		{
			InSimulation->OnSimulationReset.RemoveAll(this);
			InSimulation->OnSimulationStep.RemoveAll(this);
		}
	}
}

#endif // WITH_EDITOR
