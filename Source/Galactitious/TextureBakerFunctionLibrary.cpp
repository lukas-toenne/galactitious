// Fill out your copyright notice in the Description page of Project Settings.


#include "TextureBakerFunctionLibrary.h"

#include "ObjectTools.h"
#include "PackageTools.h"
#include "AssetRegistry/AssetRegistryModule.h"

void UTextureBakerFunctionLibrary::BakeTextureInternal(
	UTexture2D* Texture, int32 Width, int32 Height, EPixelFormat PixelFormat, ETextureSourceFormat SourceFormat,
	TextureMipGenSettings MipGenSettings, TFunctionRef<void(float X, float Y, uint8* OutData)> ValueFn)
{
	const int32 BytesPerPixel = FTextureSource::GetBytesPerPixel(SourceFormat);

	check(Width >= 1 && Height >= 1);

	if (!Texture)
	{
		return;
	}

	{
		TArray<uint8> TextureData;
		TextureData.AddUninitialized(Width * Height * BytesPerPixel);

		uint8* Data = TextureData.GetData();
		const float dX = 1.0f / FMath::Max(Width - 1, 1);
		const float dY = 1.0f / FMath::Max(Height - 1, 1);

		float Y = 0.0f;
		for (int32 j = 0; j < Height; j++)
		{
			float X = 0.0f;
			for (int32 i = 0; i < Width; i++)
			{
				ValueFn(X, Y, Data);
				X += dX;
				Data += BytesPerPixel;
			}
			Y += dY;
		}

		Texture->Source.Init(Width, Height, /*NumSlices=*/1, 1, SourceFormat, TextureData.GetData());
	}

	Texture->MipGenSettings = MipGenSettings;
	Texture->CompressionNoAlpha = true;
	Texture->CompressionSettings = TextureCompressionSettings::TC_Default;
	Texture->Filter = TF_Bilinear;
	Texture->AddressX = TA_Clamp;
	Texture->AddressY = TA_Clamp;
	Texture->LODGroup = TEXTUREGROUP_Effects;
	Texture->SRGB = false;
	Texture->bFlipGreenChannel = false;

	// Updating Texture & mark it as unsaved
	Texture->UpdateResource();
#if WITH_EDITOR
	Texture->PostEditChange();
#endif
	Texture->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(Texture);
}

UTexture2D* UTextureBakerFunctionLibrary::CreateTransientTextureInternal(int32 Width, int32 Height, EPixelFormat PixelFormat)
{
	check(Width >= 1 && Height >= 1);
	return UTexture2D::CreateTransient(Width, Height, PixelFormat);
}

#if WITH_EDITOR
UTexture2D* UTextureBakerFunctionLibrary::CreateTextureAssetInternal(
	const FString& TexturePath, int32 Width, int32 Height, EPixelFormat PixelFormat, ETextureSourceFormat SourceFormat)
{
	const int32 BytesPerPixel = FTextureSource::GetBytesPerPixel(SourceFormat);

	check(Width >= 1 && Height >= 1);

	const FString TextureName = ObjectTools::SanitizeObjectName(FPaths::GetBaseFilename(TexturePath));
	if (TextureName.IsEmpty())
	{
		return nullptr;
	}

	const FString PackageName = UPackageTools::SanitizePackageName(TexturePath);
	UPackage* AssetPackage = CreatePackage(*PackageName);

	UTexture2D* Texture = NewObject<UTexture2D>(AssetPackage, FName(TextureName), RF_Public | RF_Standalone);
	Texture->AddToRoot();
	if (Texture)
	{
		// Texture Settings
		Texture->PlatformData = new FTexturePlatformData();
		Texture->PlatformData->SizeX = Width;
		Texture->PlatformData->SizeY = Height;
		Texture->PlatformData->PixelFormat = PixelFormat;
	}

	return Texture;
}
#endif

TFunction<float(float X, float Y)> UTextureBakerFunctionLibrary::FloatCurveEvalFunction(const FInterpCurveFloat& Curve)
{
	if (!ensure(Curve.Points.Num() > 0))
	{
		return [](float X, float Y) -> float { return 0.0f; };
	}

	const float MinTime = Curve.Points[0].InVal;
	const float MaxTime = Curve.Points[Curve.Points.Num() - 1].InVal;
	const float DeltaTime = MaxTime - MinTime;

	return [Curve, DeltaTime](float X, float Y) -> float { return Curve.Eval(X * DeltaTime); };
}

TFunction<FLinearColor(float X, float Y)> UTextureBakerFunctionLibrary::LinearColorCurveEvalFunction(const FInterpCurveLinearColor& Curve)
{
	if (!ensure(Curve.Points.Num() > 0))
	{
		return [](float X, float Y) -> FLinearColor { return FLinearColor::Black; };
	}

	const float MinTime = Curve.Points[0].InVal;
	const float MaxTime = Curve.Points[Curve.Points.Num() - 1].InVal;
	const float DeltaTime = MaxTime - MinTime;

	return [Curve, DeltaTime](float X, float Y) -> FLinearColor { return Curve.Eval(X * DeltaTime); };
}
