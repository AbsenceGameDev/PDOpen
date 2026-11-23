// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class PDOpenSource : ModuleRules
{
	public PDOpenSource(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Classes"));
		if (Target.Type == TargetType.Editor)
		{
			PublicDependencyModuleNames.AddRange(new string[]
			{ 
				/*Tag*/          "GameplayTagsEditor"
			});

			PrivateDependencyModuleNames.AddRange(new string[]
			{ 
				/*Editor*/       "UnrealEd", "PropertyEditor"
			});
		}


		PublicDependencyModuleNames.AddRange(new string[]
		{ 
			/*Core*/         "Engine", "Core", "CoreUObject", "NetCore", "ApplicationCore", "RenderCore", "RHICore",
			/*App*/          "AppFramework",
			/*Rendering*/    "RHI",
			/*Settings*/     "DeveloperSettings", 
			/*Tag*/          "GameplayTags",
			/*Input*/        "InputCore",  "EnhancedInput", 
			/*Conversation*/ "CommonConversationRuntime", 
			/*PermaDev*/     "PDRTSBase", "PDInventory", "PDConversationHelper", "PDSharedUI", "PDInteraction", "RTSShaders",
		});

		PublicDependencyModuleNames.AddRange(new string[]
		{
			/*Mass*/     "MassEntity", "MassCommon", "MassNavigation", "MassMovement", "MassAIBehavior", "MassSmartObjects", "MassSignals", "MassRepresentation", "MassLOD", "MassSpawner", 
			/*AI*/       "AIModule", "StateTreeModule", "SmartObjectsModule", "NavigationSystem", 
			/*Struct*/   "StructUtils", 
			/*Animation*/"AnimToTexture", 
			/*Physics*/  "Chaos",

		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{ 	
			/*Tag*/          "GameplayTags", 
			/*Input*/        "EnhancedInput", 
			/*Widget*/       "SlateCore", "Slate", "CommonUI", "UMG", 
			/*Effects*/      "Niagara", "MassCrowd", 
            /*Conversation*/ "CommonConversationRuntime", 
			/*PermaDev*/     "PDInteraction", "PDInventory", "PDRTSBase", "PDConversationHelper", "PDUserMessageBase",
		});
	}
}
