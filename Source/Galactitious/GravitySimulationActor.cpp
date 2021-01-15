// Fill out your copyright notice in the Description page of Project Settings.

#include "GravitySimulationActor.h"

#include "TextureBakerFunctionLibrary.h"

#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

#pragma warning(push)
#pragma warning(disable : 4146)
#pragma warning(disable : 4582)
#pragma warning(disable : 4583)

THIRD_PARTY_INCLUDES_START
#pragma push_macro("TEXT")
#undef TEXT
#pragma push_macro("check")
#undef check
#pragma push_macro("CONSTEXPR")
#undef CONSTEXPR
#define NOMINMAX
#include <openvdb/tools/Interpolation.h>
#include <openvdb/points/PointConversion.h>
#include <openvdb/tools/LevelSetSphere.h>
#pragma pop_macro("TEXT")
#pragma pop_macro("check")
#pragma pop_macro("CONSTEXPR")
THIRD_PARTY_INCLUDES_END

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

void AGravitySimulationActor::DistributePoints(TArray<FVector>& OutPositions, TArray<FVector>& OutVelocities) const
{
}

void AGravitySimulationActor::GatherMoments(const TArray<FVector>& InPositions, const GridType::Ptr& InGrid) const
{
	// Create a vector with four point positions.
	std::vector<openvdb::Vec3R> positions;
	positions.push_back(openvdb::Vec3R(0, 1, 0));
	positions.push_back(openvdb::Vec3R(1.5, 3.5, 1));
	positions.push_back(openvdb::Vec3R(-1, 6, -2));
	positions.push_back(openvdb::Vec3R(1.1, 1.25, 0.06));

	// The VDB Point-Partioner is used when bucketing points and requires a
	// specific interface. For convenience, we use the PointAttributeVector
	// wrapper around an stl vector wrapper here, however it is also possible to
	// write one for a custom data structure in order to match the interface
	// required.
	openvdb::points::PointAttributeVector<openvdb::Vec3R> positionsWrapper(positions);

	// This method computes a voxel-size to match the number of
	// points / voxel requested. Although it won't be exact, it typically offers
	// a good balance of memory against performance.
	int pointsPerVoxel = 8;
	float voxelSize = openvdb::points::computeVoxelSize(positionsWrapper, pointsPerVoxel);

	// Print the voxel-size to cout
	//std::cout << "VoxelSize=" << voxelSize << std::endl;

	// Create a transform using this voxel-size.
	openvdb::math::Transform::Ptr transform = openvdb::math::Transform::createLinearTransform(voxelSize);
	// Create a PointDataGrid containing these four points and using the
	// transform given. This function has two template parameters, (1) the codec
	// to use for storing the position, (2) the grid we want to create
	// (ie a PointDataGrid).
	// We use no compression here for the positions.
	openvdb::points::PointDataGrid::Ptr grid =
		openvdb::points::createPointDataGrid<openvdb::points::NullCodec, openvdb::points::PointDataGrid>(positions, *transform);
	// Set the name of the grid
	grid->setName("Points");
}

#pragma warning(pop)
