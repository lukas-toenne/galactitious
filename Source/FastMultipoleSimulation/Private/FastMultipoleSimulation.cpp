// Fill out your copyright notice in the Description page of Project Settings.

#include "FastMultipoleSimulation.h"

#include "OpenVDBConvert.h"

#include "FastMultipoleOpenVDBGuardEnter.h"
#include <openvdb/points/PointConversion.h>
#include "FastMultipoleOpenVDBGuardLeave.h"

#define LOCTEXT_NAMESPACE "FastMultipole"
DEFINE_LOG_CATEGORY(LogFastMultipole)

class PointAttributeVectorArray
{
public:
	using ValueType = openvdb::Vec3R;

	PointAttributeVectorArray(const TArray<FVector>& data, const openvdb::Index stride = 1) : mData(data), mStride(stride) {}

	size_t size() const { return mData.Num(); }
	void get(ValueType& value, size_t n) const {value = OpenVDBConvert::Vector(mData[n]); }
	void get(ValueType& value, size_t n, openvdb::Index m) const {value = OpenVDBConvert::Vector(mData[n * mStride + m]); }

	// For use as position array in createPointDataGrid
	using PosType = ValueType;
	using value_type = ValueType;
	void getPos(size_t n, value_type& xyz) const { xyz = OpenVDBConvert::Vector(mData[n]); }

private:
	const TArray<FVector>& mData;
	const openvdb::Index mStride;
}; // PointAttributeVector

UFastMultipoleSimulation::UFastMultipoleSimulation()
{
	openvdb::initialize();
}

void UFastMultipoleSimulation::Reset(const TSharedPtr<PointBuffer>& InPositions)
{
	Positions = InPositions;

	BuildPointGrid();

	OnSimulationReset.Broadcast(this);
}

void UFastMultipoleSimulation::Clear()
{
	Positions.Reset();

	OnSimulationReset.Broadcast(this);
}

const TArray<FVector>& UFastMultipoleSimulation::GetPositionData() const
{
	static const TArray<FVector> DefaultPositions;
	return Positions ? *Positions : DefaultPositions;
}

void UFastMultipoleSimulation::BuildPointGrid()
{
	//openvdb::points::PointAttributeVector;

	PointAttributeVectorArray PositionsWrapper(*Positions);

	int PointsPerVoxel = 4;
	float VoxelSize = openvdb::points::computeVoxelSize(PositionsWrapper, PointsPerVoxel);

	openvdb::math::Transform::Ptr GridTransform = openvdb::math::Transform::createLinearTransform(VoxelSize);

	PointIndexGridType::Ptr PointIndexGrid = openvdb::tools::createPointIndexGrid<PointIndexGridType>(PositionsWrapper, *GridTransform);
	PointDataGrid =
		openvdb::points::createPointDataGrid<openvdb::points::NullCodec, PointDataGridType>(*PointIndexGrid, PositionsWrapper, *GridTransform);
	PointDataGrid->setName("Points");
}

void UFastMultipoleSimulation::ClearPointGrid()
{

}

#undef LOCTEXT_NAMESPACE
