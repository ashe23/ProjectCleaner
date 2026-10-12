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

# Settings
Project Cleaner window > Settings menu, or:

* **Project Settings > Plugins > Project Cleaner** - rules shared with the team (saved to `DefaultEditor.ini`)
  * Scan project plugins, ignored plugins
  * Primary assets are used, ignored primary asset types
  * Cinematic assets are used (Level Sequences, Template Sequences, Movie Render Queue/Graph configs), on by default
  * Editor utility assets are used, indirect (source code and config) assets are used
  * Always used classes
* **Editor Preferences > Plugins > Project Cleaner** - personal preferences
  * Toolbar button location (or hidden)
  * Scan on window open
  * Ask before fixing redirectors
  * Skip engine "About to load N assets" question while deleting (UE 5.4+)

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
| `-fix_redirectors` | fix redirectors first instead of stopping |
| `-list` | print found items, not only counts |

Exit code is `0` on success, `1` when project has redirectors or something failed to delete. UE4 uses `UE4Editor-Cmd.exe`.

# Development
Plugin is developed against every supported engine at once. `Scripts/pjc.py` (Python 3, no dependencies) generates a test host project per installed engine, links this repository into them and builds or tests all of them:
```
python Scripts/pjc.py engines              # list found engines
python Scripts/pjc.py build -j 4           # compile for every engine
python Scripts/pjc.py test -j 4            # automation tests + commandlet tests
python Scripts/pjc.py test -e 5.4,5.6      # selected engines, or ranges like 5.0-5.3
python Scripts/pjc.py cli -e 5.5 -- -scan_only -list
python Scripts/pjc.py package -e 5.5       # RunUAT BuildPlugin
python Scripts/pjc.py lint                 # include case (Linux/Mac) and clang-format checks
```
Engines are found automatically (Epic launcher, registry, `~/.config/Epic/UnrealEngine/Install.ini`, default install folders). Override with `PJC_ENGINES` (engine roots separated by `;` on Windows, `:` elsewhere) or `Scripts/pjc.local.json`:
```json
{ "engines": ["D:/Engines/UE_5.5"], "hosts_dir": "D:/ue_hosts" }
```
Host projects go to `PJC_HOSTS_DIR` or `<repo>/../hosts` by default. Automation tests live in `Source/Pjc/Private/Tests` (`ProjectCleaner.*`). Tests that create and delete content run only inside generated host projects.
