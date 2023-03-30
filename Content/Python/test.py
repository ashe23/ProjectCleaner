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

scan_settings = unreal.PjcScanSettings()
scan_settings.excluded_paths = ["/Game/Developers"]
scan_settings.excluded_class_names = [unreal.StaticMesh.static_class().get_name(), 
                                      unreal.Texture2D.static_class().get_name(), 
                                      unreal.Material.static_class().get_name(),
                                      unreal.Blueprint.static_class().get_name(),
                                      unreal.SoundWave.static_class().get_name()]
scan_settings.excluded_object_paths = ["ParticleSystem'/Game/StarterContent/Particles/P_Explosion.P_Explosion'"]
scan_result = subsystem.project_scan_by_settings(scan_settings)
# print(scan_result)

# scan_data = subsystem.project_scan(exclude_settings) # uses provided exclude settings
# scan_data = subsystem.project_scan() # uses exclude settings specified in editor

# subsystem.project_clean(unreal.PjcCleanupMethod) 

# subsystem.project_scan(exclude_settings)
# subsystem.project_scan_by_exclude_settings(exclude_settings)



