// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "GameFramework/Actor.h"

THIRD_PARTY_INCLUDES_START
#include <openvdb/openvdb.h>
THIRD_PARTY_INCLUDES_END

#include "GravitySimulationActor.generated.h"

struct FMassMoments
{
	FMassMoments() {}

	explicit FMassMoments(float InMass) { Mass = InMass; }

	float GetMass() const { return Mass; }
	float& GetMassRef() { return Mass; }

	const FMassMoments& SetZero()
	{
		Mass = 0.0f;
		return *this;
	}

	/// Assignment operator
	const FMassMoments& operator=(const FMassMoments& Other)
	{
		Mass = Other.Mass;
		return *this;
	}

	/// Equality operator, does exact floating point comparisons
	bool operator==(const FMassMoments& Other) const { return Mass == Other.Mass; }

	/// Inequality operator, does exact floating point comparisons
	bool operator!=(const FMassMoments& v) const { return !(*this == v); }

	/// Test if "this" vector is equivalent to vector v with tolerance of eps
	bool IsNearlyEqual(const FMassMoments& Other, float ErrorTolerance = SMALL_NUMBER) const
	{
		return FMath::IsNearlyEqual(Mass, Other.Mass, ErrorTolerance);
	}

	/// Negation operator, for e.g.   v1 = -v2;
	FMassMoments operator-() const { return FMassMoments(-Mass); }

	/// this = v1 + v2
	/// "this", v1 and v2 need not be distinct objects, e.g. v.add(v1,v);
	const FMassMoments& Add(const FMassMoments& M1, const FMassMoments& M2)
	{
		Mass = M1.Mass + M2.Mass;
		return *this;
	}

	/// this = v1 - v2
	/// "this", v1 and v2 need not be distinct objects, e.g. v.sub(v1,v);
	const FMassMoments& Subtract(const FMassMoments& M1, const FMassMoments& M2)
	{
		Mass = M1.Mass - M2.Mass;
		return *this;
	}

	/// this =  scalar*v, v need not be a distinct object from "this",
	/// e.g. v.scale(1.5,v1);
	template <typename S>
	const FMassMoments& Scale(S Scalar, const FMassMoments& M)
	{
		Mass = Scalar * M.Mass;
		return *this;
	}

	template <typename S>
	const FMassMoments& Divide(S Scalar, const FMassMoments& M)
	{
		Mass = M.Mass / Scalar;
		return *this;
	}

	/// Returns v, where \f$v_i *= scalar\f$ for \f$i \in [0, 1]\f$
	template <typename S>
	const FMassMoments& operator*=(S Scalar)
	{
		Mass *= Scalar;
		return *this;
	}

	/// Returns v, where \f$v_i /= scalar\f$ for \f$i \in [0, 1]\f$
	template <typename S>
	const FMassMoments& operator/=(S Scalar)
	{
		Mass /= Scalar;
		return *this;
	}

	/// True if a Nan is present in vector
	bool IsNaN() const { return FMath::IsNaN(Mass); }

	/// True if an Inf is present in vector
	bool IsFinite() const { return FMath::IsFinite(Mass); }

	static const FMassMoments ZeroMoments;

protected:
	float Mass;
};

UCLASS(BlueprintType)
class GALACTITIOUS_API AGravitySimulationActor : public AActor
{
	GENERATED_BODY()

	using TreeType = openvdb::tree::Tree4<float, 5, 4, 3>::Type;
	using GridType = openvdb::Grid<TreeType>;

public:
	AGravitySimulationActor();

	virtual void BeginPlay() override;

private:
	GridType::Ptr Grid;
};
