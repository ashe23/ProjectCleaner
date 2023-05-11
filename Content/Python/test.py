import unreal

sub = unreal.get_editor_subsystem(unreal.PjcSubsystem)
# sub.scan_project_assets()
print(sub.get_folders_empty())