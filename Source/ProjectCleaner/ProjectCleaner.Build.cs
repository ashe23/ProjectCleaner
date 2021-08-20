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
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Projects",
				"InputCore",
				"EditorFramework",
				"UnrealEd",
				"ToolMenus",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",				
				"LevelEditor",
				"ContentBrowser",
				"EditorStyle",
				"PropertyEditor",
				"AssetTools",
				"AssetRegistry"
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
