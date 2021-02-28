// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "FastMultipoleTypes.generated.h"

UENUM()
enum class EFastMultipoleSimulationIntegrator : uint8
{
	Euler,
	Leapfrog,
};

UENUM()
enum class EFastMultipoleSimulationStatus : uint8
{
	NotInitialized,
	Stopped,
	Success,
};

UENUM()
enum class EFastMultipoleSimulationForceMethod : uint8
{
	Direct,
	FastMultipole,
};

USTRUCT(BlueprintType)
struct FASTMULTIPOLESIMULATION_API FFastMultipoleSimulationSettings
{
	GENERATED_BODY()

	/** Size of the simulation time step. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float StepSize = 1.0f;

	/** Integrator method for advancing points. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EFastMultipoleSimulationIntegrator Integrator = EFastMultipoleSimulationIntegrator::Leapfrog;

	/** Force computation method. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EFastMultipoleSimulationForceMethod ForceMethod = EFastMultipoleSimulationForceMethod::FastMultipole;

	/** Softening radius of the gravitational potential to avoid instabilities at small distances.
	 * This avoids singularities when particles come too close:
	 * F = A*m1*m2 / (r^2 + s^2)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float GravitySofteningRadius = 0.1f;
};
