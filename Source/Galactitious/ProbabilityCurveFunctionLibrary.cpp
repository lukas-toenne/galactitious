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
			//// Bulk data was already allocated for the correct size when we called CreateTransient above
			//FMemory::Memcpy(MipData, UncompressedData.GetData(), NewTexture->PlatformData->Mips[0].BulkData.GetBulkDataSize());

			NewTexture->PlatformData->Mips[0].BulkData.Unlock();
		}

		NewTexture->UpdateResource();
	}

	return NewTexture;
}

void UProbabilityCurveFunctionLibrary::SampleCurveAsset(UCurveFloat* CurveAsset, FInterpCurveFloat& SampledCurve)
{
	//CurveAsset->FloatCurve
}
