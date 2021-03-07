// Fill out your copyright notice in the Description page of Project Settings.

#include "CoreMinimal.h"

#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"

class FRemoteSimulationRenderingModule : public IModuleInterface
{
	// Begin IModuleInterface Interface
	virtual void StartupModule() override
	{
		AddShaderSourceDirectoryMapping(
			TEXT("/Plugin/RemoteSimulation"),
			FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("RemoteSimulation"))->GetBaseDir(), TEXT("Shaders")));
	}
	virtual void ShutdownModule() override {}
	// End IModuleInterface Interface
};

IMPLEMENT_MODULE(FRemoteSimulationRenderingModule, RemoteSimulationRendering)
