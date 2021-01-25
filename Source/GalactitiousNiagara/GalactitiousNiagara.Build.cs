// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class GalactitiousNiagara : ModuleRules
{
	public GalactitiousNiagara(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// For boost:: and TBB:: code
		bUseRTTI = true;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"FastMultipoleSimulation",
			"Galactitious",
			"GalactitiousOpenVDB",
			"Niagara",
			"UEOpenExr"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"NiagaraCore",
			"NiagaraShader",
			"RenderCore",
			"Renderer",
			"RHI",
		});
	}
}
