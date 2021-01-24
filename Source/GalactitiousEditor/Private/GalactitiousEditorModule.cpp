// Fill out your copyright notice in the Description page of Project Settings.

#include "GalactitiousEditorModule.h"

#include "AssetToolsModule.h"
#include "AssetTypeActions_GalaxySimulation.h"
#include "IAssetTools.h"

#define LOCTEXT_NAMESPACE "GalactitiousEditor"

DEFINE_LOG_CATEGORY(LogGalactitiousEditor);

EAssetTypeCategories::Type FGalactitiousEditorModule::GalaxyAssetCategory;

void FGalactitiousEditorModule::StartupModule()
{
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	GalaxyAssetCategory = AssetTools.RegisterAdvancedAssetCategory(FName(TEXT("Galaxy")), LOCTEXT("GalaxyAssetCategory", "Galaxy"));

	// Helper lambda for registering asset type actions for automatic cleanup on shutdown
	auto RegisterAssetTypeAction = [&](TSharedRef<IAssetTypeActions> Action) {
		AssetTools.RegisterAssetTypeActions(Action);
		CreatedAssetTypeActions.Add(Action);
	};

	// Register type actions
	RegisterAssetTypeAction(MakeShareable(new FAssetTypeActions_GalaxySimulation));
}

void FGalactitiousEditorModule::ShutdownModule()
{
	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
		for (auto CreatedAssetTypeAction : CreatedAssetTypeActions)
		{
			AssetTools.UnregisterAssetTypeActions(CreatedAssetTypeAction.ToSharedRef());
		}
	}
	CreatedAssetTypeActions.Empty();
}

IMPLEMENT_MODULE(FGalactitiousEditorModule, GalactitiousEditor);

#undef LOCTEXT_NAMESPACE
