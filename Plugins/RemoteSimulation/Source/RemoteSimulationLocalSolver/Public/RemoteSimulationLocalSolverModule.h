// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RemoteSimulationFrame.h"
#include "RemoteSimulationLocalSolverTypes.h"

#include "Modules/ModuleInterface.h"

class URemoteSimulationCache;
class UWorld;

/**
 * Module class for managing simulations.
 */
class REMOTESIMULATIONLOCALSOLVER_API FRemoteSimulationLocalSolverModule : public IModuleInterface
{
public:
	/**
	 * Singleton-like access to this module's interface.  This is just for convenience!
	 * Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	 *
	 * @return Returns singleton instance, loading the module on demand if needed
	 */
	static FRemoteSimulationLocalSolverModule& Get();

	/**
	 * Store finished simulation frames in the cache.
	 * @return Number of completed frames that have been added to the cache.
	 * TODO this should probably be done with a delegate running on the game thread
	 */
	int32 CacheCompletedSteps(URemoteSimulationCache* SimulationCache);

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
		const FRemoteSimulationSolverSettings& Settings, FRemoteSimulationInvariants::ConstPtr Invariants,
		FRemoteSimulationFrame::Ptr StartFrame, UWorld* DebugWorld = nullptr);
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
	static FRemoteSimulationLocalSolverModule* Singleton;

	/** Thread that runs the simulation. */
	class TUniquePtr<struct FRemoteSimulationThreadRunnable> ThreadRunnable;
};
