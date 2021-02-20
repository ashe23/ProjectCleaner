# ProjectCleaner
Simple UE4 plugin for deleting all unused assets and empty folders in project.

No more Migration and other hacky methods needed.

Versions Supported: 4.23+  
Platform: Windows

# How its working?
It will scan your projects all assets that never used in any level.
So if you have any level asset that never used, make sure delete them first, then try to clean with plugin.
Any asset(and its dependencies) referenced by any level will remain untouched.

#### Filters
If you need to exclude some of directories from scanning, you can do it through dropdown menu and add directory pathes.
All assets in those directories and their related assets will remain untouched.

#### Assets that used directly in source code (Hardlinks)
Plugin also takes into account assets that used in source codes. So those asset will also remain untouched.
Example:
```cpp
static ConstructorHelpers::FObjectFinder<UMaterial> Material(TEXT("Material'/Game/NewMaterial.NewMaterial'"));
```

#### Empty Folders
Sometimes you can encounter a situation, where you will see empty folder in content browser, but engine wont let you delete it.
This is happening because folder contains non project(.uasset) files.
This can be frustrating :(
So be before any cleaning operation, Plugin will inform you about those files and the list will be shown in "Output Log", so later you can handle those files manually.

##### Caution 
In large projects it may take some time to locate and delete assets!
So if your engine freezes, dont worry its doing its work in background, just wait until it finishes.

# Installation
1) Create Plugins Folder in your project root directory
2) Download appropriate version of .zip in releases
3) Extract to Plugins folder
4) Open Project, and you are Done!

# Installation from source
1) Create Plugins Folder in your project root directory
2) Pick Master branch
3) Download .zip file and extract to Plugins folder
4) Open your unreal project, it will ask to rebuild project
5) Wait until building succeeded and you are Done!
