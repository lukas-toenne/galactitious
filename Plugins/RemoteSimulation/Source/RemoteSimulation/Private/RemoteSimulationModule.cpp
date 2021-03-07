// Fill out your copyright notice in the Description page of Project Settings.

#include "CoreMinimal.h"
#include "RemoteSimulationCommon.h"

#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogRemoteSimulation);

class FRemoteSimulationModule : public IModuleInterface
{
	// Begin IModuleInterface Interface
	virtual void StartupModule() override {}
	virtual void ShutdownModule() override {}
	// End IModuleInterface Interface
};

IMPLEMENT_MODULE(FRemoteSimulationModule, RemoteSimulation)
