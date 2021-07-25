// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FIndirectAsset;

class ProjectCleanerManager
{
public:
	ProjectCleanerManager();

	/**
	 * @brief Updates data containers
	 */
	void Update();

	/** Data Container getters **/
	const TArray<FAssetData>& GetAllAssets() const;
	const TSet<FName>& GetCorruptedAssets() const;
	const TSet<FName>& GetNonEngineFiles() const;
	const TMap<FName, FIndirectAsset>& GetIndirectAssets() const;
	const TSet<FName>& GetEmptyFolders() const;
	const TSet<FName>& GetPrimaryAssetClasses() const;
private:
	
	/**
	 * @brief Empties all data containers
	 */
	void Clean();

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
	TMap<FName, struct FIndirectAsset> IndirectAssets;

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
	 * @brief All excluded assets (included by path, by class and user excluded)
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
	TArray<FAssetData> LinkedAssets;

	class UCleanerConfigs* CleanerConfigs;
	class UExcludeOptions* ExcludeOptions;
	class FAssetRegistryModule* AssetRegistry;
};
