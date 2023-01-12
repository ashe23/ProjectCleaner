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

def sizeof_fmt(num, suffix="B"):
    for unit in ["", "Ki", "Mi", "Gi", "Ti", "Pi", "Ei", "Zi"]:
        if abs(num) < 1024.0:
            return f"{num:3.1f}{unit}{suffix}"
        num /= 1024.0
    return f"{num:.1f}Yi{suffix}"

subsystem = unreal.get_editor_subsystem(unreal.ProjectCleanerSubsystem)

search_filter = unreal.ProjectCleanerAssetSearchFilter()
search_filter.recursive_paths = True
search_filter.recursive_classes = False
# search_filter.scan_paths = ["/Game/StarterContent/Blueprints"]
search_filter.scan_class_names = ["BP_Parent_Actor_C"]
search_filter.exclude_class_names = ["BP_Child_Actor_C", "BP_Parent_Actor_C"]
# search_filter.scan_class_names = ["Blueprint_Effect_Fire_C"]
# search_filter.exclude_paths = ["/Game/StarterContent/Props"]


assets = subsystem.get_assets_by_filter(search_filter)
unreal.log(len(assets))

# folders = subsystem.get_folders_empty("/Game/StarterContent")
# for folder in folders:
#     unreal.log(folder)


# get all unused materials and textures
# search_settings.class_names.add("Material")
# search_settings.class_names.add("Texture")
# search_settings.path.add("/Game")

# get all unused assets excluded Materials and Textures
# search_settings.exclude_class_names.add("Material")
# search_settings.exclude_class_names.add("Texture")
# search_settings.path.add("/Game")
# subsystem.get_assets_unused(search_settings)