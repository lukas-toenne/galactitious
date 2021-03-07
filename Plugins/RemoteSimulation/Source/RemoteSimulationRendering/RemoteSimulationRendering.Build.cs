// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class RemoteSimulationRendering : ModuleRules
{
	public RemoteSimulationRendering(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"CoreUObject",
			"Projects",
			"RenderCore",
			"RHI",
		});

		PrivateIncludePathModuleNames.AddRange(new string[] { "RemoteSimulation" });
	}
}
