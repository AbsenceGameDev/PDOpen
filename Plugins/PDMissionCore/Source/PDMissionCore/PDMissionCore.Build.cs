// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class PDMissionCore : ModuleRules
{
	public PDMissionCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		bLegacyPublicIncludePaths = false;
		ShadowVariableWarningLevel = WarningLevel.Error;

		PrivateIncludePaths.AddRange(
			new string[] {
				"PDMissionCore/Private",
				// ... add other private include paths required here ...
			}
		);

		// Note: Having to add this I realize that I have written a bunch of my slate editor code in the core module 
		if (Target.bBuildEditor)
		{			
			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"PropertyEditor",
				}
				);
			
		}
		
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"DeveloperSettings",
				"InputCore",
                "NetCore",
                "AIModule",
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"UMG",
				"GameplayTags"
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
