// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FIndirectAsset;
struct FExcludedAsset;
class UCleanerConfigs;
class UExcludeOptions;

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

	/** Data Container getters **/
	const TArray<FAssetData>& GetAllAssets() const;
	const TSet<FName>& GetCorruptedAssets() const;
	const TSet<FName>& GetNonEngineFiles() const;
	const TMap<FAssetData, FIndirectAsset>& GetIndirectAssets() const;
	const TSet<FName>& GetEmptyFolders() const;
	const TSet<FName>& GetPrimaryAssetClasses() const;
	const TArray<FAssetData>& GetUnusedAssets() const;
	const TMap<FName, FExcludedAsset>& GetExcludedAssets() const;
	UCleanerConfigs* GetCleanerConfigs() const;
	UExcludeOptions* GetExcludeOptions() const;
private:
	
	/**
	 * @brief Empties all data containers
	 */
	void Clean();

	void FindUnusedAssets();
	void FindExcludedAssets();
	bool IsPrimaryAsset(const FAssetData& AssetData) const;
	bool IsExcludedByClass(const FAssetData& AssetData) const;	
	bool IsUnderDevelopersFolder(const FAssetData& AssetData) const;
	bool IsUnderMegascansFolder(const FAssetData& AssetData) const;

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
	TMap<FName, FExcludedAsset> ExcludedAssets;

	/**
	 * @brief All assets that have external referencers (outside "/Game" folder)
	 */
	TArray<FAssetData> AssetsWithExternalRefs;

	/**
	 * @brief All unused assets in "/Game" folder
	 */
	TArray<FAssetData> UnusedAssets;

	TMap<FName, struct FAssetCleanerInfo> AssetsInfoMap;

	class UCleanerConfigs* CleanerConfigs;
	class UExcludeOptions* ExcludeOptions;
	class FAssetRegistryModule* AssetRegistry;
};
