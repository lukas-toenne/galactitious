// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "Factories/Factory.h"
#include "UObject/ObjectMacros.h"

#include "GalaxySimulationAssetFactory.generated.h"

UCLASS()
class UGalaxySimulationAssetFactory : public UFactory
{
	GENERATED_BODY()

public:
	UGalaxySimulationAssetFactory();

	// UFactory interface
	virtual UObject* FactoryCreateNew(
		UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	virtual FText GetToolTip() const override;
	// End of UFactory interface
};
