// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "StructsContainer.h"
#include "CoreMinimal.h"


// Tracks for any changes in directory and asset registry
// Calls DataManager function to monitor any changes
class ProjectCleanerManager
{
public:
	ProjectCleanerManager();
	void UpdateData();

	FProjectCleanerData& GetCleanerData();
	UCleanerConfigs* GetCleanerConfigs() const;
	UExcludeOptions* GetExcludeOptions() const;
	static FName GameUserDeveloperDir;
	static FName GameDevelopersDir;
	static FName GameUserDeveloperCollectionsDir;
	static FName CollectionsDir;
private:
	FProjectCleanerData CleanerData;
	UCleanerConfigs* CleanerConfigs;
	UExcludeOptions* ExcludeOptions;
	
	/* Data functions */
	void LoadInitialData();
	void FindEmptyFoldersAndNonEngineFiles();
	void FindPrimaryAssetsAndItsDependencies();
	void FindSourceAndConfigFiles();
	void FindCorruptedAssets();
	void FindLinkedAssets(TSet<FName>& LinkedAssets);
	void GetAllAssets();
	void GetUnusedAssets();
	void GetPrimaryAssetClasses();

	bool IsExcludedByPath(const FAssetData& AssetData);
	bool IsExcludedByClass(const FAssetData& AssetData);

	bool bInitialLoading = true;
	bool bScanCancelledByUser = false;
	
	/* AssetRegistry */
	class FAssetRegistryModule* AssetRegistry;
	void RegisterDelegates();
	
	/* DirectoryWatcher */
	class FDirectoryWatcherModule* DirectoryWatcher;
};
