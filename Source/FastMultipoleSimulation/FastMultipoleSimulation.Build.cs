// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class FastMultipoleSimulation : ModuleRules
{
	public FastMultipoleSimulation(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// For boost:: and TBB:: code
		bUseRTTI = true;

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
