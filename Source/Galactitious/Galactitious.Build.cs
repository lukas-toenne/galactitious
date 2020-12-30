// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class Galactitious : ModuleRules
{
	public Galactitious(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "Niagara", "VDB" });

		PrivateDependencyModuleNames.AddRange(new string[] {  });

		if (Target.bBuildEditor == true)
		{
			PrivateDependencyModuleNames.Add("UnrealEd");
		}
	}
}
