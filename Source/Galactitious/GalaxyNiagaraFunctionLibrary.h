// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GalaxyNiagaraFunctionLibrary.generated.h"

/**
 * 
 */
UCLASS()
class GALACTITIOUS_API UGalaxyNiagaraFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static void SetCurveParameter(
		class UNiagaraParameterCollectionInstance* NiagaraParameters, const FString& Name, const FRichCurve& Value, bool bOverride = true);
	static void SetFloatParameter(
		class UNiagaraParameterCollectionInstance* NiagaraParameters, const FString& Name, float Value, bool bOverride = true);

	static void ApplyParameterCollectionUpdate(
		class UNiagaraParameterCollectionInstance* NiagaraParameters,
		TFunctionRef<void(class UNiagaraParameterCollectionInstance* UpdatedParameters)> ApplyFn);
};
