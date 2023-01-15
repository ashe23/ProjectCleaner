import unreal
import os

# project cleaner subsystem api
# all operations are performed only inside Content (/Game) folder

# query functions
# ---------------
# get_assets_all - return all assets inside Content folder
# get_assets_by_path(paths, exclude_paths) - return all assets with specified paths

# get_assets_by_class(class_names, exclude_classes) - return all assets with specified classes

# get_assets_primary
# get_assets_indirect
# get_assets_indirect_info
# get_assets_excluded ??
# get_assets_used
# get_assets_unused
# get_assets_dependencies
# get_assets_referencers
# get_assets_by_filter
# get_assets_used_by_filter
# get_assets_unused_by_filter
# get_files_corrupted
# get_files_non_engine
# ---------------

# check functions
# -----------------
# asset_is_used
# asset_is_unused
# asset_is_indirect
# asset_is_excluded ??
# asset_is_primary
# asset_is_corrupted

# utility functions
# -----------------
# get_assets_primary_class_names(bIncludeDerivedClasses) - return all primary assets class names including/excluding derived classes


# actions functions
# -----------------
# remove_assets_unused
# remove_files_corrupted
# remove_files_non_engine
# remove_folders_empty

# export functions
# -----------------
# export_to_file
# export_to_json

# before any query operations, we should ensure that
# 1. AssetRegistry is not working
# 2. Editor is not in play mode or in simulation
# 3. All Redirectors are fixed and none exists
# 4. All assets saved successfully

# folders that must be never scanned __ExternalActors__ and __ExternalObjects__. This is only for UE5 version
# folders that must be never deleted or marked as empty
# Developers folder or any folder under it
# Collections folder

# def sizeof_fmt(num, suffix="B"):
#     for unit in ["", "Ki", "Mi", "Gi", "Ti", "Pi", "Ei", "Zi"]:
#         if abs(num) < 1024.0:
#             return f"{num:3.1f}{unit}{suffix}"
#         num /= 1024.0
#     return f"{num:.1f}Yi{suffix}"

subsystem = unreal.get_editor_subsystem(unreal.ProjectCleanerSubsystem)
asset_registry = unreal.AssetRegistryHelpers.get_asset_registry()

assets_all = subsystem.get_assets_all()

with open('D:/assets.txt', 'w') as f:
    for asset in assets_all:
        f.write(f"{asset.asset_name} - {asset.asset_class} - {subsystem.asset_is_blueprint(asset, True)}\n")
# class_names = subsystem.get_class_names_derived(["Blueprint", "BP_MyActor_C", "DataAsset"])
# for name in class_names:
#     print(name)
# asset = asset_registry.get_asset_by_object_path("ItemMaster'/Game/Stagings/ItemMaster_01.ItemMaster_01'")
# unreal.log(asset.asset_class)
# unreal.log(subsystem.asset_is_blueprint(asset))
# search_filter = unreal.ProjectCleanerAssetSearchFilter()

# search_filter.recursive_paths = True
# # search_filter.recursive_classes = False
# search_filter.scan_paths = ["/Game/ParagonAurora/Characters/Heroes/Aurora"]
# search_filter.scan_class_names = ["Material"]
# search_filter.exclude_class_names = ["Texture"]
# search_filter.exclude_paths = ["/Game/StarterContent/Props"]
# search_filter.exclude_assets = ["Texture2D'/Game/StarterContent/Textures/T_Brick_Clay_Beveled_D.T_Brick_Clay_Beveled_D'"]

# assets_all = subsystem.get_assets_all(False)
# assets = subsystem.get_assets_by_class(["Blueprint"], ["BP_MyActor_C", "AnimBlueprint"], True)

# unreal.log(len(assets_all))
# unreal.log(len(assets))