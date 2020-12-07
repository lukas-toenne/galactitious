// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"

#include "StellarSamplingData.generated.h"

USTRUCT(Blueprintable, BlueprintType)
struct GALACTITIOUS_API FStellarClass : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float MinTemperature = 0.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float MinMass = 0.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float MinRadius = 0.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float MinLuminosity = 0.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float Fraction = 0.0f;
};

UCLASS(BlueprintType)
class GALACTITIOUS_API UStellarSamplingData : public UDataAsset
{
public:
	GENERATED_BODY()

	UStellarSamplingData();

	UPROPERTY(EditAnywhere, meta = (RequiredAssetDataTags = "RowStructure=StellarClass"))
	UDataTable* StellarClassesTable;

	UPROPERTY(EditAnywhere)
	float AverageLuminosity;

	UPROPERTY(EditAnywhere)
	UTexture2D* SamplingTexture;

#if WITH_EDITOR
	UFUNCTION(BlueprintCallable, CallInEditor)
	void BuildFromStellarClasses();
#endif
};
