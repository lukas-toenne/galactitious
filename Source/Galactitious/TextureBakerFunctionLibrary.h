// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "Engine/Texture.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "TextureBakerFunctionLibrary.generated.h"

struct FVector4_16
{
	FFloat16 X;
	FFloat16 Y;
	FFloat16 Z;
	FFloat16 W;
};

UCLASS()
class GALACTITIOUS_API UTextureBakerFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	template <typename ValueType>
	static UTexture2D* BakeTransientTexture(
		const FString& TexturePath, int32 Width, int32 Height, EPixelFormat PixelFormat, ETextureSourceFormat SourceFormat,
		TextureMipGenSettings MipGenSettings, TFunctionRef<ValueType(float X, float Y)> ValueFn)
	{
		const int32 BytesPerPixel = FTextureSource::GetBytesPerPixel(SourceFormat);
		check(sizeof(ValueType) == BytesPerPixel);
		if (UTexture2D* Texture = CreateTransientTextureInternal(Width, Height, PixelFormat))
		{
			BakeTextureInternal(
				Texture, Width, Height, PixelFormat, SourceFormat, MipGenSettings,
				[ValueFn, BytesPerPixel](float X, float Y, uint8* OutData) {
					ValueType Value = ValueFn(X, Y);
					memcpy(OutData, &Value, BytesPerPixel);
				});
			return Texture;
		}
		return nullptr;
	}

#if WITH_EDITOR
	template <typename ValueType>
	static UTexture2D* BakeTextureAsset(
		const FString& TexturePath, int32 Width, int32 Height, EPixelFormat PixelFormat, ETextureSourceFormat SourceFormat,
		TextureMipGenSettings MipGenSettings, TFunctionRef<ValueType(float X, float Y)> ValueFn)
	{
		const int32 BytesPerPixel = FTextureSource::GetBytesPerPixel(SourceFormat);
		check(sizeof(ValueType) == BytesPerPixel);
		if (UTexture2D* Texture = CreateTextureAssetInternal(TexturePath, Width, Height, PixelFormat, SourceFormat))
		{
			BakeTextureInternal(
				Texture, Width, Height, PixelFormat, SourceFormat, MipGenSettings,
				[ValueFn, BytesPerPixel](float X, float Y, uint8* OutData) {
					ValueType Value = ValueFn(X, Y);
					memcpy(OutData, &Value, BytesPerPixel);
				});
			return Texture;
		}
		return nullptr;
	}
#endif

	TFunction<float(float X, float Y)> FloatCurveEvalFunction(const struct FInterpCurveFloat& Curve);
	TFunction<FLinearColor(float X, float Y)> LinearColorCurveEvalFunction(const struct FInterpCurveLinearColor& Curve);

private:
	static void BakeTextureInternal(
		UTexture2D* Texture, int32 Width, int32 Height, EPixelFormat PixelFormat, ETextureSourceFormat SourceFormat,
		TextureMipGenSettings MipGenSettings, TFunctionRef<void(float X, float Y, uint8* OutData)> ValueFn);

	static UTexture2D* CreateTransientTextureInternal(int32 Width, int32 Height, EPixelFormat PixelFormat);

#if WITH_EDITOR
	static UTexture2D* CreateTextureAssetInternal(
		const FString& TexturePath, int32 Width, int32 Height, EPixelFormat PixelFormat, ETextureSourceFormat SourceFormat);
#endif
};
