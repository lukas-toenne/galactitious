// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "Kismet/BlueprintFunctionLibrary.h"

#include "FastMultipoleTextureFunctionLibrary.generated.h"

UCLASS()
class FASTMULTIPOLESIMULATION_API UFastMultipoleTextureFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	struct FAsyncTextureData
	{
		int32 Width = 0;
		int32 Height = 0;
		ETextureSourceFormat SourceFormat = TSF_Invalid;
		TArray<uint8> Bytes;
		TAtomic<bool> bCompleted = false;
		TAtomic<bool> bStopRequested = false;
	};

	using FAsyncTextureDataPtr = TSharedPtr<FAsyncTextureData, ESPMode::ThreadSafe>;

	static void BakeSliceTextureAsync(
		FAsyncTextureDataPtr TextureData, const FTransform& TextureToWorld, const class UFastMultipoleSimulationCache* SimulationCache);
};
