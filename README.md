# ProjectCleaner
Unreal engine plugin for managing unused assets and empty folders in your project.

No more need for Migration and other hacky methods.

Versions Supported: 4.27+   
Platform: Windows and Linux

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
2) Clone the repository to the 'Plugins' folder (make sure you pick the correct branch for your engine version).
3) Build the solution and you're done!

# Features
* Detection of unused assets
* Detection of corrupted assets
* Detection of indirect assets
* Detection of external files
* Empty folders detection
* Subsystem class that exposes most of the plugin functionality, which can be utilized in Blueprints or Python scripts.
* Command line interface using Commandlets
* Works fast on Large projects
* Intiutive and informative user interface
* Per-folder asset usage information
* Configurable scanning and exclusion settings
