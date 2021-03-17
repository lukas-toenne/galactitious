// Fill out your copyright notice in the Description page of Project Settings.

#include "RemoteSimulationComponent.h"

#include "RemoteSimulationCache.h"
#include "RemoteSimulationCommon.h"
#include "RemoteSimulationRenderBuffers.h"
#include "RemoteSimulationSceneProxy.h"
#include "RemoteSimulationTypes.h"
#include "RenderingThread.h"

#include "Engine/CollisionProfile.h"
#include "Materials/Material.h"

DECLARE_CYCLE_STAT(TEXT("Render Data Update"), STAT_UpdateRenderData, STATGROUP_RemoteSimulation)

#define IS_PROPERTY(Name) PropertyChangedEvent.MemberProperty->GetName().Equals(#Name)

/** Global index buffer shared between all proxies */
static TGlobalResource<FRemoteSimulationIndexBuffer> GRemoteSimulationIndexBuffer;

static const TArray<FRemoteSimulationPoint> Points({{FVector(0, 0, 0)}, {FVector(40, 0, -20)}, {FVector(30, 10, 20)}, {FVector(-40, -10, 0)}});
static const uint32 NumVisiblePoints = Points.Num();

URemoteSimulationComponent::URemoteSimulationComponent() : Material(nullptr), RenderBuffer(nullptr), bRenderDataDirty(true)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	Mobility = EComponentMobility::Movable;

	CastShadow = false;
	SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

	SimulationCache = CreateDefaultSubobject<URemoteSimulationCache>(TEXT("Remote Simulation Cache"), true);
	SimulationCache->SetCapacity(100, ERemoteSimulationCacheResizeMode::PruneStart);
}

void URemoteSimulationComponent::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
	URemoteSimulationComponent* This = CastChecked<URemoteSimulationComponent>(InThis);
	Super::AddReferencedObjects(This, Collector);
}

void URemoteSimulationComponent::PostLoad()
{
	Super::PostLoad();
}

#if WITH_EDITOR
void URemoteSimulationComponent::PreEditChange(FProperty* PropertyThatWillChange)
{
	Super::PreEditChange(PropertyThatWillChange);

	if (PropertyThatWillChange)
	{
		if (PropertyThatWillChange->GetName().Equals("PointCloud"))
		{
			RemovePointCloudListener();
		}
	}
}

void URemoteSimulationComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (PropertyChangedEvent.MemberProperty)
	{
		if (IS_PROPERTY(PointCloud))
		{
			PostPointCloudSet();
		}

		if (IS_PROPERTY(Material))
		{
			SetMaterial(0, Material);
		}

		// if (IS_PROPERTY(Gain))
		//{
		//	ApplyRenderingParameters();
		//}

		// if (IS_PROPERTY(PointShape))
		//{
		//	UpdateMaterial();
		//}
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif // WITH_EDITOR

void URemoteSimulationComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	// TODO
	bool bUpdateRenderData = true;

	if (bUpdateRenderData)
	{
		if (SceneProxy)
		{
			// Graphics update
			URemoteSimulationComponent* This = this;
			ENQUEUE_RENDER_COMMAND(FUpdateRemoteSimulationRenderData)
			([This](FRHICommandListImmediate& RHICmdList) {
				SCOPE_CYCLE_COUNTER(STAT_UpdateRenderData);

				int32 MaxPointsPerGroup = 0;
				if (This->BuildRenderData())
				{
					// add one-off build stuff here
				}

				FRemoteSimulationSceneProxy* Proxy = static_cast<FRemoteSimulationSceneProxy*>(This->SceneProxy);
				if (Proxy)
				{
					FRemoteSimulationRenderData RenderData;
					RenderData.IndexBuffer = &GRemoteSimulationIndexBuffer;
					RenderData.PointSize = 10.0f; // TODO

					RenderData.PointGroups.Reserve(1);
					// TODO: for each point group ...
					{
						FRemoteSimulationPointGroupRenderData& PointGroup = RenderData.PointGroups.AddDefaulted_GetRef();
						PointGroup.NumPoints = NumVisiblePoints;
						MaxPointsPerGroup = FMath::Max(MaxPointsPerGroup, PointGroup.NumPoints);
						PointGroup.RenderBuffer = This->RenderBuffer;
					}
					RenderData.MaxPointsPerGroup = MaxPointsPerGroup;

					if ((uint32)MaxPointsPerGroup > GRemoteSimulationIndexBuffer.Capacity)
					{
						GRemoteSimulationIndexBuffer.Resize(MaxPointsPerGroup);
					}

					Proxy->UpdateRenderData_RenderThread(RenderData);
				}
			});
		}
	}
}

FBoxSphereBounds URemoteSimulationComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	return /*PointCloud ? PointCloud->GetBounds().TransformBy(LocalToWorld) :*/ USceneComponent::CalcBounds(LocalToWorld);
}

FPrimitiveSceneProxy* URemoteSimulationComponent::CreateSceneProxy()
{
	FRemoteSimulationSceneProxy* Proxy = nullptr;
	// if (PointCloud)
	{
		Proxy = new FRemoteSimulationSceneProxy(this);
		// FLidarPointCloudLODManager::RegisterProxy(this, Proxy->ProxyWrapper);
	}
	return Proxy;
}

UBodySetup* URemoteSimulationComponent::GetBodySetup()
{
	return /*PointCloud ? PointCloud->GetBodySetup() :*/ nullptr;
}

void URemoteSimulationComponent::SetMaterial(int32 ElementIndex, UMaterialInterface* InMaterial)
{
	RemovePointCloudListener();
	Material = InMaterial;
	PostPointCloudSet();
	OnPointCloudRebuilt();
}

void URemoteSimulationComponent::PostPointCloudSet()
{
	AttachPointCloudListener();
}

void URemoteSimulationComponent::AttachPointCloudListener()
{
	// if (PointCloud)
	//{
	//	PointCloud->OnPointCloudRebuilt().AddUObject(this, &URemoteSimulationComponent::OnPointCloudRebuilt);
	//	PointCloud->OnPointCloudCollisionUpdated().AddUObject(this, &URemoteSimulationComponent::OnPointCloudCollisionUpdated);
	//}
}

void URemoteSimulationComponent::RemovePointCloudListener()
{
	// if (PointCloud)
	//{
	//	PointCloud->OnPointCloudRebuilt().RemoveAll(this);
	//	PointCloud->OnPointCloudCollisionUpdated().RemoveAll(this);
	//}
}

void URemoteSimulationComponent::UpdateMaterial()
{
	// Material = CustomMaterial;
	// ApplyRenderingParameters();
}

void URemoteSimulationComponent::OnPointCloudRebuilt()
{
	MarkRenderStateDirty();
	UpdateBounds();
	UpdateMaterial();
}

bool URemoteSimulationComponent::BuildRenderData()
{
	check(IsInRenderingThread());

	if (NumVisiblePoints > 0)
	{
		if (!RenderBuffer)
		{
			RenderBuffer = new FRemoteSimulationRenderBuffer();
			bRenderDataDirty = true;
		}

		if (bRenderDataDirty)
		{
			RenderBuffer->Resize(NumVisiblePoints);

			const size_t DataStride = sizeof(FRemoteSimulationPoint);
			uint8* StructuredBuffer = (uint8*)RHILockVertexBuffer(RenderBuffer->Buffer, 0, NumVisiblePoints * DataStride, RLM_WriteOnly);
			for (const FRemoteSimulationPoint *Data = Points.GetData(), *DataEnd = Data + NumVisiblePoints; Data != DataEnd; ++Data)
			{
				FMemory::Memcpy(StructuredBuffer, Data, DataStride);
				StructuredBuffer += DataStride;
			}
			RHIUnlockVertexBuffer(RenderBuffer->Buffer);

			bRenderDataDirty = false;
		}

		return true;
	}

	return false;
}

void URemoteSimulationComponent::ReleaseRenderData()
{
	if (RenderBuffer)
	{
		FRemoteSimulationRenderBuffer* Tmp = RenderBuffer;
		ENQUEUE_RENDER_COMMAND(RemoteSimulationComponent_ReleaseRenderData)
		([Tmp](FRHICommandListImmediate& RHICmdList) {
			Tmp->ReleaseResource();
			delete Tmp;
		});

		RenderBuffer = nullptr;
	}
}
