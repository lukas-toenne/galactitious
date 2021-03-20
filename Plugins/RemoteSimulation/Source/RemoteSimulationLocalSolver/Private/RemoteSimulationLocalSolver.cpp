// Fill out your copyright notice in the Description page of Project Settings.

#include "RemoteSimulationLocalSolver.h"

//class PointAttributeVectorArray
//{
//public:
//	using ValueType = openvdb::Vec3R;
//
//	PointAttributeVectorArray(const TArray<FVector>& data, const openvdb::Index stride = 1) : mData(data), mStride(stride) {}
//
//	size_t size() const { return mData.Num(); }
//	void get(ValueType& value, size_t n) const { value = OpenVDBConvert::Vector(mData[n]); }
//	void get(ValueType& value, size_t n, openvdb::Index m) const { value = OpenVDBConvert::Vector(mData[n * mStride + m]); }
//
//	// For use as position array in createPointDataGrid
//	using PosType = ValueType;
//	using value_type = ValueType;
//	void getPos(size_t n, value_type& xyz) const { xyz = OpenVDBConvert::Vector(mData[n]); }
//
//private:
//	const TArray<FVector>& mData;
//	const openvdb::Index mStride;
//}; // PointAttributeVector

void FRemoteSimulationLocalSolver::Init()
{
	//openvdb::initialize();
}

void FRemoteSimulationLocalSolver::Shutdown()
{
}

FRemoteSimulationLocalSolver::FRemoteSimulationLocalSolver(const FRemoteSimulationSolverSettings& InSettings) : Settings(InSettings)
{
}

FRemoteSimulationLocalSolver::~FRemoteSimulationLocalSolver()
{
}

void FRemoteSimulationLocalSolver::SetDebugWorld(UWorld* InDebugWorld)
{
#if UE_BUILD_DEBUG
	DebugWorld = InDebugWorld;
#endif
}

void FRemoteSimulationLocalSolver::Reset(
	FRemoteSimulationInvariants::ConstPtr InInvariants, FRemoteSimulationFrame::Ptr InStartFrame)
{
	Invariants = InInvariants;
	CurrentFrame = InStartFrame;
	NextFrame.Reset();

	// Force initialization before first iteration
	ComputeForces(InStartFrame->GetPositions(), InStartFrame->GetForces());
}

bool FRemoteSimulationLocalSolver::Step(FThreadSafeBool& bStopRequested, FRemoteSimulationStepResult& Result)
{
	Result.Frame.Reset();

	if (!CurrentFrame.IsValid())
	{
		Result.Status = ERemoteSimulationResultStatus::NotInitialized;
		return false;
	}

	if (bStopRequested)
	{
		Result.Status = ERemoteSimulationResultStatus::Stopped;
		return false;
	}

	NextFrame = MakeShared<FRemoteSimulationFrame, ESPMode::ThreadSafe>();
	NextFrame->ContinueFrom(*CurrentFrame);

	Integrate();

	Result.Frame = NextFrame;
	CurrentFrame = NextFrame;
	NextFrame.Reset();

	Result.Status = ERemoteSimulationResultStatus::Success;
	return true;
}

void FRemoteSimulationLocalSolver::Integrate()
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
	case ERemoteSimulationIntegrator::Euler:
		for (int32 i = 0; i < NumPoints; ++i)
		{
			NextPositions[i] = Positions[i] + Velocities[i] * dt;
			NextVelocities[i] = Velocities[i] + Forces[i] * dt;
		}

		ComputeForces(NextPositions, NextForces);
		break;

	case ERemoteSimulationIntegrator::Leapfrog:
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

void FRemoteSimulationLocalSolver::ComputeForces(const TArray<FVector>& InPositions, TArray<FVector>& OutForces)
{
	switch (Settings.ForceMethod)
	{
	case ERemoteSimulationForceMethod::Direct:
		ComputeForces_Direct(InPositions, OutForces);
		break;
	case ERemoteSimulationForceMethod::FastMultipole:
		ComputeForces_FastMultipole(InPositions, OutForces);
		break;
	}
}

void FRemoteSimulationLocalSolver::ComputeForces_Direct(const TArray<FVector>& InPositions, TArray<FVector>& OutForces)
{
	const int32 NumPoints = InPositions.Num();
	check(OutForces.Num() == NumPoints);

	const float ForceFactor = Invariants->GetForceFactor();
	const float SofteningOffset = Settings.GravitySofteningRadius * Settings.GravitySofteningRadius;
	check(SofteningOffset >= SMALL_NUMBER);
	const TArray<float>& Masses = Invariants->GetMasses();
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

void FRemoteSimulationLocalSolver::ComputeForces_FastMultipole(const TArray<FVector>& InPositions, TArray<FVector>& OutForces)
{
}
