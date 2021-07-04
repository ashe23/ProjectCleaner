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
				"EditorStyle",
				"PropertyEditor",
				"UnrealEd",
				"ToolMenus",
				"AssetTools",
			}
		);


		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
			}
		);
	}
}