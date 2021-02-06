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

FFastMultipoleSimulation::FFastMultipoleSimulation(UFastMultipoleSimulationCache* InSimulationCache, float InDeltaTime)
	: DeltaTime(InDeltaTime)
	, StepIndex(-1)
	, SimulationCache(InSimulationCache)
{
	check(SimulationCache);
}

FFastMultipoleSimulation::~FFastMultipoleSimulation()
{
}

void FFastMultipoleSimulation::Reset(TArray<FVector>& InInitialPositions, TArray<FVector>& InInitialVelocities)
{
	CurrentFrame = MakeShared<FFastMultipoleSimulationFrame, ESPMode::ThreadSafe>(InInitialPositions, InInitialVelocities);

	SimulationCache->Reset();
	SimulationCache->AddFrame(CurrentFrame);
	StepIndex = 0;
	NextFrame.Reset();
}

void FFastMultipoleSimulation::ResetToCache()
{
	if (SimulationCache->GetNumFrames() > 0)
	{
		CurrentFrame = SimulationCache->GetLastFrame();
		StepIndex = SimulationCache->GetNumFrames() - 1;
	}
	else
	{
		UE_LOG(LogFastMultipole, Warning, TEXT("Simulation cache empty, initialization failed"));
		CurrentFrame.Reset();
		StepIndex = -1;
	}

	NextFrame.Reset();
}

bool FFastMultipoleSimulation::Step(FThreadSafeBool& bStopRequested, FFastMultipoleSimulationStepResult& Result)
{
	Result.StepIndex = StepIndex;

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

	NextFrame = MakeShared<FFastMultipoleSimulationFrame, ESPMode::ThreadSafe>();

	ComputeForces();
	IntegratePositions();

	SimulationCache->AddFrame(NextFrame);

	CurrentFrame = NextFrame;
	NextFrame.Reset();
	++StepIndex;

	Result.Status = EFastMultipoleSimulationStatus::Success;
	return true;
}

void FFastMultipoleSimulation::ComputeForces()
{
	check(CurrentFrame);
	check(NextFrame);
}

void FFastMultipoleSimulation::IntegratePositions()
{
	check(CurrentFrame);
	check(NextFrame);

	NextFrame->DeltaTime = DeltaTime;
}

void FFastMultipoleSimulation::ComputeForcesDirect()
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
