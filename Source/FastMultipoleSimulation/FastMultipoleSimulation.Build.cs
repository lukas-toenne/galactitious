// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class FastMultipoleSimulation : ModuleRules
{
	public FastMultipoleSimulation(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"GalactitiousOpenVDB",
			"UEOpenExr"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
		});
	}
}
