// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "RemoteSimulationCommon.h"

#include "RemoteSimulationFrame.generated.h"

USTRUCT(BlueprintType)
struct REMOTESIMULATION_API FRemoteSimulationInvariants
{
	GENERATED_BODY()

public:
	using Ptr = TSharedPtr<FRemoteSimulationInvariants, ESPMode::ThreadSafe>;
	using ConstPtr = TSharedPtr<FRemoteSimulationInvariants const, ESPMode::ThreadSafe>;

	FRemoteSimulationInvariants();
	FRemoteSimulationInvariants(TArray<float>& Masses, bool bComputeInverseMasses = true);
	FRemoteSimulationInvariants(const FRemoteSimulationInvariants& Other) = default;

	FRemoteSimulationInvariants& operator=(const FRemoteSimulationInvariants& Other) = default;

	int32 GetNumPoints() const;
	void SetNumPoints(int32 NumPoints);
	void Empty();

	bool IsValid() const;

	const TArray<float>& GetMasses() const { return Masses; }
	const TArray<float>& GetInverseMasses() const { return InvMasses; }

	TArray<float>& GetMasses() { return Masses; }
	TArray<float>& GetInverseMasses() { return InvMasses; }

	void ComputeInverseMasses();

private:
	// Proportionality factor A of forces to inverse-square distance:
	// F = A * m1 * m2 / r^2
	float ForceFactor;

	// Mass of particles
	TArray<float> Masses;
	TArray<float> InvMasses;
};

USTRUCT(BlueprintType)
struct REMOTESIMULATION_API FRemoteSimulationFrame
{
	GENERATED_BODY()

public:
	using Ptr = TSharedPtr<FRemoteSimulationFrame, ESPMode::ThreadSafe>;
	using ConstPtr = TSharedPtr<FRemoteSimulationFrame const, ESPMode::ThreadSafe>;

	FRemoteSimulationFrame();
	FRemoteSimulationFrame(TArray<FVector>& Positions, TArray<FVector>& Velocities);
	FRemoteSimulationFrame(const FRemoteSimulationFrame& Other) = default;

	FRemoteSimulationFrame& operator=(const FRemoteSimulationFrame& Other) = default;

	void ContinueFrom(const FRemoteSimulationFrame& Other);

	int32 GetNumPoints() const;
	void SetNumPoints(int32 NumPoints);
	void Empty();

	bool IsValid() const;

	const TArray<FVector>& GetPositions() const { return Positions; }
	const TArray<FVector>& GetVelocities() const { return Velocities; }
	const TArray<FVector>& GetForces() const { return Forces; }

	TArray<FVector>& GetPositions() { return Positions; }
	TArray<FVector>& GetVelocities() { return Velocities; }
	TArray<FVector>& GetForces() { return Forces; }

	void SetPoint(int32 Index, const FVector& InPosition, const FVector& InVelocity, const FVector& InForce = FVector::ZeroVector);
	void SetPostion(int32 Index, const FVector& InPosition);
	void SetVelocity(int32 Index, const FVector& InVelocity);
	void SetForce(int32 Index, const FVector& InForce);
	void AddForce(int32 Index, const FVector& InForce);

private:
	TArray<FVector> Positions;
	TArray<FVector> Velocities;
	TArray<FVector> Forces;
};
