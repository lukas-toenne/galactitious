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
		const FString& TexturePath, int32 Width, int32 Height, EPixelFormat PixelFormat, ETextureSourceFormat SourceFormat,
		TextureMipGenSettings MipGenSettings, TFunctionRef<ValueType(float X, float Y)> ValueFn)
	{
		const int32 BytesPerPixel = FTextureSource::GetBytesPerPixel(SourceFormat);
		check(sizeof(ValueType) == BytesPerPixel);
		return BakeTextureInternal(
			TexturePath, Width, Height, PixelFormat, SourceFormat, MipGenSettings,
			[ValueFn, BytesPerPixel](float X, float Y, uint8* OutData) {
				ValueType Value = ValueFn(X, Y);
				memcpy(OutData, &Value, BytesPerPixel);
			});
	}

private:
	static UTexture2D* BakeTextureInternal(
		const FString& TexturePath, int32 Width, int32 Height, EPixelFormat PixelFormat, ETextureSourceFormat SourceFormat,
		TextureMipGenSettings MipGenSettings, TFunctionRef<void(float X, float Y, uint8* OutData)> ValueFn);
};
