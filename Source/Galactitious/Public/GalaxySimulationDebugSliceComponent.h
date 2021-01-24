// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FastMultipoleTextureFunctionLibrary.h"

#include "Components/StaticMeshComponent.h"

#include "GalaxySimulationDebugSliceComponent.generated.h"

UCLASS(meta = (BlueprintSpawnableComponent))
class GALACTITIOUS_API UGalaxySimulationDebugSliceComponent : public UStaticMeshComponent
{
	GENERATED_BODY()

public:
	using FAsyncTextureData = UFastMultipoleTextureFunctionLibrary::FAsyncTextureData;
	using FAsyncTextureDataPtr = UFastMultipoleTextureFunctionLibrary::FAsyncTextureDataPtr;

	UGalaxySimulationDebugSliceComponent(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	void InitSliceTexture(
		int32 Width, int32 Height, EPixelFormat PixelFormat, ETextureSourceFormat SourceFormat, TextureMipGenSettings MipGenSettings);
	void UpdateSliceTexture(const class UFastMultipoleSimulation* Simulation);

	UFUNCTION()
	void OnSimulationReset(class UFastMultipoleSimulation* Simulation);
	UFUNCTION()
	void OnSimulationStep(class UFastMultipoleSimulation* Simulation);
	void OnTransformUpdated(USceneComponent* UpdatedComponent, EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport);

private:
	UPROPERTY(Transient)
	UMaterialInstanceDynamic* SliceMaterial;

	UPROPERTY(Transient)
	UTexture2D* SliceTexture;

	FAsyncTextureDataPtr PendingTextureData;
};
