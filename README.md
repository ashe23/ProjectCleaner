# ProjectCleaner
Simple UE4 plugin for deleting all unused assets in project.

No more Migration and other hacky methods needed.

Versions Supported: 4.23+  
Platform: Windows

# How its working?
It will scan your projects all assets that never used in any level.
So if you have any level asset that never used, make sure delete them first, then try to clean with plugin.
Any asset(and its dependencies) referenced by any level will remain untouched.

##### Caution: In large projects it may take some time to locate and delete assets!


# Installation
1) Create Plugins Folder in your project root directory
2) Pick appropriate version in branches dropdown
3) Download .zip file and extract to Plugins folder
4) Open Project, and you are Done!

# Installation from source
1) Create Plugins Folder in your project root directory
2) Pick Master branch
3) Download .zip file and extract to Plugins folder
4) Open your unreal project, it will ask to rebuild project
5) Wait until building succeeded and you are Done!
