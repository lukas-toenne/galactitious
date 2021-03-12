// Fill out your copyright notice in the Description page of Project Settings.

#include "RemoteSimulationSceneProxy.h"

#include "RemoteSimulationCommon.h"
#include "RemoteSimulationComponent.h"
#include "RemoteSimulationRenderBuffers.h"
#include "RemoteSimulationVertexFactory.h"

DECLARE_DWORD_COUNTER_STAT(TEXT("Draw Calls"), STAT_DrawCallCount, STATGROUP_RemoteSimulation)

static FRemoteSimulationBatchElementUserData BuildUserDataElement(const FSceneView* InView, const FRemoteSimulationRenderData& RenderData);

FRemoteSimulationRenderData::FRemoteSimulationRenderData() : NumPoints(0), RenderBuffer(nullptr)
{
}

class FRemoteSimulationOneFrameResource : public FOneFrameResource
{
public:
	FRemoteSimulationBatchElementUserData Payload;
	virtual ~FRemoteSimulationOneFrameResource() {}
};

FRemoteSimulationSceneProxy::FRemoteSimulationSceneProxy(URemoteSimulationComponent* Component) : FPrimitiveSceneProxy(Component)
{
	// Skip material verification - async update could occasionally cause it to crash
	bVerifyUsedMaterials = false;

	Material = Component->GetMaterial(0);
	MaterialRelevance = Component->GetMaterialRelevance(GetScene().GetFeatureLevel());

	// TODO
	bEditorView = false;
}

FRemoteSimulationSceneProxy::~FRemoteSimulationSceneProxy()
{
}

void FRemoteSimulationSceneProxy::GetDynamicMeshElements(
	const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap,
	FMeshElementCollector& Collector) const
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_PointCloudSceneProxy_GetDynamicMeshElements);

	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
	{
		const FSceneView* View = Views[ViewIndex];

		if (IsShown(View) && (VisibilityMap & (1 << ViewIndex)))
		{
			if (RenderData.NumPoints)
			{
				check(RenderData.RenderBuffer);

				FRemoteSimulationBatchElementUserData& UserData =
					Collector.AllocateOneFrameResource<FRemoteSimulationOneFrameResource>().Payload;

				FMeshBatch& MeshBatch = Collector.AllocateMesh();

				MeshBatch.Type = PT_TriangleList;
				MeshBatch.LODIndex = 0;
				MeshBatch.VertexFactory = &GRemoteSimulationVertexFactory;
				MeshBatch.bWireframe = false;
				MeshBatch.MaterialRenderProxy = Material->GetRenderProxy();
				MeshBatch.ReverseCulling = IsLocalToWorldDeterminantNegative();
				MeshBatch.DepthPriorityGroup = SDPG_World;

				FMeshBatchElement& BatchElement = MeshBatch.Elements[0];
				BatchElement.PrimitiveUniformBuffer = GetUniformBuffer();
				BatchElement.IndexBuffer = RenderData.IndexBuffer;
				BatchElement.FirstIndex = 0;
				BatchElement.MinVertexIndex = 0;
				BatchElement.NumPrimitives = RenderData.NumPoints * 2;
				UserData = BuildUserDataElement(View);
				BatchElement.UserData = &UserData;

				Collector.AddMesh(ViewIndex, MeshBatch);

				INC_DWORD_STAT(STAT_DrawCallCount);
			}

#if !(UE_BUILD_SHIPPING)
			FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);

			// Draw selected nodes' bounds
			// if (Component->bDrawNodeBounds)
			//{
			//	for (const FBox& Node : RenderData.Bounds)
			//	{
			//		DrawWireBox(PDI, Node, FColor(72, 72, 255), SDPG_World);
			//	}
			//}

			// Render bounds
			if (ViewFamily.EngineShowFlags.Bounds)
			{
				RenderBounds(PDI, ViewFamily.EngineShowFlags, GetBounds(), IsSelected());
			}
#endif // !(UE_BUILD_SHIPPING)
		}
	}
}

FPrimitiveViewRelevance FRemoteSimulationSceneProxy::GetViewRelevance(const FSceneView* View) const
{
	FPrimitiveViewRelevance Result;

	Result.bDrawRelevance = IsShown(View);
	Result.bShadowRelevance = IsShadowCast(View);
	Result.bDynamicRelevance = true;
	Result.bStaticRelevance = false;
	Result.bRenderInMainPass = ShouldRenderInMainPass();
	Result.bUsesLightingChannels = GetLightingChannelMask() != GetDefaultLightingChannelMask();
	Result.bRenderCustomDepth = ShouldRenderCustomDepth();
	MaterialRelevance.SetPrimitiveViewRelevance(Result);

	return Result;
}

/** UserData is used to pass rendering information to the VertexFactory */
FRemoteSimulationBatchElementUserData FRemoteSimulationSceneProxy::BuildUserDataElement(const FSceneView* InView) const
{
	FRemoteSimulationBatchElementUserData UserDataElement;

	// const bool bUsesSprites = Component->PointSize > 0;

	// FVector BoundsSize = Component->GetPointCloud()->GetBounds().GetSize();

	//// Make sure to apply minimum bounds size
	// BoundsSize.X = FMath::Max(BoundsSize.X, 0.001f);
	// BoundsSize.Y = FMath::Max(BoundsSize.Y, 0.001f);
	// BoundsSize.Z = FMath::Max(BoundsSize.Z, 0.001f);

	//// Update shader parameters
	UserDataElement.bEditorView = bEditorView;
	UserDataElement.SpriteSize = RenderData.PointSize;

	UserDataElement.ViewRightVector = InView->GetViewRight();
	UserDataElement.ViewUpVector = InView->GetViewUp();
	UserDataElement.bUseCameraFacing = true;

	// UserDataElement.IndexDivisor = bUsesSprites ? 4 : 1;
	// UserDataElement.LocationOffset = Component->GetPointCloud()->GetLocationOffset().ToVector();
	// UserDataElement.BoundsSize = BoundsSize;
	// UserDataElement.ElevationColorBottom = FVector(Component->ColorSource == ELidarPointCloudColorationMode::None ? FColor::White :
	// Component->ElevationColorBottom); UserDataElement.ElevationColorTop = FVector(Component->ColorSource ==
	// ELidarPointCloudColorationMode::None ? FColor::White : Component->ElevationColorTop); UserDataElement.bUseCircle = bUsesSprites &&
	// Component->PointShape == ELidarPointCloudSpriteShape::Circle; UserDataElement.bUseColorOverride = Component->ColorSource !=
	// ELidarPointCloudColorationMode::Data; UserDataElement.bUseElevationColor = Component->ColorSource ==
	// ELidarPointCloudColorationMode::Elevation || Component->ColorSource == ELidarPointCloudColorationMode::None; UserDataElement.Offset =
	// Component->Offset; UserDataElement.Contrast = Component->Contrast; UserDataElement.Saturation = Component->Saturation;
	// UserDataElement.Gamma = Component->Gamma;
	// UserDataElement.Tint = FVector(Component->ColorTint);
	// UserDataElement.IntensityInfluence = Component->IntensityInfluence;

	// UserDataElement.bUseClassification = Component->ColorSource == ELidarPointCloudColorationMode::Classification;
	// UserDataElement.SetClassificationColors(Component->ClassificationColors);
	//
	// UserDataElement.NumClippingVolumes = FMath::Min(RenderData.ClippingVolumes.Num(), 16);

	// for (uint32 i = 0; i < UserDataElement.NumClippingVolumes; ++i)
	//{
	//	const ALidarClippingVolume* ClippingVolume = RenderData.ClippingVolumes[i];
	//	const FVector Extent = ClippingVolume->GetActorScale3D() * 100;
	//	UserDataElement.ClippingVolume[i] = FMatrix(FPlane(ClippingVolume->GetActorLocation(), ClippingVolume->Mode ==
	// ELidarClippingVolumeMode::ClipInside), 												FPlane(ClippingVolume->GetActorForwardVector(),
	// Extent.X), 											    FPlane(ClippingVolume->GetActorRightVector(), Extent.Y),
	// FPlane(ClippingVolume->GetActorUpVector(), Extent.Z));

	//	UserDataElement.bStartClipped = UserDataElement.bStartClipped || ClippingVolume->Mode == ELidarClippingVolumeMode::ClipOutside;
	//}

	UserDataElement.DataBuffer = RenderData.RenderBuffer->SRV;

	return UserDataElement;
}

bool FRemoteSimulationSceneProxy::CanBeOccluded() const
{
	return !MaterialRelevance.bDisableDepthTest;
}

uint32 FRemoteSimulationSceneProxy::GetMemoryFootprint() const
{
	return (sizeof(*this) + GetAllocatedSize());
}

uint32 FRemoteSimulationSceneProxy::GetAllocatedSize() const
{
	return (FPrimitiveSceneProxy::GetAllocatedSize());
}

SIZE_T FRemoteSimulationSceneProxy::GetTypeHash() const
{
	static size_t UniquePointer;
	return reinterpret_cast<size_t>(&UniquePointer);
}

void FRemoteSimulationSceneProxy::UpdateRenderData_RenderThread(const FRemoteSimulationRenderData& InRenderData)
{
	check(IsInRenderingThread());
	RenderData = InRenderData;
}
