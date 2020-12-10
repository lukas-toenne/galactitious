// Fill out your copyright notice in the Description page of Project Settings.

#include "TelescopeData.h"

#include "TextureBakerFunctionLibrary.h"

namespace
{
	FVector2D BesselRe(int32 Alpha, float X)
	{
		float Arg = X - Alpha * PI / 2.0f - PI / 4.0f;
		float SinArg, CosArg;
		FMath::SinCos(&SinArg, &CosArg, Arg);
		return FVector2D(CosArg, SinArg) / FMath::Sqrt(2.0f * PI * X);
	}

	float AiryDiskIntensity(float X)
	{
		if (FMath::IsNearlyZero(X))
		{
			return 1.0f;
		}

		FVector2D J = BesselRe(1, X);
		return 2.0f * (J.X * J.X - J.Y * J.Y) / X;
	}
}

#if WITH_EDITOR
void UTelescopeData::BakeTextures()
{
	if (!ensureMsgf(AiryDiskTexture != nullptr, TEXT("Airy disk texture not set")))
	{
		return;
	}

	AiryDiskTexture = UTextureBakerFunctionLibrary::BakeTextureAsset<FVector4_16>(
		AiryDiskTexture->GetPathName(), 1024, 1024, PF_A32B32G32R32F, TSF_RGBA16F, TMGS_SimpleAverage, [this](float X, float Y) -> FVector4_16 {
			FVector4_16 Result;

			const FVector2D p = FVector2D(X - 0.5f, Y - 0.5f) * 2.0f * AiryDiskScale;
			const float q = p.Size();

			//const float sigma = 0.42f;
			//const float I = FMath::Exp(-q * q / (2.0f * sigma * sigma));

			const float I = AiryDiskIntensity(q);

			Result.X = I;
			Result.Y = I;
			Result.Z = I;
			Result.W = 1.0f;
			return Result;
		});
}
#endif
