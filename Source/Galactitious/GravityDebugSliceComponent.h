// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GravitySimulationActor.h"

#include "Components/StaticMeshComponent.h"

THIRD_PARTY_INCLUDES_START
#include <openvdb/openvdb.h>
THIRD_PARTY_INCLUDES_END

#include "GravityDebugSliceComponent.generated.h"

UCLASS(meta = (BlueprintSpawnableComponent))
class GALACTITIOUS_API UGravityDebugSliceComponent : public UStaticMeshComponent
{
	GENERATED_BODY()

public:
	using GridType = AGravitySimulationActor::GridType;

	struct FTextureData
	{
		int32 Width = 0;
		int32 Height = 0;
		ETextureSourceFormat SourceFormat = TSF_Invalid;
		TArray<uint8> Bytes;
		volatile int32 bFinished = 0;
	};

	using TextureDataPtr = TSharedPtr<FTextureData, ESPMode::ThreadSafe>;
	using TextureDataPtrPtr = TWeakPtr<TextureDataPtr, ESPMode::ThreadSafe>;

	UGravityDebugSliceComponent(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	void InitSliceTexture(
		int32 Width, int32 Height, EPixelFormat PixelFormat, ETextureSourceFormat SourceFormat, TextureMipGenSettings MipGenSettings);
	void UpdateSliceTexture(GridType::Ptr Grid);

	static bool BakeSliceTexture(
		const TextureDataPtr& TextureData, const TextureDataPtrPtr& CurrentTextureData, const FTransform& MeshToWorld,
		const typename GridType::Ptr& Grid);

	void OnTransformUpdated(USceneComponent* UpdatedComponent, EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport);

private:
	UPROPERTY(Transient)
	UMaterialInstanceDynamic* SliceMaterial;

	UPROPERTY(Transient)
	UTexture2D* SliceTexture;

	TSharedPtr<FTextureData, ESPMode::ThreadSafe> PendingTextureData;
};
