// Fill out your copyright notice in the Description page of Project Settings.

#include "GravitySimulationActor.h"

#include "TextureBakerFunctionLibrary.h"

#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

THIRD_PARTY_INCLUDES_START
#pragma warning(push)
#pragma warning(disable : 4146)
#pragma push_macro("TEXT")
#undef TEXT
#pragma push_macro("check")
#undef check
#include <openvdb/tools/Interpolation.h>
#include <openvdb/points/PointConversion.h>
#include <openvdb/tools/LevelSetSphere.h>
#pragma pop_macro("check")
#pragma pop_macro("TEXT")
#pragma warning(pop)
THIRD_PARTY_INCLUDES_END

//const FMassMoments FMassMoments::ZeroMoments = FMassMoments(0.0f);

struct OpenVDBConvert
{
	static inline FVector Vector(const openvdb::Vec3f& v) { return FVector(v.x(), v.y(), v.z()); }
	static inline openvdb::Vec3f Vector(const FVector& v) { return openvdb::Vec3f(v.X, v.Y, v.Z); }

	static inline FQuat Quat(const openvdb::QuatR& q) { return FQuat(q.x(), q.y(), q.z(), q.w()); }
	static inline openvdb::QuatR Quat(const FQuat& q) { return openvdb::QuatR(q.X, q.Y, q.Z, q.W); }

	static inline FMatrix Matrix4(const openvdb::math::Mat4f& m)
	{
		FMatrix R;
		memcpy(R.M, m.asPointer(), sizeof(float) * 16);
		return R;
	}
	static inline openvdb::math::Mat4f Matrix4(const FMatrix& m)
	{
		openvdb::math::Mat4f R;
		memcpy(R.asPointer(), m.M, sizeof(float) * 16);
		return R;
	}
};

AGravitySimulationActor::AGravitySimulationActor()
{
}

void AGravitySimulationActor::BeginPlay()
{
	Super::BeginPlay();

	openvdb::initialize();

	// Create a FloatGrid and populate it with a narrow-band
	// signed distance field of a sphere.
	Grid = openvdb::tools::createLevelSetSphere<GridType>(
		/*radius=*/50.0, /*center=*/openvdb::Vec3f(1.5, 2, 3),
		/*voxel size=*/0.5, /*width=*/4.0);
	// Associate some metadata with the grid.
	Grid->insertMeta("radius", openvdb::FloatMetadata(50.0));
	// Name the grid "LevelSetSphere".
	Grid->setName("LevelSetSphere");
}
