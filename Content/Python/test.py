import unreal

# project cleaner subsystem api
# all operations are performed only inside Content (/Game) folder

# query functions
# ---------------
# get_assets
# get_assets_primary
# get_assets_indirect
# get_assets_indirect_info
# get_assets_excluded ??
# get_assets_used
# get_assets_unused
# get_assets_dependencies
# get_assets_referencers
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


subsystem = unreal.get_editor_subsystem(unreal.ProjectCleanerSubsystem)

assets = subsystem.get_assets_by_path(["/Game/AnimStarterPack"], False)
assets_rec = subsystem.get_assets_by_path(["/Game/AnimStarterPack"], True)

unreal.log(len(assets))
unreal.log(len(assets_rec))


# scan_data = subsystem.project_scan()
# subsystem.remove_unused_assets()
# subsystem.remove_empty_folders()
# unreal.log(len(scan_data.assets_all))

# assets = unreal.ProjectCleanerLibAsset.get_assets_indirect_info()

# for asset in assets:
    # print(asset.asset_data.object_path)
    # print(asset.file_path)