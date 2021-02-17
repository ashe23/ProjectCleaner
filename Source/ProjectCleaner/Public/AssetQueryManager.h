// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

struct FAssetData;

/**
 * @brief This class responsible for querying assets from AssetRegistry
 */
class AssetQueryManager
{
public:	
	/**
	* @brief Finds all assets in project
	* @param AssetContainer Container for all game assets
	*/
	static void GetAllAssets(TArray<FAssetData>& AssetContainer);

	/**
	 * @brief Finds assets with given parameters
	 * @param AssetContainer Container for game assets
	 * @param Path SearchPath
	 * @param bRecursive Search recursively or not
	 */
	static void GetAssets(TArray<FAssetData>& AssetContainer, const FName& Path, bool bRecursive);
	
	/**
	 * @brief Finds all "Level" assets in project
	 * @param AssetContainer Container for game assets
	 */
	static void GetLevelAssets(TArray<FAssetData>& AssetContainer);

	/**
	 * @brief Finds all dependencies for given array of assets
	 * @param AssetContainer Container for game assets
	 * @param DependencyContainer Dependency container for assets
	 */
	static void GetDependencies(TArray<FAssetData>& AssetContainer, TArray<FAssetData>& DependencyContainer);

	/**
	 * @brief Returns asset data for given asset name
	 * @param AssetName 
	 * @param AssetContainer Container of assets
	 * @return FAssetData|nullptr
	 */
	static FAssetData* GetAssetData(const FName& AssetName, TArray<FAssetData>& AssetContainer);
};
