import unreal
import os

# used assets
# - primary
# - excluded
# - indirect
# - ext_referenced
# - editor
# - megascans



# exclude_settings = unreal.PjcExcludeSettings()
# exclude_settings.excluded_paths = ["/Game/StarterContent"]

subsystem = unreal.get_editor_subsystem(unreal.PjcSubsystem)
subsystem.test("Blueprint'/Game/ParagonAurora/Characters/Heroes/Aurora/AuroraPlayerCharacter.AuroraPlayerCharacter'")

# scan_data = subsystem.project_scan(exclude_settings) # uses provided exclude settings
# scan_data = subsystem.project_scan() # uses exclude settings specified in editor

# subsystem.project_clean(unreal.PjcCleanupMethod) 

# subsystem.project_scan(exclude_settings)
# subsystem.project_scan_by_exclude_settings(exclude_settings)



