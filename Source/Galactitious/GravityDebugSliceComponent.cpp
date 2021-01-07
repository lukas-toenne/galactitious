// Fill out your copyright notice in the Description page of Project Settings.

#include "GravityDebugSliceComponent.h"

#include "OpenVDBConvert.h"
#include "TextureBakerFunctionLibrary.h"

#include "Async/Async.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

THIRD_PARTY_INCLUDES_START
#include <openvdb/tools/Interpolation.h>
#include <tbb/task.h>
THIRD_PARTY_INCLUDES_END

DEFINE_LOG_CATEGORY_STATIC(LogGravityDebugSlice, Log, All);

UGravityDebugSliceComponent::UGravityDebugSliceComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshFinder(TEXT("/Engine/BasicShapes/Plane.Plane"));
	check(MeshFinder.Object);
	SetStaticMesh(MeshFinder.Object);

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(TEXT("/Game/Materials/SliceMaterial.SliceMaterial"));
	check(MaterialFinder.Object);
	SetMaterial(0, MaterialFinder.Object);
}

void UGravityDebugSliceComponent::BeginPlay()
{
	Super::BeginPlay();

	InitSliceTexture(512, 512, PF_A8R8G8B8, TSF_BGRA8, TMGS_SimpleAverage);

	AGravitySimulationActor* SimActor = GetOwner<AGravitySimulationActor>();
	if (ensure(SimActor))
	{
		UpdateSliceTexture(SimActor->GetGrid());
		TransformUpdated.AddUObject(this, &UGravityDebugSliceComponent::OnTransformUpdated);
	}
}

void UGravityDebugSliceComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	if (!PendingTextureData.IsValid())
	{
		SetComponentTickEnabled(false);
	}
	else if (FPlatformAtomics::AtomicRead(&PendingTextureData->bFinished) == 1)
	{
		SliceTexture->Source.Init(
			PendingTextureData->Width, PendingTextureData->Height, /*NumSlices=*/1, /*NumMips=*/1, PendingTextureData->SourceFormat,
			PendingTextureData->Bytes.GetData());
		SliceTexture->UpdateResource();

		PendingTextureData.Reset();

		SetComponentTickEnabled(false);
	}
}

namespace
{
	template <class SamplerType>
	bool BakeSliceTextureData(
		const UGravityDebugSliceComponent::TextureDataPtr& TextureDataPtr,
		const UGravityDebugSliceComponent::TextureDataPtrPtr& CurrentTextureData,
		const FTransform& MeshToWorld, SamplerType Sampler)
	{
		UGravityDebugSliceComponent::FTextureData* TextureData = TextureDataPtr.Get();

		const int32 BytesPerPixel = FTextureSource::GetBytesPerPixel(TextureData->SourceFormat);
		check(BytesPerPixel == sizeof(FColor));

		const int32 Width = TextureData->Width;
		const int32 Height = TextureData->Height;
		uint8* Data = TextureData->Bytes.GetData();
		const float dX = 1.0f / FMath::Max(Width - 1, 1);
		const float dY = 1.0f / FMath::Max(Height - 1, 1);

		float Y = 0.0f;
		for (int32 j = 0; j < Height; j++)
		{
			float X = 0.0f;
			for (int32 i = 0; i < Width; i++)
			{
				if (!CurrentTextureData.HasSameObject(&TextureDataPtr))
				{
					return false;
				}

				FVector LocalPosition = FVector(X, Y, 0.5f);
				openvdb::Vec3f vdbPosition = OpenVDBConvert::Vector(MeshToWorld.TransformPosition(LocalPosition));
				float Mass = Sampler.wsSample(vdbPosition);
				*(FColor*)Data = FColor::MakeRedToGreenColorFromScalar(FMath::Clamp(Mass, 0.0f, 1.0f));

				X += dX;
				Data += BytesPerPixel;
			}
			Y += dY;
		}

		return true;
	}
} // namespace

void UGravityDebugSliceComponent::InitSliceTexture(
	int32 Width, int32 Height, EPixelFormat PixelFormat, ETextureSourceFormat SourceFormat, TextureMipGenSettings MipGenSettings)
{
	SliceTexture = UTextureBakerFunctionLibrary::CreateTransientTexture(Width, Height, PixelFormat);
	if (SliceTexture)
	{
		const int32 BytesPerPixel = FTextureSource::GetBytesPerPixel(SourceFormat);
		TArray<uint8> TextureBytes;
		TextureBytes.SetNumZeroed(Width * Height * BytesPerPixel);
		SliceTexture->Source.Init(Width, Height, 1, 1, SourceFormat, TextureBytes.GetData());

		SliceTexture->MipGenSettings = MipGenSettings;
		SliceTexture->CompressionNoAlpha = true;
		SliceTexture->CompressionSettings = TextureCompressionSettings::TC_Default;
		SliceTexture->Filter = TF_Bilinear;
		SliceTexture->AddressX = TA_Clamp;
		SliceTexture->AddressY = TA_Clamp;
		SliceTexture->LODGroup = TEXTUREGROUP_Effects;
		SliceTexture->SRGB = false;
		SliceTexture->bFlipGreenChannel = false;

		SliceTexture->UpdateResource();

		SliceMaterial = CreateDynamicMaterialInstance(0);
		SliceMaterial->SetTextureParameterValue(TEXT("Texture"), SliceTexture);
	}
}

void UGravityDebugSliceComponent::UpdateSliceTexture(GridType::Ptr Grid)
{
	if (!SliceTexture || !Grid)
	{
		return;
	}

	const FBox MeshBounds = Bounds.GetBox();
	const FTransform MeshToWorld = FTransform(FQuat::Identity, MeshBounds.Min, MeshBounds.Max - MeshBounds.Min) * GetComponentTransform();

	PendingTextureData = MakeShared<FTextureData, ESPMode::ThreadSafe>();
	PendingTextureData->Width = SliceTexture->GetSizeX();
	PendingTextureData->Height = SliceTexture->GetSizeY();
	PendingTextureData->SourceFormat = TSF_BGRA8;
	SetComponentTickEnabled(true);

	TextureDataPtr TextureData = PendingTextureData;
	TextureDataPtrPtr CurrentTextureData = MakeShared<TextureDataPtr, ESPMode::ThreadSafe>(PendingTextureData);
	Async(EAsyncExecution::ThreadPool, [MeshToWorld, Grid, TextureData, CurrentTextureData]() {
		UE_LOG(LogGravityDebugSlice, Display, TEXT("TASK: Texture data user count = %d"), TextureData.GetSharedReferenceCount());
		if (BakeSliceTexture(TextureData, CurrentTextureData, MeshToWorld, Grid))
		{
			FPlatformAtomics::AtomicStore(&TextureData->bFinished, 1);
		}
		UE_LOG(LogGravityDebugSlice, Display, TEXT("TASK DONE! Texture data size = %d"), TextureData->Bytes.Num());
	});
}

bool UGravityDebugSliceComponent::BakeSliceTexture(
	const TextureDataPtr& TextureData, const TextureDataPtrPtr& CurrentTextureData, const FTransform& MeshToWorld, const GridType::Ptr& Grid)
{
	if (!CurrentTextureData.HasSameObject(&TextureData))
	{
		return;
	}

	using SamplerType = openvdb::tools::GridSampler<GridType, openvdb::tools::PointSampler>;
	const int32 BytesPerPixel = FTextureSource::GetBytesPerPixel(TextureData->SourceFormat);

	TextureData->Bytes.AddUninitialized(TextureData->Width * TextureData->Height * BytesPerPixel);

	return BakeSliceTextureData(TextureData.Get(), CurrentTextureData, MeshToWorld, SamplerType(*Grid));
}

void UGravityDebugSliceComponent::OnTransformUpdated(
	USceneComponent* UpdatedComponent, EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport)
{
	AGravitySimulationActor* SimActor = GetOwner<AGravitySimulationActor>();
	if (SimActor)
	{
		UpdateSliceTexture(SimActor->GetGrid());
	}
}
