// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Playground : ModuleRules
{
	public Playground(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(
			new string[] {
				
			});

		PublicDependencyModuleNames.AddRange(new string[] { 
			"Core", 
			"CoreUObject", 
			"Engine", 
			"InputCore", 
			"HeadMountedDisplay", 
			"AnimGraph", 
			"AnimGraphRuntime", 
			"EnhancedInput",
			"BlueprintGraph",});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"UnrealEd",
			"AnimGraph",
			"AnimGraphRuntime",
			"BlueprintGraph", 
			"EnhancedInput",
			"Engine"});
	}
}
