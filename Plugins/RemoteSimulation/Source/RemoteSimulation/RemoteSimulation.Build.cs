// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class RemoteSimulation : ModuleRules
{
	public RemoteSimulation(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"RemoteSimulationRendering",
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"CoreUObject",
			"Engine",
			"RenderCore",
			"RHI",
		});
	}
}
