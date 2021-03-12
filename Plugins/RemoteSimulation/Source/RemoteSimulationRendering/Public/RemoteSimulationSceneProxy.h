// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "PrimitiveSceneProxy.h"

//#include "RemoteSimulationVertexFactory.h"

class FRemoteSimulationIndexBuffer;
class FRemoteSimulationRenderBuffer;
class URemoteSimulationComponent;

/** Used to pass data to RT to update the proxy's render data */
struct REMOTESIMULATIONRENDERING_API FRemoteSimulationRenderData
{
	FRemoteSimulationRenderData();

	int32 NumPoints;
	FRemoteSimulationIndexBuffer* IndexBuffer;
	FRemoteSimulationRenderBuffer* RenderBuffer;

	float PointSize;
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

	void UpdateRenderData_RenderThread(const FRemoteSimulationRenderData& InRenderData);

private:
	struct FRemoteSimulationBatchElementUserData BuildUserDataElement(const FSceneView* InView) const;

private:
	FRemoteSimulationRenderData RenderData;

	UMaterialInterface* Material;
	FMaterialRelevance MaterialRelevance;

	bool bEditorView;
};
