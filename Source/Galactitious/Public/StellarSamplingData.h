// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"

#include "StellarSamplingData.generated.h"

USTRUCT(BlueprintType)
struct GALACTITIOUS_API FStellarClass : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	float MinTemperature = 0.0f;

	UPROPERTY(EditAnywhere)
	float MinMass = 0.0f;

	UPROPERTY(EditAnywhere)
	float MinRadius = 0.0f;

	UPROPERTY(EditAnywhere)
	float MinLuminosity = 0.0f;

	UPROPERTY(EditAnywhere)
	float Fraction = 0.0f;
};

UCLASS(BlueprintType)
class GALACTITIOUS_API UGalaxyShapeSettings : public UDataAsset
{
	GENERATED_BODY()

	UFUNCTION(BlueprintCallable, CallInEditor)
	void UpdateNiagaraParameters();

	UPROPERTY(EditAnywhere)
	class UNiagaraParameterCollectionInstance* NiagaraParameters;

	UPROPERTY(EditAnywhere)
	float Radius = 100.0f;

	UPROPERTY(EditAnywhere)
	float Velocity = 10.0f;

	UPROPERTY(EditAnywhere)
	float Perturbation = 0.7f;

	UPROPERTY(EditAnywhere)
	float WindingFrequency = -1.5f;

	UPROPERTY(EditAnywhere)
	UCurveFloat* RadialDensityCurve;

	UPROPERTY(EditAnywhere)
	UCurveFloat* ThicknessCurve;
};

UCLASS(BlueprintType)
class GALACTITIOUS_API UStarSettings : public UDataAsset
{
	GENERATED_BODY()

	UFUNCTION(BlueprintCallable, CallInEditor)
	void UpdateNiagaraParameters();

	UPROPERTY(EditAnywhere)
	class UNiagaraParameterCollectionInstance* NiagaraParameters;

	UPROPERTY(EditAnywhere, meta = (RequiredAssetDataTags = "RowStructure=StellarClass"))
	UDataTable* StellarClassesTable;
};
