# ProjectCleaner
Unreal engine plugin for managing unused assets and empty folders in your project.

No more need for Migration and other hacky methods.

Versions Supported: 4.27, 5.0 - 5.8 (single `main` branch for all versions)   
Platform: Windows, Linux and Mac

<img src="https://github.com/ashe23/ProjectCleaner/assets/8270558/265ed34e-1b7b-46fb-953c-bc0d37c10fd1" />

# Installation
The plugin is available on the marketplace [Marketplace URL](https://www.unrealengine.com/marketplace/en-US/product/4d7f5dc837fc4b009bb91e678adf9fd0)

# Installation from github
1) Create 'Plugins' folder in your project root directory
2) Download appropriate version of .zip in releases
3) Extract to Plugins folder
4) Open your project, and you're done!

# Installation from source
1) Create 'Plugins' folder in your project root directory
2) Clone the repository to the 'Plugins' folder (`main` branch works for every supported engine version).
3) Build the solution and you're done!

# Before you clean
Project Cleaner deletes files from your project. It can not know every way a project loads assets (for example by
name from code at runtime), so treat its list as suggestions:
1. Save your work to version control (Git, Perforce, Plastic, SVN) or copy the project folder somewhere safe.
2. Scan, then look through the unused list and the folder tree before deleting. Exclude anything you are not sure about.
3. Clean, then open the project and play your main levels before committing.

If something goes wrong, restore the deleted files from version control or from your copy.

# Features
* Detection of unused assets
* Detection of corrupted assets
* Detection of indirect assets
* Detection of external files
* Empty folders detection
* Scanning of project plugins content (optional, for plugin based projects like Lyra)
* Subsystem class that exposes most of the plugin functionality, which can be utilized in Blueprints or Python scripts.
* Command line interface using Commandlets
* Works fast on Large projects
* Intiutive and informative user interface
* Per-folder asset usage information
* Configurable scanning and exclusion settings

# What counts as used
An asset is used when something that ships or runs needs it, directly or through its dependencies (hard and soft references):
* primary assets of Asset Manager types (maps are primary by default), including types registered by Game Feature data
* assets referenced from outside scanned content (engine, not scanned plugins, World Partition and one file per actor packages)
* assets named in project or plugin `Source` and `Config` files
* editor utility assets, cinematic assets (settings), always used classes and excluded assets
* assets in folders of Primary Asset Labels with "Label Assets In My Directory" (setting, on by default)
* GameplayCue notifies (loaded by tag from `GameplayCueNotifyPaths`)

Redirectors are assets too: one referenced by a used asset is used, an unreferenced one is unused. Scanning and cleanup work without fixing them, **Fix Redirectors** (toolbar, `-fix_redirectors`) resaves referencing assets and deletes fixed redirectors.

Cleanup never force deletes. An unused asset that something still holds in memory (an open asset editor, a loaded level, an engine helper object) is kept and listed in Output Log together with its holder. Close the editor that holds it and clean again, or delete it from the Content Browser yourself.

Non asset files the engine or the project reads are never external files: `Localization`, `Movies`, `Splash`, `Python` and `Collections` folders of every content root, folders staged as raw files in packaging settings (`DirectoriesToAlwaysStageAsUFS`/`NonUFS`) and files named in `Source`/`Config` (e.g. a Slate brush `.svg`).

# Settings
Project Cleaner window > Settings menu, or:

* **Project Settings > Plugins > Project Cleaner** - rules shared with the team (saved to `DefaultEditor.ini`)
  * Scan project plugins, ignored plugins
  * Primary assets are used, ignored primary asset types
  * Folder labels mark assets as used (Primary Asset Labels with "Label Assets In My Directory"), on by default.
    Such labels are cook rules, often for content loaded by path. A label in content root (e.g. Lyra `DefaultGame_Label`)
    makes every asset used: the scan reports how many assets only labels keep (Folder Labels row, Output Log), turn it off to review them.
  * Cinematic assets are used (Level Sequences, Template Sequences, Movie Render Queue/Graph configs), on by default
  * Editor utility assets are used, indirect (source code and config) assets are used
  * Always used classes
* **Editor Preferences > Plugins > Project Cleaner** - personal preferences
  * Toolbar button location (or hidden)
  * Scan on window open
  * Ask before Fix Redirectors runs
  * Skip engine "About to load N assets" question while deleting (UE 5.4+), on by default

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
| `-fix_redirectors` | fix redirectors before scanning (resaves referencing assets) |
| `-list` | print found items, not only counts |
| `-report=<file.json>` | write stats and every found item as json (CI, scripts) |

Exit code is `0` on success, `1` when something failed to delete. The engine also returns `1` when anything logs an error (e.g. engine ensures), check the report `result` field in CI. UE4 uses `UE4Editor-Cmd.exe`.

# Development
Plugin is developed against every supported engine at once. `Scripts/pjc.py` (Python 3, no dependencies) generates a test host project per installed engine, links this repository into them and builds or tests all of them:
```
python Scripts/pjc.py engines              # list found engines
python Scripts/pjc.py build -j 4           # compile for every engine
python Scripts/pjc.py test -j 4            # automation tests + commandlet tests
python Scripts/pjc.py test -e 5.4,5.6      # selected engines, or ranges like 5.0-5.3
python Scripts/pjc.py cli -e 5.5 -- -scan_only -list
python Scripts/pjc.py package -e 5.5       # RunUAT BuildPlugin
python Scripts/pjc.py polygon -j 3         # clean real projects (e.g. Lyra) for real and verify, see below
python Scripts/pjc.py lint                 # include case (Linux/Mac) and clang-format checks
```
Engines are found automatically (Epic launcher, registry, `~/.config/Epic/UnrealEngine/Install.ini`, default install folders). Override with `PJC_ENGINES` (engine roots separated by `;` on Windows, `:` elsewhere) or `Scripts/pjc.local.json`:
```json
{ "engines": ["D:/Engines/UE_5.5"], "hosts_dir": "D:/ue_hosts" }
```
Host projects go to `PJC_HOSTS_DIR` or `<repo>/../hosts` by default. Automation tests live in `Source/Pjc/Private/Tests` (`ProjectCleaner.*`). Tests that create and delete content run only inside generated host projects. Commandlet tests add a Lyra like layout to the host: game feature plugin in `Plugins/GameFeatures`, level with one file per actor, redirectors, staged and localization files and a plugin that is present but not enabled.

## Polygon projects
`polygon` runs the plugin on real projects (Lyra samples work well) and checks the result independently of the plugin: a Python script inside the editor (`Scripts/pjc_snapshot.py`) records the package graph of the clean project, then every scenario (scans with different settings, redirector fix, full cleanup with and without plugin content) is verified against it: no kept asset may reference a deleted one, Asset Manager primary assets, assets named in `Config`/`Source` and GameplayCue notifies must survive, nothing outside scanned content may change and a rescan after cleanup must be clean.

Each project must be the root of its own git repository (local is enough), it is reset with `git reset --hard` + `git clean -fd` before and after every scenario. Untracked build output (`Binaries`, `Intermediate`, `Saved`, `DerivedDataCache`) should be ignored. The plugin build of the matching host project is copied into `Plugins/ProjectCleaner`, other copies of the plugin are moved aside for the run. Projects come from arguments, `PJC_POLYGONS` or `pjc.local.json`:
```json
{ "polygons": ["D:/Projects/Lyra5*"] }
```
