// Fill out your copyright notice in the Description page of Project Settings.

#include "AssetTypeActions_GalaxySimulation.h"
#include "GalactitiousEditorModule.h"
#include "GalaxySimulationAsset.h"

#define LOCTEXT_NAMESPACE "WaterWaves"

FText FAssetTypeActions_GalaxySimulation::GetName() const
{
	return NSLOCTEXT("AssetTypeActions", "FAssetTypeActions_GalaxySimulation", "Galaxy Simulation");
}

UClass* FAssetTypeActions_GalaxySimulation::GetSupportedClass() const
{
	return UGalaxySimulationAsset::StaticClass();
}

uint32 FAssetTypeActions_GalaxySimulation::GetCategories()
{
	return FGalactitiousEditorModule::GetAssetCategory();
}

#undef LOCTEXT_NAMESPACE