# ProjectCleaner
Unreal Engine plugin that finds unused assets and other content problems in your project, and cleans them up.

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
* **Overview**: project numbers, what keeps assets used, storage by class and folder, issues, and the last cook compared with the scan
* **Assets Unused**: unused assets folder by folder, as a tree or a size map; delete them with the assets that must go with them, or exclude them
* **Inspector**: why an asset is used, as a graph from every root down to it, with what uses it and what it uses
* **Issues**: redirectors, external files, corrupted files and empty folders
* Scan bar and scan settings panel on every tab, optional scanning of project plugins content (for plugin based projects like Lyra)
* Subsystem for Blueprints and Python, command line interface

An asset is used when a used asset references it, or for its own reason: a map or primary asset, in a folder a Primary Asset Label covers, named in source or config files, an editor utility or editor tool data, a cinematic, a class you mark as always used, excluded by you or referenced from outside scanned content. Editor only uses count as used but are shown apart, because they do not ship with the game. Cleanup never force deletes: assets still held in memory are kept and marked as not deleted.

# Settings
Scan settings panel on every tab, or **Project Settings > Plugins > Project Cleaner** (rules shared with the team). Personal preferences: **Editor Preferences > Plugins > Project Cleaner**. Both open from the Settings menu of the Project Cleaner window.

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
