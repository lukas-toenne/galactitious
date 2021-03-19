// Fill out your copyright notice in the Description page of Project Settings.

#include "RemoteSimulationFrame.h"

#include "RemoteSimulationRenderBuffers.h"

#define LOCTEXT_NAMESPACE "RemoteSimulation"

/** Global index buffer for point sprites shared between all proxies */
static TGlobalResource<FRemoteSimulationIndexBuffer> GRemoteSimulationPointIndexBuffer;

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

void FRemoteSimulationInvariants::ComputeInverseMasses()
{
	InvMasses.SetNumUninitialized(Masses.Num());
	for (int32 i = 0; i < Masses.Num(); ++i)
	{
		const float Mass = Masses[i];
		InvMasses[i] = FMath::IsNearlyZero(Mass) ? 0.0f : 1.0f / Mass;
	}
}

FRemoteSimulationFrame::FRemoteSimulationFrame() : PointDataBuffer(nullptr)
{
}

FRemoteSimulationFrame::FRemoteSimulationFrame(TArray<FVector>& InPositions, TArray<FVector>& InVelocities) : PointDataBuffer(nullptr)
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
	PointDataBuffer = nullptr;
	return *this;
}

void FRemoteSimulationFrame::ContinueFrom(const FRemoteSimulationFrame& Other)
{
	const int32 NumPoints = Other.GetNumPoints();

	Positions = Other.Positions;
	Velocities = Other.Velocities;
	Forces.SetNumZeroed(NumPoints);
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
}

void FRemoteSimulationFrame::Empty()
{
	Positions.Empty();
	Velocities.Empty();
	Forces.Empty();
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
}

void FRemoteSimulationFrame::SetPostion(int32 Index, const FVector& InPosition)
{
	Positions[Index] = InPosition;
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

bool FRemoteSimulationFrame::HasRenderData() const
{
	return PointDataBuffer != nullptr;
}

FRemoteSimulationIndexBuffer* FRemoteSimulationFrame::GetPointIndexBuffer()
{
	return &GRemoteSimulationPointIndexBuffer;
}

bool FRemoteSimulationFrame::BuildRenderData() const
{
	check(IsInRenderingThread());

	// Build point data buffer
	const int32 NumPoints = Positions.Num();
	if (NumPoints > 0)
	{
		if (!PointDataBuffer)
		{
			PointDataBuffer = new FRemoteSimulationPointDataBuffer();
			// bRenderDataDirty = true;
		}

		int32 MaxPointsPerGroup = 0;
		// if (bRenderDataDirty)
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

			// bRenderDataDirty = false;
		}

		if ((uint32)MaxPointsPerGroup > GRemoteSimulationPointIndexBuffer.Capacity)
		{
			GRemoteSimulationPointIndexBuffer.Resize(MaxPointsPerGroup);
		}

		return true;
	}

	return false;
}

void FRemoteSimulationFrame::ReleaseRenderData() const
{
	if (PointDataBuffer)
	{
		ENQUEUE_RENDER_COMMAND(RemoteSimulationComponent_ReleaseRenderData)
		([Buffer = PointDataBuffer](FRHICommandListImmediate& RHICmdList) {
			Buffer->ReleaseResource();
			delete Buffer;
		});

		PointDataBuffer = nullptr;
	}
}

#undef LOCTEXT_NAMESPACE
