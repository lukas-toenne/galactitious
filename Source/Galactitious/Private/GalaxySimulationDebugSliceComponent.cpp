// Fill out your copyright notice in the Description page of Project Settings.

#include "GalaxySimulationDebugSliceComponent.h"

#include "FastMultipoleSimulation.h"
#include "GalaxySimulationActor.h"

#include "Async/Async.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogGalaxySimulationDebugSlice, Log, All);

UGalaxySimulationDebugSliceComponent::UGalaxySimulationDebugSliceComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
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

void UGalaxySimulationDebugSliceComponent::BeginPlay()
{
	Super::BeginPlay();

	InitSliceTexture(512, 512, PF_A8R8G8B8, TSF_BGRA8, TMGS_SimpleAverage);

	AGalaxySimulationActor* SimActor = GetOwner<AGalaxySimulationActor>();
	if (SimActor)
	{
		if (UFastMultipoleSimulation* Simulation = SimActor->GetSimulation())
		{
			UpdateSliceTexture(Simulation);
			Simulation->OnSimulationReset.AddDynamic(this, &UGalaxySimulationDebugSliceComponent::OnSimulationReset);
		}

		TransformUpdated.AddUObject(this, &UGalaxySimulationDebugSliceComponent::OnTransformUpdated);
	}
}

void UGalaxySimulationDebugSliceComponent::TickComponent(
	float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	if (!PendingTextureData.IsValid())
	{
		SetComponentTickEnabled(false);
	}
	else if (PendingTextureData->bCompleted)
	{
		SliceTexture->Source.Init(
			PendingTextureData->Width, PendingTextureData->Height, /*NumSlices=*/1, /*NumMips=*/1, PendingTextureData->SourceFormat,
			PendingTextureData->Bytes.GetData());
		SliceTexture->UpdateResource();

		PendingTextureData.Reset();

		SetComponentTickEnabled(false);
	}
}

void UGalaxySimulationDebugSliceComponent::InitSliceTexture(
	int32 Width, int32 Height, EPixelFormat PixelFormat, ETextureSourceFormat SourceFormat, TextureMipGenSettings MipGenSettings)
{
	SliceTexture = UTexture2D::CreateTransient(Width, Height, PixelFormat);
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

void UGalaxySimulationDebugSliceComponent::UpdateSliceTexture(const class UFastMultipoleSimulation* Simulation)
{
	if (!SliceTexture || !Simulation)
	{
		return;
	}

	const FBox MeshBounds = Bounds.GetBox();
	const FTransform MeshToWorld = FTransform(FQuat::Identity, MeshBounds.Min, MeshBounds.Max - MeshBounds.Min) * GetComponentTransform();

	if (FAsyncTextureData* CurrentTextureData = PendingTextureData.Get())
	{
		CurrentTextureData->bStopRequested = true;
	}

	PendingTextureData = MakeShared<FAsyncTextureData, ESPMode::ThreadSafe>();
	PendingTextureData->Width = SliceTexture->GetSizeX();
	PendingTextureData->Height = SliceTexture->GetSizeY();
	PendingTextureData->SourceFormat = TSF_BGRA8;
	SetComponentTickEnabled(true);

	UFastMultipoleTextureFunctionLibrary::BakeSliceTextureAsync(PendingTextureData, MeshToWorld, Simulation);
}

void UGalaxySimulationDebugSliceComponent::OnSimulationReset(class UFastMultipoleSimulation* Simulation)
{
	UpdateSliceTexture(Simulation);
}

void UGalaxySimulationDebugSliceComponent::OnSimulationStep(class UFastMultipoleSimulation* Simulation)
{
	UpdateSliceTexture(Simulation);
}

void UGalaxySimulationDebugSliceComponent::OnTransformUpdated(
	USceneComponent* UpdatedComponent, EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport)
{
	AGalaxySimulationActor* SimActor = GetOwner<AGalaxySimulationActor>();
	if (SimActor)
	{
		UpdateSliceTexture(SimActor->GetSimulation());
	}
}
