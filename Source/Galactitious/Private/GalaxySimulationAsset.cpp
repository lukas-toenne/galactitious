// Fill out your copyright notice in the Description page of Project Settings.

#include "GalaxySimulationAsset.h"

#include "FastMultipoleSimulationCache.h"

//void UGalaxySimulationAsset::SetSimulationCache(UFastMultipoleSimulationCache* InSimulationCache)
//{
//	if (InSimulationCache != SimulationCache)
//	{
//#if WITH_EDITOR
//		RegisterOnUpdateSimulation(SimulationCache, /*bRegister = */ false);
//#endif // WITH_EDITOR
//
//		SimulationCache = InSimulationCache;
//
//#if WITH_EDITOR
//		RegisterOnUpdateSimulation(SimulationCache, /*bRegister = */ true);
//		OnUpdateSimulationAssetData.Broadcast(this, EPropertyChangeType::ValueSet);
//#endif // WITH_EDITOR
//	}
//}

UGalaxySimulationAsset::UGalaxySimulationAsset()
{
	SimulationCache = CreateDefaultSubobject<UFastMultipoleSimulationCache>(TEXT("SimulationCache"));
}

UGalaxySimulationAsset::~UGalaxySimulationAsset()
{
}

void UGalaxySimulationAsset::BeginDestroy()
{
	Super::BeginDestroy();

#if WITH_EDITOR
	RegisterOnUpdateSimulation(SimulationCache, /*bRegister = */ false);
#endif // WITH_EDITOR
}

#if WITH_EDITOR
void UGalaxySimulationAsset::PostDuplicate(EDuplicateMode::Type DuplicateMode)
{
	Super::PostDuplicate(DuplicateMode);

	RegisterOnUpdateSimulation(SimulationCache, /*bRegister = */ true);
}

void UGalaxySimulationAsset::PostLoad()
{
	Super::PostLoad();

	RegisterOnUpdateSimulation(SimulationCache, /*bRegister = */ true);
}

void UGalaxySimulationAsset::PreEditUndo()
{
	Super::PreEditUndo();

	// On undo, when PreEditChange is called, PropertyAboutToChange is nullptr so we need to unregister from the previous object here :
	RegisterOnUpdateSimulation(SimulationCache, /*bRegister = */ false);
}

void UGalaxySimulationAsset::PostEditUndo()
{
	Super::PostEditUndo();

	// On undo, when PostEditChangeProperty is called, PropertyChangedEvent is fake so we need to register to the new object here :
	RegisterOnUpdateSimulation(SimulationCache, /*bRegister = */ true);
}

void UGalaxySimulationAsset::PreEditChange(FProperty* PropertyAboutToChange)
{
	Super::PreEditChange(PropertyAboutToChange);

	const FName PropertyName = PropertyAboutToChange ? PropertyAboutToChange->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UGalaxySimulationAsset, SimulationCache))
	{
		RegisterOnUpdateSimulation(SimulationCache, /*bRegister = */ false);
	}
}

void UGalaxySimulationAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UGalaxySimulationAsset, SimulationCache))
	{
		RegisterOnUpdateSimulation(SimulationCache, /*bRegister = */ true);
	}

	OnUpdateSimulationAssetData.Broadcast(this, PropertyChangedEvent.ChangeType);
}

void UGalaxySimulationAsset::OnCacheReset(UFastMultipoleSimulationCache* InSimulationCache)
{
	// There was a data change on our internal data, just forward the event :
	OnUpdateSimulationAssetData.Broadcast(this, EPropertyChangeType::Unspecified);
}

void UGalaxySimulationAsset::OnCacheFrameAdded(UFastMultipoleSimulationCache* InSimulationCache)
{
	// There was a data change on our internal data, just forward the event :
	OnUpdateSimulationAssetData.Broadcast(this, EPropertyChangeType::Unspecified);
}

void UGalaxySimulationAsset::RegisterOnUpdateSimulation(UFastMultipoleSimulationCache* InSimulationCache, bool bRegister)
{
	if (InSimulationCache != nullptr)
	{
		if (bRegister)
		{
			CacheResetHandle = InSimulationCache->OnReset.AddUObject(this, &UGalaxySimulationAsset::OnCacheReset);
			CacheFrameAddedHandle = InSimulationCache->OnFrameAdded.AddUObject(this, &UGalaxySimulationAsset::OnCacheFrameAdded);
		}
		else
		{
			InSimulationCache->OnReset.Remove(CacheResetHandle);
			InSimulationCache->OnFrameAdded.Remove(CacheFrameAddedHandle);
		}
	}
}

#endif // WITH_EDITOR
