// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FastMultipoleSimulationFrame.h"
#include "FastMultipoleTypes.h"

#include "Modules/ModuleInterface.h"

class UFastMultipoleSimulationCache;
class UWorld;

/**
 * Module class for managing simulations.
 */
class FASTMULTIPOLESIMULATION_API FFastMultipoleSimulationModule : public IModuleInterface
{
public:
	/**
	 * Singleton-like access to this module's interface.  This is just for convenience!
	 * Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	 *
	 * @return Returns singleton instance, loading the module on demand if needed
	 */
	static FFastMultipoleSimulationModule& Get();

	/**
	 * Store finished simulation frames in the cache.
	 * @return Number of completed frames that have been added to the cache.
	 */
	int32 CacheCompletedSteps(UFastMultipoleSimulationCache* SimulationCache);

	/**
	 * Get the number of steps that are currently scheduled.
	 * @return Number currently scheduled steps.
	 */
	int32 GetNumScheduledSteps() const;

	/**
	 * Schedule a number of frames to precompute.
	 * @param MaxStepsScheduled Maximum steps to schedule, taking already scheduled steps into account.
	 * @return Number of frames that have been scheduled.
	 */
	int32 ScheduleSteps(int32 MaxStepsScheduled);

	/**
	 * Discard all currently scheduled simulation steps.
	 */
	void CancelScheduledSteps();

	void StartSimulation(
		const FFastMultipoleSimulationSettings& Settings, FFastMultipoleSimulationInvariants::ConstPtr Invariants,
		FFastMultipoleSimulationFrame::Ptr StartFrame, UWorld* DebugWorld = nullptr);
	void StopSimulation();

private:
	// IModuleInterface

	/**
	 * Called when Http module is loaded
	 * Load dependent modules
	 */
	virtual void StartupModule() override;

	/**
	 * Called after Http module is loaded
	 * Initialize platform specific parts of Http handling
	 */
	virtual void PostLoadCallback() override;

	/**
	 * Called before Http module is unloaded
	 * Shutdown platform specific parts of Http handling
	 */
	virtual void PreUnloadCallback() override;

	/**
	 * Called when Http module is unloaded
	 */
	virtual void ShutdownModule() override;

private:
	/** Singleton for the module while loaded and available */
	static FFastMultipoleSimulationModule* Singleton;

	/** Thread that runs the simulation. */
	class TUniquePtr<struct FFastMultipoleSimulationThreadRunnable> ThreadRunnable;
};
