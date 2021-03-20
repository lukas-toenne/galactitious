// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class RemoteSimulationLocalSolver : ModuleRules
{
	public RemoteSimulationLocalSolver(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// For boost:: and TBB:: code
		bUseRTTI = true;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			// "GalactitiousOpenVDB",
			"RemoteSimulation",
			// "UEOpenExr",
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
		});
	}
}
