// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "StructsContainer.h"
#include "CoreMinimal.h"

struct FAssetData;

class ProjectCleanerDataManagerV2
{
public:
	
	/**
	 * @brief Returns all assets in given path using AssetRegistry
	 * @param InPath - Relative path. Example "/Game"
	 * @param AllAssets - Asset Container
	 */
	static void GetAllAssetsByPath(const FName& InPath, TArray<FAssetData>& AllAssets);

	/**
	 * @brief Returns all corrupted assets and non engine files in given path
	 * @param InPath - Absolute path. Example "C:/dev/projects/project_name/content/"
	 * @param AllAssets - All assets in path
	 * @param CorruptedAssets - Corrupted assets container
	 * @param NonEngineFiles - Non engine files container
	 */
	static void GetInvalidFilesByPath(
		const FString& InPath,
		const TArray<FAssetData>& AllAssets,
		TSet<FName>& CorruptedAssets,
		TSet<FName>& NonEngineFiles
	);

	/**
	 * @brief Returns all asset package names that used in source code and config files, under Source/, Config/ or Plugins/ Directories
	 * @param InPath - Absolute path. Example "C:/dev/projects/project_name/content/"
	 * @param IndirectlyUsedAssets - Indirect assets container
	 */
	static void GetIndirectAssetsByPath(const FString& InPath, TMap<FName, FIndirectAsset>& IndirectlyUsedAssets);

	/**
	 * @brief Returns all empty folders in given path
	 * @param InPath - Absolute path. Example "C:/dev/projects/project_name/content/"
	 * @param EmptyFolders - Empty Folders container
	 * @param bScanDevelopersContent - Flag should we include Developer Content or not
	 */
	static void GetEmptyFolders(const FString& InPath, TSet<FName>& EmptyFolders, const bool bScanDevelopersContent);

	/**
	 * @brief Returns all primary asset classes in project
	 * @param PrimaryAssetClasses - Primary assets classes container
	 */
	static void GetPrimaryAssetClasses(TSet<FName>& PrimaryAssetClasses);

	/**
	 * @brief Checks if given package is under exclude directories or not
	 * @param PackagePath - Asset package path ("/Game/Folder/") 
	 * @param ExcludeOptions - Exclude options ptr 
	 * @return bool
	 */
	static bool ExcludedByPath(const FName& PackagePath, const UExcludeOptions* ExcludeOptions);

	static bool ExcludedByClass(const FAssetData& AssetData, const UExcludeOptions* ExcludeOptions);

private:
	/**
	 * @brief Finds all empty folders in given path recursively
	 * @param FolderPath - Absolute path
	 * @param EmptyFolders - Empty Folders container
	 * @return bool
	 */
	static bool FindEmptyFolders(const FString& FolderPath, TSet<FName>& EmptyFolders);
};


/**
 * @brief Responsible for gathering and analyzing data in project
 *			* Unused assets
 *			* Empty folders
 *			* Non engine files
 *			* Corrupted assets
 *			etc.
 */
class ProjectCleanerDataManager
{
public:
	ProjectCleanerDataManager();
	
	void Update();
	
	// getters
	int64 GetTotalProjectSize() const;
	int64 GetTotalUnusedAssetsSize() const;
	const TArray<FAssetData>& GetAllAssets() const;
	const TSet<FName>& GetUsedAssets() const;
	const TArray<FAssetData>& GetUnusedAssets() const;
	const TSet<FName>& GetAllFiles() const;
	const TSet<FName>& GetNonEngineFiles() const;
	const TSet<FName>& GetCorruptedAssets() const;
	const TSet<FName>& GetEmptyFolders() const;
	const TSet<FName>& GetPrimaryAssetClasses() const;
	const TMap<FAssetData, FIndirectAsset>& GetIndirectlyUsedAssets() const;
	const TArray<FAssetData>& GetUserExcludedAssets() const;
	const TArray<FAssetData>& GetExcludedAssets() const;
	const TArray<FAssetData>& GetLinkedAssets() const;
	const TSet<FName>& GetScanDirectories() const;
	UCleanerConfigs* GetCleanerConfigs() const;
	UExcludeOptions* GetExcludeOptions() const;
	FString GetAbsoluteContentDir() const;
	FName GetRelativeContentDir() const;



private:
	void Empty();
	void FindPrimaryAssetClasses();
	void FindUsedAssets();
	void FindUnusedAssets();
	void FindIndirectlyUsedAssets();
	void FindInvalidFiles();
	void RemoveForbiddenFolders();
	void FindLinkedAssets(const TSet<FName>& FilteredAssets, TSet<FName>& LinkedAssets);
	bool FindEmptyFolders(const FString& FolderPath);

	bool HasExternalReferencers(const FName& PackageName);
	bool ExcludedByPath(const FName& PackagePath);
	bool ExcludedByClass(const FAssetData& AssetData);
	
	/**
	 * @brief Project Size (Assets in Content folder)
	 */
	int64 TotalProjectSize;
	
	/**
	 * @brief Unused assets size
	 */
	int64 TotalUnusedAssetsSize;
	
	/**
	* @brief All assets in AssetRegistry
	*/
	TArray<FAssetData> AllAssets;
	
	/**
	* @brief All used assets (PackageNames)
	*/
	TSet<FName> UsedAssets;
	
	/**
	* @brief All Unused assets
	*/
	TArray<FAssetData> UnusedAssets;

	/**
	* @brief Contains all files from "Game" folder, including non engine files
	*/
	TSet<FName> AllFiles; // todo:ashe23 remove candidate, unused mostly
	
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

	// todo:ashe23
	TSet<FName> PossiblyIndirectAssets;
	
	/**
	* @brief All assets that used indirectly (in source or config files)
	*/
	TMap<FAssetData, FIndirectAsset> IndirectlyUsedAssets;

	/**
	* @brief All Primary asset classes in project (Level assets are primary by default)
	*/
	TSet<FName> PrimaryAssetClasses;

	/**
	 * @brief Assets that user excluded from UnusedAssetTab
	 */
	TArray<FAssetData> UserExcludedAssets;
	
	/**
	 * @brief All excluded assets (include by path, by class and user excluded)
	 */
	TArray<FAssetData> ExcludedAssets;
	
	/**
	 * @brief All linked assets of excluded assets (Dependencies and Referencers)
	 * @note This keeps assets that are not directly excluded, but connected one of excluded assets
	 * @example
	 *		Lets say we have 3 assets BP, Static_Mesh and Material. BP contains Static_Mesh and Static Mesh uses Material
	 *		Hierarchy BP --> Static_Mesh --> Material
	 *		Now if excluded asset is StaticMesh, then linked assets are BP and Material
	 *		ExcludedAssets : StaticMesh
	 *		LinkedAssets : BP, Material
	 * @reason This preventing breaking links between assets
	 */
	TArray<FAssetData> LinkedAssets1;

	class FAssetRegistryModule* AssetRegistry;
	class FDirectoryWatcherModule* DirectoryWatcher;
	class UCleanerConfigs* CleanerConfigs;
	class UExcludeOptions* ExcludeOptions;
	class UContentBrowserSettings* ContentBrowserSettings;

	FString ContentDir_Absolute;
	FName ContentDir_Relative;
private:
	// This part is for automation testing,
	// because scanning based on project content path,
	// in tests we want create separate folder which wont conflict with already existing files and assets
	// so we making that class as friend and below function need only there
	friend class FProjectCleanerDataManagerEmptyFoldersTests;
	void EnableTestMode();
	
	FString SourceDir;
	FString ConfigDir;
	FString PluginsDir;
};