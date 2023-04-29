﻿// Copyright Ashot Barkhudaryan. All Rights Reserved.

using UnrealBuildTool;

public class Pjc : ModuleRules
{
	public Pjc(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// PublicIncludePaths.AddRange(
		// 	new[]
		// 	{
		// 		"Runtime/Slate/Public",
		// 	}
		// );


		// PrivateIncludePaths.AddRange(
		// 	new[]
		// 	{
		// 		"Pjc/Private",
		// 		"Pjc/Private/Commandlets"
		// 	}
		// );


		PublicDependencyModuleNames.AddRange(
			new[]
			{
				"Core",
			}
		);


		PrivateDependencyModuleNames.AddRange(
			new[]
			{
				"Engine",
				"CoreUObject",
				"Slate",
				"SlateCore",
				"ToolMenus",
				"Projects",
				"UnrealEd",
				"EditorStyle",
				"AssetRegistry",
				"PropertyEditor",
				"ContentBrowser",
				"InputCore",
				"EditorSubsystem",
				// "UMGEditor",
				// "EditorWidgets",
				// "ContentBrowserData",
				// "Blutility",
				// "DeveloperSettings",
				// "AssetManagerEditor",
				// "IntroTutorials",
				// "AssetTools",
				// "LevelEditor",
				// "ConfigEditor",
				// "Settings",
				// "EditorScriptingUtilities"
			}
		);
	}
}