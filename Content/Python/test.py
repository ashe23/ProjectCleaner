import unreal

sub = unreal.get_editor_subsystem(unreal.PjcSubsystem)
assets_indirect = sub.get_assets_by_category(unreal.PjcAssetCategory.INDIRECT)

for asset in assets_indirect:
    infos = sub.get_asset_indirect_info(asset)
    for info in infos:
        print(f"{info.file_path} - {info.file_num}")