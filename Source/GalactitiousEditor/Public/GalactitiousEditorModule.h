// Fill out your copyright notice in the Description page of Project Settings.

#include "Modules/ModuleManager.h"

#include "AssetTypeCategories.h"
#include "IAssetTypeActions.h"

GALACTITIOUSEDITOR_API DECLARE_LOG_CATEGORY_EXTERN(LogGalactitiousEditor, Log, All);

class FGalactitiousEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;

	virtual void ShutdownModule() override;

	static EAssetTypeCategories::Type GetAssetCategory() { return GalaxyAssetCategory; }

private:
	/** Array of component class names we have registered, so we know what to unregister afterwards */
	TArray<FName> RegisteredComponentClassNames;

	static EAssetTypeCategories::Type GalaxyAssetCategory;

	TArray<TSharedPtr<IAssetTypeActions>> CreatedAssetTypeActions;
};
