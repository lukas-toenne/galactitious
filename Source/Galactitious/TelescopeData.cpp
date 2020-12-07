// Fill out your copyright notice in the Description page of Project Settings.

#include "TelescopeData.h"

#if WITH_EDITOR
#include "GalactitiousEditor/TextureBaker.h"
#endif

#if WITH_EDITOR
void UTelescopeData::BakeTextures()
{
	if (!ensureMsgf(AiryDiskTexture != nullptr, TEXT("Airy disk texture not set")))
	{
		return;
	}

	AiryDiskTexture = FTextureBaker::BakeTexture<FVector4_16>(
		AiryDiskTexture->GetPathName(), 512, 1, PF_A32B32G32R32F, TSF_RGBA16F, TMGS_SimpleAverage, [](float X, float Y) -> FVector4_16 {
			FVector4_16 Result;
			Result.X = 1.0f;
			Result.Y = 0.5f;
			Result.Z = 0.0f;
			Result.W = 1.0f;
			return Result;
		});
}
#endif
