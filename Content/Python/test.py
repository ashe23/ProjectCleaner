import unreal

subsystem_scanner = unreal.get_engine_subsystem(unreal.PjcScannerSubsystem)


# asset_exclude_settings = unreal.PjcAssetExcludeSettings()
# asset_exclude_settings.excluded_folders = ["/Game/StarterContent"]
# asset_exclude_settings.excluded_classes = [unreal.Material.static_class().get_name()]
# asset_exclude_settings.excluded_assets = ["DialogueVoice'/Game/ParagonAurora/Audio/Aurora.Aurora'"]


settings = unreal.PjcFileExcludeSettings()
# settings.excluded_extensions = [".txt"]
# settings.excluded_folders = ["W:/ue_projects/Workshop427/Content/ParagonCountess"]
settings.excluded_files = ["W:/ue_projects/Workshop427/Content/ParagonCountess/Placeholder.txt"]

subsystem_scanner.scan_project_paths(settings)
print(subsystem_scanner.get_files_by_category(unreal.PjcFileCategory.EXCLUDED))

