// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Curves/CurveFloat.h"
#include "ProbabilityCurveFunctionLibrary.generated.h"

USTRUCT(BlueprintType)
struct FDrawDebugCurveSettings
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 CurveResolution = 12;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float PointSize = 3.0f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FColor PointColor = FColor::White;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float LineThickness = 0.f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FColor LineColor = FColor(170, 170, 170, 255);

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float TangentThickness = 0.f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FColor TangentColor = FColor::Yellow;
};

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

	UFUNCTION(BlueprintCallable, Category = ProbabilityCurve, meta = (WorldContext = "WorldContextObject"))
	static void DrawDebugCurve(
		const UObject* WorldContextObject, const FInterpCurveFloat& Curve, const FTransform& Transform,
		const FDrawDebugCurveSettings& Settings, bool bPersistentLines = false, float LifeTime = -1.f, uint8 DepthPriority = 0);
};
