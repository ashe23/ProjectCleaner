# ProjectCleaner
Unreal engine plugin for managing unused assets and empty folders in your project.

No more need for Migration and other hacky methods.

Versions Supported: 4.27, 5.0 - 5.8   
Platform: Windows, Linux and Mac

<img src="https://github.com/ashe23/ProjectCleaner/assets/8270558/265ed34e-1b7b-46fb-953c-bc0d37c10fd1" />

# Installation
* [Marketplace](https://www.unrealengine.com/marketplace/en-US/product/4d7f5dc837fc4b009bb91e678adf9fd0)
* From GitHub: download the zip for your engine version from releases and extract it to `<YourProject>/Plugins`
* From source: clone this repository (`main` branch) into `<YourProject>/Plugins` and build your project

# Before you clean
Project Cleaner deletes files and can not know every way a project loads assets, so treat its results as suggestions:
1. Save your work to version control (Git, Perforce, ...) or copy the project folder somewhere safe.
2. Scan, look through the unused assets and exclude anything you are not sure about.
3. Clean, then open the project and play your main levels before committing.

# Features
* Detection of unused assets, indirect assets (used in source code or config files), corrupted assets, external files and empty folders
* Per-folder asset usage information
* Asset Usage tab (experimental): project assets as a zoomable graph, shows why each asset is used or unused
* Optional scanning of project plugins content (for plugin based projects like Lyra)
* Configurable scanning and exclusion settings
* Subsystem for Blueprints and Python, command line interface

An asset is used when a used asset references it, or for its own reason: primary asset (like maps), named in source or config files, editor utility, excluded by you or referenced from outside scanned content. Cleanup never force deletes, assets still held in memory are kept and listed in Output Log.

# Settings
Project Cleaner window > Settings menu, or **Project Settings > Plugins > Project Cleaner** (rules shared with the team) and **Editor Preferences > Plugins > Project Cleaner** (personal preferences).

# Command line
```
UnrealEditor-Cmd.exe <Project.uproject> -run=Pjc <switches>
```
| Switch | Description |
|---|---|
| `-scan_only` | print stats only |
| `-full_cleanup` | all delete operations below |
| `-delete_assets_unused` | delete unused assets |
| `-delete_folders_empty` | delete empty folders |
| `-delete_files_external` | delete external (non asset) files in content folders |
| `-delete_files_corrupted` | delete corrupted asset files |
| `-fix_redirectors` | fix redirectors before scanning |
| `-list` | print found items, not only counts |
| `-report=<file.json>` | write results as json |

UE4 uses `UE4Editor-Cmd.exe`.

# Development
`python Scripts/pjc.py --help` builds and tests the plugin against every installed engine.
