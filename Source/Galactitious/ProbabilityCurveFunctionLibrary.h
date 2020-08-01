// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Curves/CurveFloat.h"
#include "ProbabilityCurveFunctionLibrary.generated.h"

/**
 * 
 */
UCLASS()
class GALACTITIOUS_API UProbabilityCurveFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

    UFUNCTION(BlueprintCallable, Category = ProbabilityCurve)
    static UTexture2D* CurveToTexture2D(const FInterpCurveFloat& Curve);

    UFUNCTION(BlueprintCallable, Category = ProbabilityCurve)
    static void SampleCurveAsset(UCurveFloat *CurveAsset, FInterpCurveFloat& SampledCurve);
};
