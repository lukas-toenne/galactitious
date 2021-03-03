// Fill out your copyright notice in the Description page of Project Settings.

#include "FastMultipoleSimulation.h"

#include "FastMultipoleOpenVDBGuardEnter.h"
#include "OpenVDBConvert.h"
#include "FastMultipoleOpenVDBGuardLeave.h"

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

FFastMultipoleSimulation::FFastMultipoleSimulation(const FFastMultipoleSimulationSettings& InSettings) : Settings(InSettings)
{
}

FFastMultipoleSimulation::~FFastMultipoleSimulation()
{
}

void FFastMultipoleSimulation::SetDebugWorld(UWorld* InDebugWorld)
{
#if UE_BUILD_DEBUG
	DebugWorld = InDebugWorld;
#endif
}

void FFastMultipoleSimulation::Reset(
	FFastMultipoleSimulationInvariants::ConstPtr InInvariants, FFastMultipoleSimulationFrame::Ptr InStartFrame)
{
	Invariants = InInvariants;
	CurrentFrame = InStartFrame;
	NextFrame.Reset();

	// Force initialization before first iteration
	ComputeForces(InStartFrame->GetPositions(), InStartFrame->GetForces());
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

	NextFrame = MakeShared<FFastMultipoleSimulationFrame, ESPMode::ThreadSafe>();
	NextFrame->ContinueFrom(*CurrentFrame);

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

	const float dt = Settings.StepSize;
	const float dt2 = 0.5f * Settings.StepSize;
	const float dtt2 = 0.5f * Settings.StepSize * Settings.StepSize;

	const int32 NumPoints = CurrentFrame->GetNumPoints();
	check(NextFrame->GetNumPoints() == NumPoints);

	const TArray<FVector>& Positions = CurrentFrame->GetPositions();
	const TArray<FVector>& Velocities = CurrentFrame->GetVelocities();
	const TArray<FVector>& Forces = CurrentFrame->GetForces();
	TArray<FVector>& NextPositions = NextFrame->GetPositions();
	TArray<FVector>& NextVelocities = NextFrame->GetVelocities();
	TArray<FVector>& NextForces = NextFrame->GetForces();

	switch (Settings.Integrator)
	{
	case EFastMultipoleSimulationIntegrator::Euler:
		for (int32 i = 0; i < NumPoints; ++i)
		{
			NextPositions[i] = Positions[i] + Velocities[i] * dt;
			NextVelocities[i] = Velocities[i] + Forces[i] * dt;
		}

		ComputeForces(NextPositions, NextForces);
		break;

	case EFastMultipoleSimulationIntegrator::Leapfrog:
		for (int32 i = 0; i < NumPoints; ++i)
		{
			NextPositions[i] = Positions[i] + Velocities[i] * dt + Forces[i] * dtt2;
		}

		ComputeForces(NextPositions, NextForces);

		for (int32 i = 0; i < NumPoints; ++i)
		{
			NextVelocities[i] = Velocities[i] + (Forces[i] + NextForces[i]) * dt2;
		}
		break;
	}
}

void FFastMultipoleSimulation::ComputeForces(const TArray<FVector>& InPositions, TArray<FVector>& OutForces)
{
	switch (Settings.ForceMethod)
	{
	case EFastMultipoleSimulationForceMethod::Direct:
		ComputeForces_Direct(InPositions, OutForces);
		break;
	case EFastMultipoleSimulationForceMethod::FastMultipole:
		ComputeForces_FastMultipole(InPositions, OutForces);
		break;
	}
}

void FFastMultipoleSimulation::ComputeForces_Direct(const TArray<FVector>& InPositions, TArray<FVector>& OutForces)
{
	const int32 NumPoints = InPositions.Num();
	check(OutForces.Num() == NumPoints);

	const float ForceFactor = Invariants->ForceFactor;
	const float SofteningOffset = Settings.GravitySofteningRadius * Settings.GravitySofteningRadius;
	check(SofteningOffset >= SMALL_NUMBER);
	const TArray<float>& Masses = Invariants->Masses;
	check(Masses.Num() == NumPoints);

	for (int32 i = 0; i < NumPoints; ++i)
	{
		const float Mass1 = Masses[i];
		const FVector Pos1 = InPositions[i];

		for (int32 j = i + 1; j < NumPoints; ++j)
		{
			const float Mass2 = Masses[j];
			const FVector Pos2 = InPositions[j];
			const FVector R = Pos2 - Pos1;
			const float DD = R.SizeSquared() + SofteningOffset;
			const float D = FMath::Sqrt(DD);
			const FVector Force = ForceFactor * R / (DD * D);
			OutForces[i] += Force * Mass2;
			OutForces[j] -= Force * Mass1;
		}
	}
}

void FFastMultipoleSimulation::ComputeForces_FastMultipole(const TArray<FVector>& InPositions, TArray<FVector>& OutForces)
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

void FFastMultipoleSimulationUtils::ComputeStableForceFactor(
	const FFastMultipoleSimulationSettings& Settings, FFastMultipoleSimulationInvariants::ConstPtr Invariants,
	FFastMultipoleSimulationFrame::ConstPtr StartFrame, float& OutKineticEnergyAverage, float& OutStableForceFactor)
{
	check(Invariants.IsValid());
	check(StartFrame.IsValid());

	const TArray<float>& Masses = Invariants->Masses;
	const TArray<FVector>& Positions = StartFrame->GetPositions();
	const TArray<FVector>& Velocities = StartFrame->GetVelocities();

	int32 NumPoints = Masses.Num();
	check(Positions.Num() == NumPoints);
	check(Velocities.Num() == NumPoints);
	if (NumPoints == 0)
	{
		OutKineticEnergyAverage = 0.0f;
		OutStableForceFactor = 1.0f;
		return;
	}

	const float SofteningOffset = Settings.GravitySofteningRadius * Settings.GravitySofteningRadius;
	check(SofteningOffset >= SMALL_NUMBER);

	float PotentialEnergy = 0.0f;
	float KineticEnergy = 0.0f;
	for (int32 i = 0; i < NumPoints; ++i)
	{
		const float Mass1 = Masses[i];
		const FVector Pos1 = Positions[i];

		for (int32 j = i + 1; j < NumPoints; ++j)
		{
			const float Mass2 = Masses[j];
			const FVector Pos2 = Positions[j];
			const FVector R = Pos2 - Pos1;
			const float DD = R.SizeSquared() + SofteningOffset;
			const float D = FMath::Sqrt(DD);
			const float U = -Mass1 * Mass2 / D;
			// Potential energy for particle pair
			PotentialEnergy += U;
		}

		const FVector Vel = Velocities[i];
		KineticEnergy += 0.5f * Mass1 * Vel.SizeSquared();
	}

	ensure(PotentialEnergy <= 0.0f);
	ensure(KineticEnergy >= 0.0f);
	OutKineticEnergyAverage = KineticEnergy / NumPoints;
	if (FMath::IsNearlyZero(PotentialEnergy))
	{
		UE_LOG(LogFastMultipole, Warning, TEXT("Not enough potential energy to compute stable force factor, setting force factor to 1"));
		OutStableForceFactor = 1.0f;
	}
	else if (FMath::IsNearlyZero(KineticEnergy))
	{
		UE_LOG(LogFastMultipole, Warning, TEXT("Not enough kinetic energy to compute stable force factor, setting force factor to 1"));
		OutStableForceFactor = 1.0f;
	}
	else
	{
		OutStableForceFactor = -2.0f * KineticEnergy / PotentialEnergy;
	}
}

#undef LOCTEXT_NAMESPACE
