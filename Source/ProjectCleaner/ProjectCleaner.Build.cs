// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

using UnrealBuildTool;

public class ProjectCleaner : ModuleRules
{
	public ProjectCleaner(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(
			new string[]
			{
				"Runtime/Slate/Public",
			}
		);


		PrivateIncludePaths.AddRange(
			new string[]
			{
				"ProjectCleaner/Private",
				"ProjectCleaner/Private/Commandlets",
			}
		);


		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
			}
		);


		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Projects",
				"InputCore",
				"UnrealEd",
				"LevelEditor",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"ContentBrowser",
				"ContentBrowserData",
				"EditorStyle",
				"PropertyEditor",
				"UnrealEd",
				"ToolMenus",
				"AssetTools",
				"AssetRegistry",
				"Blutility",
				"UMGEditor",
				"EditorSubsystem",
			}
		);


		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
			}
		);
	}
}