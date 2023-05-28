# import unreal

# sub = unreal.get_editor_subsystem(unreal.PjcSubsystem)

# QUERY
# sub.get_assets_all() - returns all assets in project (inside Content Folder only)
# sub.get_assets_used() - returns all used assets in project
# sub.get_assets_unused() - returns all unused assets in project
# sub.get_assets_primary() - returns all primary assets in project
# sub.get_assets_indirect() - return all assets that used in source code or config files
# sub.get_assets_circular() - return all assets that have circular dependencies
# sub.get_assets_editor() - return all editor specific assets in project
# sub.get_assets_excluded() - return all excluded assets by user
# sub.get_assets_ext_referenced - return all assets that have external referencered outside Content folder
# sub.get_files()
# sub.get_files_by_ext()
# sub.get_files_external() - return all external files inside Content folder
# sub.get_files_corrupted() - return all corrupted asset files inside Content folder
# sub.get_folders()
# sub.get_folders_empty() - return all empty folders inside Content folder
# sub.get_class_names_primary()
# sub.get_class_names_editor()
# sub.get_class_names_excluded()

# CHECKS
# sub.asset_is_blueprint()
# sub.asset_is_ext_referenced()
# sub.asset_is_circular()

# ACTIONS
# sub.delete_assets_unused()
# sub.delete_folders_empty()
# sub.delete_files_external()
# sub.delete_files_corrupted()

# for f in sub.get_folders_empty():
    # print(f)

# ./UE4Editor-Cmd.exe "C:\dev\ue4\MyProject\MyProject.uproject" -run=ProjectCleanerCLI
# UE4Editor.exe <GameName or uproject> -run=ResavePackages -fixupredirects -autocheckout -projectonly -unattended
cmd_ref = "C:\\Program Files\\Epic Games\\UE_4.27\\Engine\\Binaries\\Win64\\UE4Editor.exe \"W:/dev/ue_projects/Workshop427/Workshop427.uproject\" -run=ResavePackages -fixupredirects -autocheckout -projectonly -unattended"

cmd = "C:\\Program Files\\Epic Games\\UE_4.27\\Engine\\Binaries\\Win64\\UE4Editor-Cmd.exe \"W:/dev/ue_projects/Workshop427/Workshop427.uproject\" -run=Pjc -NoShaderCompile -delete_assets_unused -delete_folders_empty -delete_files_external -delete_files_corrupted"

import subprocess

subprocess.call(cmd)