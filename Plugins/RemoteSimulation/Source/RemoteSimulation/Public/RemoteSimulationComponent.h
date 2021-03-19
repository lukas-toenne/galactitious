// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/MeshComponent.h"
#include "RemoteSimulationComponent.generated.h"

struct FRemoteSimulationRenderData;
class IRemoteSimulationPointGenerator;
class UBodySetup;
class URemoteSimulationCache;

/** Component for rendering remote simulation results. */
UCLASS(ClassGroup=Rendering, ShowCategories = (Rendering), HideCategories = (Object, LOD, Physics, Activation, Materials, Cooking, Input, HLOD, Mobile), meta = (BlueprintSpawnableComponent))
class REMOTESIMULATION_API URemoteSimulationComponent : public UMeshComponent
{
	GENERATED_BODY()
		
public:
	URemoteSimulationComponent();

	// Begin UObject Interface.
	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
	virtual void PostLoad() override;

#if WITH_EDITOR
	virtual void PreEditChange(FProperty* PropertyThatWillChange) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditImport() override
	{
		Super::PostEditImport();

		// Make sure to update the material after duplicating this component
		UpdateMaterial();
	}
#endif
	// End UObject Interface.

	// Begin UActorComponent Interface
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	// End UActorComponent Interface

	// Begin USceneComponent Interface
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	// End USceneComponent Interface

	// Begin UPrimitiveComponent Interface
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual UBodySetup* GetBodySetup() override;
	// End UMeshComponent Interface

	// End UMeshComponent Interface
	virtual int32 GetNumMaterials() const override { return 1; }
	virtual UMaterialInterface* GetMaterial(int32 ElementIndex) const override { return Material; }
	virtual void SetMaterial(int32 ElementIndex, UMaterialInterface* InMaterial) override;
	// End UMeshComponent Interface

	UFUNCTION(BlueprintGetter)
	URemoteSimulationCache* GetSimulationCache() const { return SimulationCache; }

	UFUNCTION(BlueprintCallable, Category = "Remote Simulation")
	void InitializeCache(int32 NumPoints, const TScriptInterface<IRemoteSimulationPointGenerator>& Generator);

private:
	void UpdateMaterial();

	void AttachCacheListener();
	void RemoveCacheListener();
	void OnCacheUpdated();

	void PostCacheSet();

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material", meta = (AllowPrivateAccess = "true"))
	UMaterialInterface* Material;

	UPROPERTY(Transient, BlueprintGetter = GetSimulationCache, Category = "Remote Simulation")
	URemoteSimulationCache* SimulationCache;

	bool bRenderDataDirty;
};
