// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "StructsContainer.h"
#include "CoreMinimal.h"

struct FAssetData;

/**
 * @brief Responsible for gathering and analyzing data
 */
class ProjectCleanerDataManager
{
	friend class FProjectCleanerDataManagerTests;
public:
	ProjectCleanerDataManager();
	
	void Empty();
	void Update();
	void SetRootFolder(const FString& Folder);

	void FindPrimaryAssetClasses();
	void FindUsedAssets();
	void FindUnusedAssets();
	void FindIndirectlyUsedAssets();
	void AnalyzeProjectDirectory();
	void FindLinkedAssets(TSet<FName>& LinkedAssets);

	static FString GameUserDeveloperDir;
	static FString GameDevelopersDir;
	static FString GameUserDeveloperCollectionsDir;
	static FString CollectionsDir;
	static FString ProjectContentDir;
private:
	/**
	* @brief All assets in AssetRegistry
	*/
	TArray<FAssetData> AllAssets;
	
	/**
	* @brief All used assets
	*/
	TSet<FName> UsedAssets;
	
	/**
	* @brief All Unused assets
	*/
	TArray<FAssetData> UnusedAssets;

	/**
	* @brief Contains all files from "Game" folder
	*/
	TSet<FName> AllFiles;
	
	/**
	* @brief All non engine files (not .uasset or .umap files)
	*/
	TSet<FName> NonEngineFiles;
	
	/**
	* @brief Corrupted Assets (Assets with engine extension, but not available in AssetRegistry)
	*/
	TSet<FName> CorruptedAssets;
	
	/**
	* @brief All empty folders in "Game" folder
	*/
	TSet<FName> EmptyFolders;
	
	/**
	* @brief All assets that used indirectly (in source or config files)
	*/
	TMap<FAssetData, FIndirectAsset> IndirectlyUsedAssets;

	/**
	* @brief All Primary asset classes in project (Level assets are primary by default)
	*/
	TSet<FName> PrimaryAssetClasses;
private:
	class FAssetRegistryModule* AssetRegistry;
	class FDirectoryWatcherModule* DirectoryWatcher;
	class UCleanerConfigs* CleanerConfigs;
	class UExcludeOptions* ExcludeOptions;
	class UContentBrowserSettings* ContentBrowserSettings;

	FName AbsoluteRootFolder;
	FName RelativeRootFolder;
};