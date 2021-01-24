// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class GalactitiousEditor : ModuleRules
{
	public GalactitiousEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "Galactitious" });

		PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd" });
	}
}
