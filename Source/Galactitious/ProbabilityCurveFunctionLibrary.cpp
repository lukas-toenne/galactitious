// Fill out your copyright notice in the Description page of Project Settings.


#include "ProbabilityCurveFunctionLibrary.h"

UTexture2D* UProbabilityCurveFunctionLibrary::CurveToTexture2D(const FInterpCurveFloat& SampledCurve)
{
	int32 Width = SampledCurve.Points.Num();
	int32 Height = 1;
	EPixelFormat PixelFormat = PF_R32_FLOAT;

	UTexture2D* NewTexture = UTexture2D::CreateTransient(Width, Height, PixelFormat);
	if (NewTexture)
	{
		NewTexture->MipGenSettings = TMGS_NoMipmaps;

		{
			float* MipData = static_cast<float*>(NewTexture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE));
			for (const FInterpCurvePointFloat& Point : SampledCurve.Points)
			{
				*(MipData++) = Point.OutVal;
			}

			NewTexture->PlatformData->Mips[0].BulkData.Unlock();
		}

		NewTexture->UpdateResource();
	}

	return NewTexture;
}

bool UProbabilityCurveFunctionLibrary::SampleCurveAsset(UCurveFloat* CurveAsset, int32 Resolution, FInterpCurveFloat& SampledCurve)
{
	SampledCurve.bIsLooped = false;
	SampledCurve.LoopKeyOffset = 0.0f;

	if (Resolution < 1)
	{
		SampledCurve.Points.Empty();
		return false;
	}

	SampledCurve.Points.SetNum(Resolution);

	const FRichCurve& FloatCurve = CurveAsset->FloatCurve;
	float MinTime, MaxTime;
	FloatCurve.GetTimeRange(MinTime, MaxTime);
	const float DeltaX = Resolution > 1 ? (MaxTime - MinTime) / (float)(Resolution - 1) : MaxTime - MinTime;
	const float InvDeltaX = 1.0f / DeltaX;

	float X = MinTime;
	float Value = CurveAsset->FloatCurve.Eval(X);
	float PrevValue = Value;
	float NextValue = CurveAsset->FloatCurve.Eval(X + DeltaX);
	for (FInterpCurvePointFloat& Point : SampledCurve.Points)
	{
		Point.InVal = X;
		Point.OutVal = NextValue;
		Point.ArriveTangent = (Value - PrevValue) * InvDeltaX;
		Point.LeaveTangent = (NextValue - Value) * InvDeltaX;
		Point.InterpMode = EInterpCurveMode::CIM_Linear;

		X += DeltaX;
		PrevValue = Value;
		Value = NextValue;
		NextValue = CurveAsset->FloatCurve.Eval(X + DeltaX);
	}

	return true;
}
