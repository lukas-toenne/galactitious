// Fill out your copyright notice in the Description page of Project Settings.

#include "RemoteSimulationComponent.h"

#include "RemoteSimulationSceneProxy.h"

#include "Engine/CollisionProfile.h"
#include "Materials/Material.h"

#define IS_PROPERTY(Name) PropertyChangedEvent.MemberProperty->GetName().Equals(#Name)

URemoteSimulationComponent::URemoteSimulationComponent() : Material(nullptr)
{
	PrimaryComponentTick.bCanEverTick = false;
	Mobility = EComponentMobility::Movable;

	CastShadow = false;
	SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
}

FPrimitiveSceneProxy* URemoteSimulationComponent::CreateSceneProxy()
{
	FRemoteSimulationSceneProxy* Proxy = nullptr;
	//if (PointCloud)
	{
		Proxy = new FRemoteSimulationSceneProxy(this);
		//FLidarPointCloudLODManager::RegisterProxy(this, Proxy->ProxyWrapper);
	}
	return Proxy;
}

FBoxSphereBounds URemoteSimulationComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	return /*PointCloud ? PointCloud->GetBounds().TransformBy(LocalToWorld) :*/ USceneComponent::CalcBounds(LocalToWorld);
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
	//if (PointCloud)
	//{
	//	PointCloud->OnPointCloudRebuilt().AddUObject(this, &URemoteSimulationComponent::OnPointCloudRebuilt);
	//	PointCloud->OnPointCloudCollisionUpdated().AddUObject(this, &URemoteSimulationComponent::OnPointCloudCollisionUpdated);
	//}
}

void URemoteSimulationComponent::RemovePointCloudListener()
{
	//if (PointCloud)
	//{
	//	PointCloud->OnPointCloudRebuilt().RemoveAll(this);
	//	PointCloud->OnPointCloudCollisionUpdated().RemoveAll(this);
	//}
}

void URemoteSimulationComponent::UpdateMaterial()
{
	//Material = CustomMaterial;
	// ApplyRenderingParameters();
}

void URemoteSimulationComponent::OnPointCloudRebuilt()
{
	MarkRenderStateDirty();
	UpdateBounds();
	UpdateMaterial();
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
#endif
