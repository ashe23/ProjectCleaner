// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FIndirectAsset;
struct FExcludedAsset;
class UCleanerConfigs;
class UExcludeOptions;

enum class EProjectCleanerManagerState
{
	Initial,
	Cached,
	FullScan
};

enum class EProjectCleanerManagerCacheState
{
	None,
	Cached,
	RescanRequired
};

class ProjectCleanerManager
{
public:
	ProjectCleanerManager();

	/**
	 * @brief Updates data containers
	 */
	void Update();
	void AddToUserExcludedAssets(const TArray<FAssetData>& NewUserExcludedAssets);
	void AddToExcludeClasses(const TArray<FAssetData>& ExcludedByTypeAssets);
	void RemoveFromExcludedAssets(const TArray<FAssetData>& Assets);

	/** Data Container getters **/
	const TArray<FAssetData>& GetAllAssets() const;
	const TSet<FName>& GetCorruptedAssets() const;
	const TSet<FName>& GetNonEngineFiles() const;
	const TMap<FAssetData, FIndirectAsset>& GetIndirectAssets() const;
	const TSet<FName>& GetEmptyFolders() const;
	const TSet<FName>& GetPrimaryAssetClasses() const;
	const TArray<FAssetData>& GetUnusedAssets() const;
	const TSet<FName>& GetExcludedAssets() const;
	UCleanerConfigs* GetCleanerConfigs() const;
	UExcludeOptions* GetExcludeOptions() const;
	float GetUnusedAssetsPercent() const;
private:
	
	void LoadInitialData();
	void FindUnusedAssets();
	bool IsExcludedByClass(const FAssetData& AssetData) const;

	/** Data Containers **/
	
	/**
	* @brief All assets in "/Game" folder
	*/
	TArray<FAssetData> AllAssets;

	/**
	* @brief Corrupted assets (assets with engine extension, but not available in AssetRegistry)
	*/
	TSet<FName> CorruptedAssets;

	/**
	* @brief All non engine files (not .uasset or .umap files)
	*/
	TSet<FName> NonEngineFiles;

	/**
	 * @brief Indirectly used assets container
	 *		Key - is Asset PackageName
	 *		Value - FIndirectAsset - File path where its used
	 *							   - Line number
	 * 
	 */
	TMap<FAssetData, struct FIndirectAsset> IndirectAssets;

	/**
	 * @brief All empty folders in "Game" folder
	 */
	TSet<FName> EmptyFolders;

	/**
	 * @brief All Primary asset classes in project (Level assets are primary by default)
	 */
	TSet<FName> PrimaryAssetClasses;

	/**
	 * @brief  Assets that user manually excluded from UnusedAssetTab
	 */
	TArray<FAssetData> UserExcludedAssets;

	/**
	* @brief Excluded assets and their linked assets map
	* @example
	*		Lets say we have 3 assets BP, Static_Mesh and Material. BP contains Static_Mesh and Static Mesh uses Material
	*		Hierarchy BP --> Static_Mesh --> Material
	*		Now if excluded asset is StaticMesh, then linked assets are BP and Material
	*		ExcludedAsset : StaticMesh
	*		LinkedAssets : BP, Material
	* @reason This preventing breaking links between assets
	*/
	TSet<FName> ExcludedAssets;

	/**
	 * @brief All assets that have external referencers (outside "/Game" folder)
	 */
	TArray<FAssetData> AssetsWithExternalRefs;

	/**
	 * @brief All unused assets in "/Game" folder
	 */
	TArray<FAssetData> UnusedAssets;

	class UCleanerConfigs* CleanerConfigs;
	class UExcludeOptions* ExcludeOptions;

	/* Asset Registry */
	class FAssetRegistryModule* AssetRegistry;
	void OnAssetAdded(const FAssetData& AssetData);
	void OnAssetRemoved(const FAssetData& AssetData);
	void OnAssetUpdated(const FAssetData& AssetData);
	void OnAssetRenamed(const FAssetData& AssetData, const FString& Name);

	/* Directory Watcher Module */
	class FDirectoryWatcherModule* DirectoryWatcher;
	void OnDirectoryChanged(const TArray<struct FFileChangeData>& InFileChanges);

	bool bIsActualData = false;
	bool bDelegatesRegistered = false;
};
