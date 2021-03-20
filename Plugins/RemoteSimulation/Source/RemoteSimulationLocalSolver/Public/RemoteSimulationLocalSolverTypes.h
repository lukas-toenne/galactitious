// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "RemoteSimulationLocalSolverTypes.generated.h"

UENUM()
enum class ERemoteSimulationIntegrator : uint8
{
	Euler,
	Leapfrog,
};

UENUM()
enum class ERemoteSimulationResultStatus : uint8
{
	NotInitialized,
	Stopped,
	Success,
};

UENUM()
enum class ERemoteSimulationForceMethod : uint8
{
	Direct,
	FastMultipole,
};

USTRUCT(BlueprintType)
struct REMOTESIMULATIONLOCALSOLVER_API FRemoteSimulationSolverSettings
{
	GENERATED_BODY()

	/** Size of the simulation time step. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float StepSize = 1.0f;

	/** Integrator method for advancing points. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ERemoteSimulationIntegrator Integrator = ERemoteSimulationIntegrator::Leapfrog;

	/** Force computation method. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ERemoteSimulationForceMethod ForceMethod = ERemoteSimulationForceMethod::FastMultipole;

	/** Softening radius of the gravitational potential to avoid instabilities at small distances.
	 * This avoids singularities when particles come too close:
	 * F = A*m1*m2 / (r^2 + s^2)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float GravitySofteningRadius = 0.1f;
};
