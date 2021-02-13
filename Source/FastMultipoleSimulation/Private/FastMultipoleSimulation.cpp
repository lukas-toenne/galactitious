// Fill out your copyright notice in the Description page of Project Settings.

#include "FastMultipoleSimulation.h"

#include "FastMultipoleOpenVDBGuardEnter.h"
#include "FastMultipoleOpenVDBGuardLeave.h"
#include "OpenVDBConvert.h"

#include <openvdb/points/PointConversion.h>

#define LOCTEXT_NAMESPACE "FastMultipole"
DEFINE_LOG_CATEGORY(LogFastMultipole)

class PointAttributeVectorArray
{
public:
	using ValueType = openvdb::Vec3R;

	PointAttributeVectorArray(const TArray<FVector>& data, const openvdb::Index stride = 1) : mData(data), mStride(stride) {}

	size_t size() const { return mData.Num(); }
	void get(ValueType& value, size_t n) const { value = OpenVDBConvert::Vector(mData[n]); }
	void get(ValueType& value, size_t n, openvdb::Index m) const { value = OpenVDBConvert::Vector(mData[n * mStride + m]); }

	// For use as position array in createPointDataGrid
	using PosType = ValueType;
	using value_type = ValueType;
	void getPos(size_t n, value_type& xyz) const { xyz = OpenVDBConvert::Vector(mData[n]); }

private:
	const TArray<FVector>& mData;
	const openvdb::Index mStride;
}; // PointAttributeVector

void FFastMultipoleSimulation::Init()
{
	openvdb::initialize();
}

void FFastMultipoleSimulation::Shutdown()
{
}

FFastMultipoleSimulation::FFastMultipoleSimulation(float InStepSize, EFastMultipoleSimulationIntegrator InIntegrator)
	: StepSize(InStepSize)
	, Integrator(InIntegrator)
{
}

FFastMultipoleSimulation::~FFastMultipoleSimulation()
{
}

void FFastMultipoleSimulation::Reset(FFastMultipoleSimulationFrame::ConstPtr InFrame)
{
	CurrentFrame = InFrame;
	NextFrame.Reset();
}

bool FFastMultipoleSimulation::Step(FThreadSafeBool& bStopRequested, FFastMultipoleSimulationStepResult& Result)
{
	Result.Frame.Reset();

	if (!CurrentFrame.IsValid())
	{
		Result.Status = EFastMultipoleSimulationStatus::NotInitialized;
		return false;
	}

	if (bStopRequested)
	{
		Result.Status = EFastMultipoleSimulationStatus::Stopped;
		return false;
	}

	NextFrame = MakeShared<FFastMultipoleSimulationFrame, ESPMode::ThreadSafe>(*CurrentFrame);

	Integrate();

	Result.Frame = NextFrame;
	CurrentFrame = NextFrame;
	NextFrame.Reset();

	Result.Status = EFastMultipoleSimulationStatus::Success;
	return true;
}

void FFastMultipoleSimulation::Integrate()
{
	check(CurrentFrame);
	check(NextFrame);

	const float dt = StepSize;
	const float dt2 = 0.5f * StepSize;
	const float dtt2 = 0.5f * StepSize * StepSize;

	NextFrame->SetDeltaTime(dt);

	const TArray<FVector>& Positions = CurrentFrame->GetPositions();
	const TArray<FVector>& Velocities = CurrentFrame->GetVelocities();
	const TArray<FVector>& Forces = CurrentFrame->GetForces();
	TArray<FVector>& NextPositions = NextFrame->GetPositions();
	TArray<FVector>& NextVelocities = NextFrame->GetVelocities();
	TArray<FVector>& NextForces = NextFrame->GetForces();

	const int32 NumPoints = CurrentFrame->GetNumPoints();
	check(NextFrame->GetNumPoints() == NumPoints);

	switch (Integrator)
	{
	case EFastMultipoleSimulationIntegrator::Euler:
		for (int32 i = 0; i < NumPoints; ++i)
		{
			ComputeForces(Positions, NextForces);
			NextPositions[i] = Positions[i] + Velocities[i] * dt;
			NextVelocities[i] = Velocities[i] + NextForces[i] * dt;
		}
		break;

	case EFastMultipoleSimulationIntegrator::Leapfrog:
		for (int32 i = 0; i < NumPoints; ++i)
		{
			NextPositions[i] = Positions[i] + Velocities[i] * dt + Forces[i] * dtt2;
			ComputeForces(Positions, NextForces);
			NextVelocities[i] = Velocities[i] + (Forces[i] + NextForces[i]) * dt2;
		}
		break;
	}
}

void FFastMultipoleSimulation::ComputeForces(const TArray<FVector>& InPositions, TArray<FVector>& OutForces)
{
	check(CurrentFrame);
	check(NextFrame);
}

void FFastMultipoleSimulation::ComputeForcesDirect(const TArray<FVector>& InPositions, TArray<FVector>& OutForces)
{
}

void FFastMultipoleSimulation::BuildPointGrid(const TArray<FVector>& Positions, PointDataGridType::Ptr& PointDataGrid)
{
	// openvdb::points::PointAttributeVector;

	PointAttributeVectorArray PositionsWrapper(Positions);

	int PointsPerVoxel = 4;
	float VoxelSize = openvdb::points::computeVoxelSize(PositionsWrapper, PointsPerVoxel);

	openvdb::math::Transform::Ptr GridTransform = openvdb::math::Transform::createLinearTransform(VoxelSize);

	PointIndexGridType::Ptr PointIndexGrid = openvdb::tools::createPointIndexGrid<PointIndexGridType>(PositionsWrapper, *GridTransform);
	PointDataGrid = openvdb::points::createPointDataGrid<openvdb::points::NullCodec, PointDataGridType>(
		*PointIndexGrid, PositionsWrapper, *GridTransform);
	PointDataGrid->setName("Points");
}

void FFastMultipoleSimulation::ClearPointGrid()
{
}

#undef LOCTEXT_NAMESPACE
