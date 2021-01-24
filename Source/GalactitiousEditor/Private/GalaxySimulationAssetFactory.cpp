// Fill out your copyright notice in the Description page of Project Settings.

#include "GalaxySimulationAssetFactory.h"

#include "AssetTypeCategories.h"
#include "GalaxySimulationAsset.h"

#define LOCTEXT_NAMESPACE "GalactitiousSimulationAssetFactory"

UGalaxySimulationAssetFactory::UGalaxySimulationAssetFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UGalaxySimulationAsset::StaticClass();
}

UObject* UGalaxySimulationAssetFactory::FactoryCreateNew(
	UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UGalaxySimulationAsset* NewAsset = NewObject<UGalaxySimulationAsset>(InParent, Class, Name, Flags | RF_Transactional);
	return NewAsset;
}

FText UGalaxySimulationAssetFactory::GetToolTip() const
{
	return LOCTEXT("GalacitiousSimulationTooltip", "Galaxy Simulation Asset");
}

#undef LOCTEXT_NAMESPACE
