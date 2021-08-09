// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FIndirectAsset;
struct FExcludedAsset;
class UCleanerConfigs;
class UExcludeOptions;
class FAssetRegistryModule;


DECLARE_DELEGATE(FOnCleanerManagerUpdated);

class ProjectCleanerManager
{
public:
	ProjectCleanerManager();
	~ProjectCleanerManager();

	/**
	 * @brief Updates data containers
	 */
	void Update();
	void ExcludeSelectedAssets(const TArray<FAssetData>& NewUserExcludedAssets);
	void ExcludeSelectedAssetsByType(const TArray<FAssetData>& ExcludedByTypeAssets);
	void ExcludePath(const FString& InPath);
	void IncludePath(const FString& InPath);
	void IncludeSelectedAssets(const TArray<FAssetData>& Assets);
	void IncludeAllAssets();
	void DeleteSelectedAssets(const TArray<FAssetData>& Assets);
	void DeleteAllUnusedAssets();
	void DeleteAllEmptyFolders();

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
	const FAssetRegistryModule* GetAssetRegistry() const;

	/**
	 * @brief Delegate, called when data updates
	 */
	FOnCleanerManagerUpdated OnCleanerManagerUpdated;
private:
	
	void LoadData();
	void FindUnusedAssets();
	void FindUsedAssets(TSet<FName>& UsedAssets);
	void FindUsedAssetsDependencies(const TSet<FName>& UsedAssets, TSet<FName>& UsedAssetsDeps) const;
	bool IsExcludedByClass(const FAssetData& AssetData) const;
	bool IsExcludedByPath(const FAssetData& AssetData) const;
	void GetDeletionChunk(TArray<FAssetData>& Chunk);
	FText GetDeletionProgressText(const int32 DeletedAssetNum, const int32 Total) const;

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
	 * @brief All primary assets in project
	 */
	TArray<FAssetData> PrimaryAssets;
	
	/**
	 * @brief  Assets that user manually excluded from UnusedAssetTab
	 */
	TArray<FAssetData> UserExcludedAssets;

	/**
	* @brief Excluded assets
	* 
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
	FAssetRegistryModule* AssetRegistry;
};
