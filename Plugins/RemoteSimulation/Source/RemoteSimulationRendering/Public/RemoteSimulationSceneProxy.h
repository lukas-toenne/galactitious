// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "PrimitiveSceneProxy.h"

//#include "RemoteSimulationVertexFactory.h"

class URemoteSimulationComponent;

/** Used to pass data to RT to update the proxy's render data */
struct FRemoteSimulationProxyUpdateData
{
//	TWeakPtr<FLidarPointCloudSceneProxyWrapper, ESPMode::ThreadSafe> SceneProxyWrapper;
//
//	/** Number of elements within the structured buffer related to this proxy */
//	int32 NumElements;
//
//	TArray<FRemoteSimulationProxyUpdateDataNode> SelectedNodes;
//
//	float VDMultiplier;
//	float RootCellSize;
//
//#if !(UE_BUILD_SHIPPING)
//	/** Stores bounds of selected nodes, used for debugging */
//	TArray<FBox> Bounds;
//#endif
//
//	TArray<const class ALidarClippingVolume*> ClippingVolumes;

	FRemoteSimulationProxyUpdateData();
};

class REMOTESIMULATIONRENDERING_API FRemoteSimulationSceneProxy : public FPrimitiveSceneProxy
{
public:
	FRemoteSimulationSceneProxy(URemoteSimulationComponent* Component);
	virtual ~FRemoteSimulationSceneProxy();

	virtual void GetDynamicMeshElements(
		const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap,
		FMeshElementCollector& Collector) const override;

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override;

	virtual bool CanBeOccluded() const override;
	virtual uint32 GetMemoryFootprint() const override;
	uint32 GetAllocatedSize() const;

	virtual SIZE_T GetTypeHash() const override;

	void UpdateRenderData(FRemoteSimulationProxyUpdateData InRenderData);

public:
	//TSharedPtr<FRemoteSimulationSceneProxyWrapper, ESPMode::ThreadSafe> ProxyWrapper;

private:
	FRemoteSimulationProxyUpdateData RenderData;

	URemoteSimulationComponent* Component;
	FMaterialRelevance MaterialRelevance;
	AActor* Owner;
};

//FPrimitiveSceneProxy* URemoteSimulationComponent::CreateSceneProxy()
//{
//	FRemoteSimulationSceneProxy* Proxy = nullptr;
//	if (PointCloud)
//	{
//		Proxy = new FRemoteSimulationSceneProxy(this);
//		FLidarPointCloudLODManager::RegisterProxy(this, Proxy->ProxyWrapper);
//	}
//	return Proxy;
//}
