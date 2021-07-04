# ProjectCleaner
Simple UE4 plugin for deleting all unused assets and empty folders in project.

No more Migration and other hacky methods needed.

Versions Supported: 4.23+  
Platform: Windows

<img src="https://user-images.githubusercontent.com/8270558/124382914-d9ccf000-dcda-11eb-9256-441ac8eceb4b.png" />

  
[comment]: <img src="https://user-images.githubusercontent.com/8270558/124383180-07feff80-dcdc-11eb-90b7-ffbf4a770cbf.png" width="600" height="320" />

# Installation
1) Create Plugins Folder in your project root directory
2) Download appropriate version of .zip in releases
3) Extract to Plugins folder
4) Open Project, and you are Done!



# How its working?

* Unused assets detection
* Empty folders detection
* Invalid assets detection
* Assets used indirectly
* Exclude options
* Deletion chunk size configuration

![ProjectCleanerPlugin-Query](https://user-images.githubusercontent.com/8270558/124382551-18fa4180-dcd9-11eb-99b6-6916579f9d18.png)

### Unused Assets Detection
Unused assets are those, which are not primary assets and not in their dependencies. So any assset that not used by primary asset is marked as unused.
Level assets are primary by default in engine.

### Empty Folders
Finds all empty folders, also takes into account nested folders.
Sometimes you can encounter a situation, where you will see empty folder in content browser, but engine wont let you delete it.
This is happening because folder contains non engine (.umap or .uasset) files.
This can be frustrating :( <br>
So those files are shown seperately in Non Engine Files tab.

### Invalid assets detection
These are Non engine(not .umap or .uasset) and corrupted files.
Corrupted are those which exists in project folder, but for some reason doesnt exist in AssetRegistry and therefore not showing in ContentBrowser.
All those files will be detected and shown in their tab.

### Asset used indirectly
This are assets that used in source code files or config files.<br>
Example:
```cpp
static ConstructorHelpers::FObjectFinder<UMaterial> Material(TEXT("Material'/Game/NewMaterial.NewMaterial'"));
```
If you try to delete those assets , engine will not inform that those assets are in use.<br>
Those assets have seperate tab, where it show Asset -> In which File its used -> And on which line

![p2](https://user-images.githubusercontent.com/8270558/124383870-c1ab9f80-dcdf-11eb-9035-2daba351f9eb.png)

### Exclude Options
If you need to exclude some directories or assets from scanning, you can do it through filters.

![p6](https://user-images.githubusercontent.com/8270558/124384045-b1e08b00-dce0-11eb-80ac-d802d322b55b.png)

Also if you excluding assets, its related assets also will be excluded to prevent any link corruption.<br>
Example:<br>
Lets say we got blueprint that uses static mesh and material
![2021-07-04_16-06-49](https://user-images.githubusercontent.com/8270558/124384391-020c1d00-dce2-11eb-8166-065828391079.png)

So if we exclude static mesh or material, all 3 assets will be excluded.

![2021-07-04_16-09-39](https://user-images.githubusercontent.com/8270558/124384436-48617c00-dce2-11eb-9918-5a74a052353a.png)

### Deletion chunk configuration
If project have a lots of assets, it could freeze engine if we try to delete all at once.
To prevent this you can specify deletion chunk maximum limit. Default is 20. So plugin deletes assets in chunks and progress will be shown in right bottom corner.
### Caution 
In large projects it may take some time to locate and delete assets!
So if your engine freezes, dont worry its doing its work in background, just wait until it finished.
