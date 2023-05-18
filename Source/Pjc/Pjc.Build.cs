// Copyright Ashot Barkhudaryan. All Rights Reserved.

using UnrealBuildTool;

public class Pjc : ModuleRules
{
	public Pjc(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateIncludePaths.AddRange(
			new[]
			{
				"Pjc/Private",
				"Pjc/Private/Commandlets"
			}
		);


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
				"ContentBrowserData",
				"InputCore",
				"EditorSubsystem",
				"Blutility",
				"EditorScriptingUtilities",
				"EditorWidgets",
				"IntroTutorials",
				"UMGEditor",
				"AssetManagerEditor",
				// "DeveloperSettings",
				// "AssetTools",
				// "LevelEditor",
				// "ConfigEditor",
				// "Settings",
			}
		);
	}
}