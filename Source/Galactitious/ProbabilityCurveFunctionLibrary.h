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
    static UTexture2D* CurveToTexture2D(const FInterpCurveFloat& Curve, int32 Resolution);

    UFUNCTION(BlueprintCallable, Category = ProbabilityCurve)
    static void ConvertCurveAsset(UCurveFloat *CurveAsset, FInterpCurveFloat& Curve);

    UFUNCTION(BlueprintCallable, Category = ProbabilityCurve)
    static void IntegrateCurve(const FInterpCurveFloat& Curve, float Offset, FInterpCurveFloat& IntegratedCurve);

    UFUNCTION(BlueprintCallable, Category = ProbabilityCurve)
    static void NormalizeCurve(const FInterpCurveFloat& Curve, FInterpCurveFloat& NormalizedCurve);

    UFUNCTION(BlueprintCallable, Category = ProbabilityCurve)
    static void InvertCurve(const FInterpCurveFloat& Curve, FInterpCurveFloat& InvertedCurve);
};
