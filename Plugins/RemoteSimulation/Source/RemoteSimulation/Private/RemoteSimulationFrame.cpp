// Fill out your copyright notice in the Description page of Project Settings.

#include "RemoteSimulationFrame.h"

#define LOCTEXT_NAMESPACE "RemoteSimulation"

FRemoteSimulationInvariants::FRemoteSimulationInvariants()
{
}

FRemoteSimulationInvariants::FRemoteSimulationInvariants(TArray<float>& InMasses, bool bComputeInverseMasses)
{
	Masses = MoveTemp(InMasses);

	if (bComputeInverseMasses)
	{
		ComputeInverseMasses();
	}
}

int32 FRemoteSimulationInvariants::GetNumPoints() const
{
	return Masses.Num();
}

void FRemoteSimulationInvariants::SetNumPoints(int32 NumPoints)
{
	Masses.SetNumUninitialized(NumPoints);
	InvMasses.Empty();
}

void FRemoteSimulationInvariants::Empty()
{
	Masses.Empty();
	InvMasses.Empty();
}

bool FRemoteSimulationInvariants::IsValid() const
{
	const int32 NumPoints = Masses.Num();
	return InvMasses.Num() == NumPoints;
}

void FRemoteSimulationInvariants::SetForceFactor(float InForceFactor)
{
	ForceFactor = InForceFactor;
}

void FRemoteSimulationInvariants::ComputeInverseMasses()
{
	InvMasses.SetNumUninitialized(Masses.Num());
	for (int32 i = 0; i < Masses.Num(); ++i)
	{
		const float Mass = Masses[i];
		InvMasses[i] = FMath::IsNearlyZero(Mass) ? 0.0f : 1.0f / Mass;
	}
}

FRemoteSimulationFrame::FRemoteSimulationFrame()
	: PositionBounds(EForceInit::ForceInitToZero)
	, bBoundsDirty(true)
	, PointDataBuffer(nullptr)
{
}

FRemoteSimulationFrame::FRemoteSimulationFrame(TArray<FVector>& InPositions, TArray<FVector>& InVelocities)
	: PositionBounds(EForceInit::ForceInitToZero)
	, bBoundsDirty(true)
	, PointDataBuffer(nullptr)
{
	if (InPositions.Num() != InVelocities.Num())
	{
		UE_LOG(
			LogRemoteSimulation, Error, TEXT("Input arrays must have same size (positions: %d, velocities: %d)"), InPositions.Num(),
			InVelocities.Num());
		return;
	}

	Positions = MoveTemp(InPositions);
	Velocities = MoveTemp(InVelocities);
	Forces.SetNumZeroed(Positions.Num());
}

FRemoteSimulationFrame::FRemoteSimulationFrame(const FRemoteSimulationFrame& Other)
{
	*this = Other;
}

FRemoteSimulationFrame& FRemoteSimulationFrame::operator=(const FRemoteSimulationFrame& Other)
{
	Positions = Other.Positions;
	Velocities = Other.Velocities;
	Forces = Other.Forces;

	PositionBounds = Other.PositionBounds;
	bBoundsDirty = Other.bBoundsDirty;

	return *this;
}

void FRemoteSimulationFrame::ContinueFrom(const FRemoteSimulationFrame& Other)
{
	const int32 NumPoints = Other.GetNumPoints();

	Positions = Other.Positions;
	Velocities = Other.Velocities;
	Forces.SetNumZeroed(NumPoints);

	PositionBounds = Other.PositionBounds;
	bBoundsDirty = Other.bBoundsDirty;
}

int32 FRemoteSimulationFrame::GetNumPoints() const
{
	return Positions.Num();
}

void FRemoteSimulationFrame::SetNumPoints(int32 NumPoints)
{
	Positions.SetNumUninitialized(NumPoints);
	Velocities.SetNumUninitialized(NumPoints);
	Forces.SetNumUninitialized(NumPoints);
	bBoundsDirty = true;
}

void FRemoteSimulationFrame::Empty()
{
	Positions.Empty();
	Velocities.Empty();
	Forces.Empty();
	PositionBounds = FBoxSphereBounds(EForceInit::ForceInitToZero);
	bBoundsDirty = false;
}

bool FRemoteSimulationFrame::IsValid() const
{
	const int32 NumPoints = Positions.Num();
	return Velocities.Num() == NumPoints && Forces.Num() == NumPoints;
}

void FRemoteSimulationFrame::SetPoint(int32 Index, const FVector& InPosition, const FVector& InVelocity, const FVector& InForce)
{
	check(Index < Positions.Num());
	Positions[Index] = InPosition;
	Velocities[Index] = InVelocity;
	Forces[Index] = InForce;
	bBoundsDirty = true;
}

void FRemoteSimulationFrame::SetPostion(int32 Index, const FVector& InPosition)
{
	Positions[Index] = InPosition;
	bBoundsDirty = true;
}

void FRemoteSimulationFrame::SetVelocity(int32 Index, const FVector& InVelocity)
{
	Velocities[Index] = InVelocity;
}

void FRemoteSimulationFrame::SetForce(int32 Index, const FVector& InForce)
{
	Forces[Index] = InForce;
}

void FRemoteSimulationFrame::AddForce(int32 Index, const FVector& InForce)
{
	Forces[Index] += InForce;
}

bool FRemoteSimulationFrame::UpdateBounds()
{
	if (bBoundsDirty)
	{
		PositionBounds = FBoxSphereBounds(FBox(Positions.GetData(), Positions.Num()), FSphere(Positions.GetData(), Positions.Num()));
		bBoundsDirty = false;
		return true;
	}
	return false;
}

FBoxSphereBounds FRemoteSimulationFrame::GetBounds(bool bUpdateBounds)
{
	if (bUpdateBounds)
	{
		UpdateBounds();
	}

	return PositionBounds;
}

FBoxSphereBounds FRemoteSimulationFrame::GetBounds() const
{
	return PositionBounds;
}

bool FRemoteSimulationFrame::IsBoundsDirty() const
{
	return bBoundsDirty;
}

#undef LOCTEXT_NAMESPACE
