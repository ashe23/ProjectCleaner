import unreal
import os

# used assets
# - primary
# - excluded
# - indirect
# - ext_referenced
# - editor
# - megascans


subsystem = unreal.get_editor_subsystem(unreal.PjcSubsystem)

exclude_settings = unreal.PjcAssetExcludeSettings()
# exclude_settings.excluded_package_paths = ["/Game/ParagonProps"]
exclude_settings.excluded_object_paths = ["Material'/Game/StarterContent/Materials/M_ColorGrid_LowSpec.M_ColorGrid_LowSpec'"]

subsystem.scan_project(exclude_settings)
print(len(subsystem.get_assets_all()))
# subsystem.get_assets_all()
# subsystem.get_assets_primary()
# subsystem.get_assets_editor()
# subsystem.get_assets_indirect()
# subsystem.get_assets_indirect_with_info()
# subsystem.get_assets_ext_referenced()
# subsystem.get_assets_excluded(exclude_settings)
# subsystem.get_assets_used(exclude_settings)
# subsystem.get_assets_unused(exclude_settings)
# subsystem.get_assets_size()
# subsystem.get_files_size()
# subsystem.get_files_external()
# subsystem.get_files_corrupted()
# subsystem.get_folders_empty()
# subsystem.get_assets_by_path()
# subsystem.get_assets_by_paths()
# subsystem.get_assets_by_object_path()
# subsystem.get_assets_by_object_paths()
# subsystem.get_assets_by_class_name()
# subsystem.get_assets_by_class_names()
