// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"

#include "TelescopeData.generated.h"

UCLASS(BlueprintType)
class GALACTITIOUS_API UTelescopeData : public UDataAsset
{
public:
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	float ApertureObscuration = 0.15f;

	UPROPERTY(EditAnywhere)
	float AiryDiskScale = 8.0f;

	UPROPERTY(EditAnywhere)
	UTexture2D* AiryDiskTexture;

#if WITH_EDITOR
	UFUNCTION(BlueprintCallable, CallInEditor)
	void BakeTextures();
#endif
};
