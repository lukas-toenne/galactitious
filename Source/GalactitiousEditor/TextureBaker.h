// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture.h"

struct FVector4_16
{
	FFloat16 X;
	FFloat16 Y;
	FFloat16 Z;
	FFloat16 W;
};

struct GALACTITIOUSEDITOR_API FTextureBaker
{
public:
	template <typename ValueType>
	static UTexture2D* BakeTexture(
		const FString& Name, int32 Resolution, EPixelFormat PixelFormat, ETextureSourceFormat SourceFormat,
		TextureMipGenSettings MipGenSettings, TFunctionRef<ValueType(float X)> ValueFn)
	{
		const int32 BytesPerPixel = FTextureSource::GetBytesPerPixel(SourceFormat);
		check(sizeof(ValueType) == BytesPerPixel);
		return BakeTextureInternal(
			Name, Resolution, PixelFormat, SourceFormat, MipGenSettings, [ValueFn, BytesPerPixel](float X, uint8* OutData) {
				ValueType Value = ValueFn(X);
				memcpy(OutData, &Value, BytesPerPixel);
			});
	}

private:
	static UTexture2D* BakeTextureInternal(
		const FString& Name, int32 Resolution, EPixelFormat PixelFormat, ETextureSourceFormat SourceFormat,
		TextureMipGenSettings MipGenSettings, TFunctionRef<void(float X, uint8* OutData)> ValueFn);
};
