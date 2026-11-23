// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class PDConversationHelper : ModuleRules
{
	public PDConversationHelper(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core", 
				"CommonConversationRuntime",
				"GameFeatures",
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"GameFeatures",
				"InputCore",
				"Slate",
				"SlateCore", 
			}
			);
	}
}
