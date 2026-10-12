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
                // core modules
				"Engine",
				"CoreUObject",
				"DeveloperSettings",
				"Settings",
				"Slate",
				"SlateCore",
				"AssetRegistry",
				"AssetTools",
                // editor modules
				"AssetManagerEditor",
				"Blutility",
				"BlueprintGraph",
				"ToolMenus",
				"Projects",
				"SourceControl",
				"PropertyEditor",
				"UnrealEd",
				"EditorStyle",
				"EditorSubsystem",
				"EditorScriptingUtilities",
				"EditorWidgets",
				"InputCore",
				"ContentBrowser",
				"ContentBrowserData",
				"UMGEditor",
			}
		);
		
        // modules require for engine version 5.x+
		if (Target.Version.MajorVersion >= 5)
		{
			PrivateDependencyModuleNames.Add("RenderCore");
			PrivateDependencyModuleNames.Add("EditorFramework");
			PrivateDependencyModuleNames.Add("DeveloperToolSettings");
		}
	}
}