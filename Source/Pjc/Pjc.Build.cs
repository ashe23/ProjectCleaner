// Copyright Ashot Barkhudaryan. All Rights Reserved.

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
				// "UnrealEd",
				// "UMGEditor",
				// "EditorStyle",
				// "EditorSubsystem",
				// "EditorWidgets",
				// "ContentBrowser",
				// "ContentBrowserData",
				// "Blutility",
				// "DeveloperSettings",
				// "InputCore",
				// "AssetManagerEditor",
				// "IntroTutorials",
				// "AssetTools",
				// "AssetRegistry",
				// "LevelEditor",
				// "PropertyEditor",
				// "ConfigEditor",
				// "Settings",
				// "EditorScriptingUtilities"
			}
		);
	}
}