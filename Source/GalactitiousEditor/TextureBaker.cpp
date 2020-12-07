// Fill out your copyright notice in the Description page of Project Settings.


#include "TextureBaker.h"

#include "ObjectTools.h"
#include "PackageTools.h"
#include "AssetRegistry/AssetRegistryModule.h"

UTexture2D* FTextureBaker::BakeTextureInternal(
	const FString& TexturePath, int32 Width, int32 Height, EPixelFormat PixelFormat, ETextureSourceFormat SourceFormat,
	TextureMipGenSettings MipGenSettings,
	TFunctionRef<void(float X, float Y, uint8* OutData)> ValueFn)
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
		Texture->PostEditChange();
		Texture->MarkPackageDirty();
		FAssetRegistryModule::AssetCreated(Texture);
	}

	return Texture;
}
