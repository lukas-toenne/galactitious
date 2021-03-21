// Fill out your copyright notice in the Description page of Project Settings.

#include "RemoteSimulationComponent.h"

#include "IRemoteSimulationPointGenerator.h"
#include "RemoteSimulationCache.h"
#include "RemoteSimulationCommon.h"
#include "RemoteSimulationFrame.h"
#include "RemoteSimulationRenderBuffers.h"
#include "RemoteSimulationSceneProxy.h"
#include "RemoteSimulationTypes.h"
#include "RenderingThread.h"

#include "Engine/CollisionProfile.h"
#include "Materials/Material.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogRemoteSimulationComponent, Log, All);

DECLARE_CYCLE_STAT(TEXT("Render Data Update"), STAT_UpdateRenderData, STATGROUP_RemoteSimulation)

#define IS_PROPERTY(Name) PropertyChangedEvent.MemberProperty->GetName().Equals(#Name)

/** Global index buffer for point sprites shared between all proxies */
static TGlobalResource<FRemoteSimulationIndexBuffer> GRemoteSimulationPointIndexBuffer;

URemoteSimulationComponent::URemoteSimulationComponent() : Material(nullptr)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	Mobility = EComponentMobility::Movable;

	CastShadow = false;
	SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

	static ConstructorHelpers::FObjectFinder<UMaterial> MaterialFinder(TEXT("/RemoteSimulation/Materials/DefaultPoints"));
	Material = MaterialFinder.Object;

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
		if (PropertyThatWillChange->GetName().Equals("SimulationCache"))
		{
			RemoveCacheListener();
		}
	}
}

void URemoteSimulationComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (PropertyChangedEvent.MemberProperty)
	{
		if (IS_PROPERTY(SimulationCache))
		{
			PostCacheSet();
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
			FRemoteSimulationFrame::ConstPtr Frame = SimulationCache ? SimulationCache->GetLastFrame() : nullptr;

			// Graphics update
			URemoteSimulationComponent* This = this;
			ENQUEUE_RENDER_COMMAND(FUpdateRemoteSimulationRenderData)
			([This, Frame](FRHICommandListImmediate& RHICmdList) {
				SCOPE_CYCLE_COUNTER(STAT_UpdateRenderData);

				FRemoteSimulationSceneProxy* Proxy = static_cast<FRemoteSimulationSceneProxy*>(This->SceneProxy);
				if (Proxy)
				{
					FRemoteSimulationRenderData RenderData;
					RenderData.PointSize = 10.0f; // TODO

					if (Frame && This->BuildFrameRenderData(*Frame))
					{
						RenderData.IndexBuffer = &GRemoteSimulationPointIndexBuffer;

						RenderData.PointGroups.Reserve(1);
						// TODO: for each point group ...
						{
							FRemoteSimulationPointGroupRenderData& PointGroup = RenderData.PointGroups.AddDefaulted_GetRef();
							PointGroup.NumPoints = Frame->GetNumPoints();
							PointGroup.PointDataBuffer = Frame->GetPointDataBuffer();
						}

						Proxy->UpdateRenderData_RenderThread(RenderData);
					}
				}
			});
		}
	}
}

FBoxSphereBounds URemoteSimulationComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	if (SimulationCache)
	{
		FRemoteSimulationFrame::ConstPtr Frame = SimulationCache->GetLastFrame();
		if (Frame)
		{
			if (Frame->IsBoundsDirty())
			{
				UE_LOG(LogRemoteSimulationComponent, Warning, TEXT("Cache frame bounds are dirty"));
			}
			return Frame->GetBounds().TransformBy(LocalToWorld);
		}
	}

	return USceneComponent::CalcBounds(LocalToWorld);
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
	RemoveCacheListener();
	Material = InMaterial;
	PostCacheSet();
	OnCacheUpdated();
}

void URemoteSimulationComponent::InitializeCache(int32 NumPoints, const TScriptInterface<IRemoteSimulationPointGenerator>& Generator)
{
	SimulationCache->Reset();

	TArray<float> Masses;
	TArray<FVector> Positions, Velocities;
	Masses.SetNumUninitialized(NumPoints);
	Positions.SetNumUninitialized(NumPoints);
	Velocities.SetNumUninitialized(NumPoints);

	FRemoteSimulationPointResult PointResult;
	for (int32 i = 0; i < NumPoints; ++i)
	{
		IRemoteSimulationPointGenerator::Execute_GeneratePoint(Generator.GetObject(), i, PointResult);
		Masses[i] = PointResult.Mass;
		Positions[i] = PointResult.Position;
		Velocities[i] = PointResult.Velocity;
	}

	FRemoteSimulationInvariants::Ptr Invariants = MakeShared<FRemoteSimulationInvariants, ESPMode::ThreadSafe>(Masses, true);
	SimulationCache->SetInvariants(Invariants);

	FRemoteSimulationFrame::Ptr CacheFrame = MakeShared<FRemoteSimulationFrame, ESPMode::ThreadSafe>(Positions, Velocities);
	CacheFrame->UpdateBounds();
	SimulationCache->AddFrame(CacheFrame);

	OnCacheUpdated();
}

void URemoteSimulationComponent::PostCacheSet()
{
	AttachCacheListener();
}

void URemoteSimulationComponent::AttachCacheListener()
{
	// if (PointCloud)
	//{
	//	PointCloud->OnPointCloudRebuilt().AddUObject(this, &URemoteSimulationComponent::OnPointCloudRebuilt);
	//	PointCloud->OnPointCloudCollisionUpdated().AddUObject(this, &URemoteSimulationComponent::OnPointCloudCollisionUpdated);
	//}
}

void URemoteSimulationComponent::RemoveCacheListener()
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

void URemoteSimulationComponent::OnCacheUpdated()
{
	MarkRenderStateDirty();
	UpdateBounds();
	UpdateMaterial();
}

bool URemoteSimulationComponent::BuildFrameRenderData(const FRemoteSimulationFrame& Frame)
{
	check(IsInRenderingThread());

	// Build point data buffer
	const TArray<FVector>& Positions = Frame.GetPositions();
	const int32 NumPoints = Positions.Num();
	if (NumPoints > 0)
	{
		FRemoteSimulationPointDataBuffer* PointDataBuffer = Frame.GetPointDataBuffer();
		if (!PointDataBuffer)
		{
			PointDataBuffer = new FRemoteSimulationPointDataBuffer();
			Frame.SetPointDataBuffer(PointDataBuffer);
		}

		int32 MaxPointsPerGroup = 0;
		if (Frame.IsRenderDataDirty())
		{
			PointDataBuffer->Resize(NumPoints);

			const size_t DataStride = sizeof(FRemoteSimulationPointData);
			uint8* StructuredBuffer = (uint8*)RHILockVertexBuffer(PointDataBuffer->Buffer, 0, NumPoints * DataStride, RLM_WriteOnly);
			const FVector* Pos = Positions.GetData();

			for (int32 i = 0; i < NumPoints; ++i)
			{
				FRemoteSimulationPointData* PointData = (FRemoteSimulationPointData*)StructuredBuffer;
				PointData->Location = *Pos;

				StructuredBuffer += DataStride;
				++Pos;
			}
			RHIUnlockVertexBuffer(PointDataBuffer->Buffer);

			MaxPointsPerGroup = FMath::Max(MaxPointsPerGroup, NumPoints);

			Frame.SetRenderDataDirty(false);
		}

		if ((uint32)MaxPointsPerGroup > GRemoteSimulationPointIndexBuffer.Capacity)
		{
			GRemoteSimulationPointIndexBuffer.Resize(MaxPointsPerGroup);
		}

		return true;
	}

	return false;
}

void URemoteSimulationComponent::ReleaseFrameRenderData(const FRemoteSimulationFrame& Frame)
{
	FRemoteSimulationPointDataBuffer* PointDataBuffer = Frame.GetPointDataBuffer();
	if (PointDataBuffer)
	{
		ENQUEUE_RENDER_COMMAND(RemoteSimulationComponent_ReleaseRenderData)
		([PointDataBuffer](FRHICommandListImmediate& RHICmdList) {
			PointDataBuffer->ReleaseResource();
			delete PointDataBuffer;
		});

		Frame.SetPointDataBuffer(nullptr);
	}
}