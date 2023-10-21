// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Playground : ModuleRules
{
	public Playground(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;		

		PublicDependencyModuleNames.AddRange(new string[] { 
			"Core", 
			"CoreUObject", 
			"Engine", 
			"InputCore", 
			"HeadMountedDisplay", 
			"EnhancedInput",
			"CrabToolsUE5",});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"UnrealEd",
			"EnhancedInput",
			"Engine",
			"CrabToolsUE5",});

		PrivateIncludePathModuleNames.AddRange(new string[] {
			"CrabToolsUE5",
		});

		/*
		PublicIncludePaths.AddRange(
			new string[] {
				"CrabToolsUE5/Public",
			});
		*/
	}
}
