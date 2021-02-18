// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

struct FAssetData;
struct FNode;
class UDirectoryFilterSettings;


/**
 * @brief This class responsible for querying assets from AssetRegistry
 */
class AssetQueryManager
{
public:
	/**
	 * @brief Returns all unused assets in project
	 * @param AssetContainer Container for all game assets
	 * @param DirectoryFilterSettings Directories never scan
	 * @param AdjacencyList Adjacency list for assets
	 */
	static void GetUnusedAssets(TArray<FAssetData>& AssetContainer,
	                            UDirectoryFilterSettings* DirectoryFilterSettings,
	                            TArray<FNode>& AdjacencyList);
	/**
	 * @brief Returns total size of given assets (in bytes)
	 * @param AssetContainer Container for all game assets
	 * @return 
	 */
	static int64 GetTotalSize(const TArray<FAssetData>& AssetContainer);

	/**
	 * Root Assets are those who has not any referencers on it.
	 * @brief Returns root assets from asset container
	 * @param RootAssets
	 * @param AssetContainer 
	 */
	static void GetRootAssets(TArray<FAssetData>& RootAssets, TArray<FAssetData>& AssetContainer);

	/**
	* @brief Finds all related assets using DFS for given single asset
	* @param Node - given asset
	* @param RelatedAssets - Assets container
	* @param AdjacencyList - Where to search assets
	*/
	static void FindAllRelatedAssets(const FNode& Node,
	                                 TArray<FName>& RelatedAssets,
	                                 const TArray<FNode> AdjacencyList);
	/**
	* @brief Finds all assets in project
	* @param AssetContainer Container for all game assets
	*/
	static void GetAllAssets(TArray<FAssetData>& AssetContainer);

	/**
	 * @brief Finds all assets that used in any "Level" asset
	 * @param DependencyContainer Dependency container for assets
	 */
	static void GetAllAssetNamesThatUsedInLevel(TSet<FName>& DependencyContainer);

	/**
	 * @brief Returns asset data for given asset name
	 * @param AssetName 
	 * @param AssetContainer Container of assets
	 * @return FAssetData|nullptr
	 */
	static FAssetData* GetAssetData(const FName& AssetName, TArray<FAssetData>& AssetContainer);

	/**
	 * @brief If user entered any exclude directory, then we should exclude given directories from scanning
	 * @param DirectoryFilterSettings 
	 * @return bool
	 */
	static bool ShouldApplyDirectoryFilters(UDirectoryFilterSettings* DirectoryFilterSettings);

	/**
	 * How This function works:
	 * 1) It Creates Adjacency List for all assets in AssetsContainer todo:ashe23 update later
	 * 2) Finds all assets in given directory filter paths("Exclude This Directories" from UI)
	 * 3) Using DFS algorithm finds all related assets for founded assets in Step #2
	 * 4) And finally removes them from AssetContainer
	 *
	 * This is needed, because asset dependencies can be in different folders and if even one asset in dependency chain
	 * is in filter directory path, we should exclude all assets in chain.
	 *
	 * Example:
	 *	Lets Say you have this folders in project.
	 *		Blueprint
	 *			- BP_SomeActor
	 *		Materials
	 *			- M_SomeActorMaterial
	 *		Textures
	 *			- T_SomeActorTexture
	 *	And BP_SomeActor blueprint using M_SomeActorMaterial in it. M_SomeActorMaterial using T_SomeActorTexture.
	 *	
	 *	And lets say user excluded "Blueprint" folder from scanning.
	 *	So this function will find BP_SomeActor blueprint then it will find M_SomeActorMaterial and T_SomeActorTexture,
	 *	and it will remove all of them from AssetContainer
	 *
	 *	
	 * @brief Deletes all assets from AssetContainer that user picked from "Exclude This Directories" filter 
	 * @param AssetContainer 
	 * @param DirectoryFilterSettings
	 * @param AdjacencyList Adjacency list for assets
	 */
	static void ApplyDirectoryFilters(TArray<FAssetData>& AssetContainer,
	                                  UDirectoryFilterSettings* DirectoryFilterSettings,
	                                  TArray<FNode>& AdjacencyList);

	/**
	 * @brief Create adjacency list for given assets
	 * @param AssetContainer 
	 * @param AdjacencyList 
	 */
	static void CreateAdjacencyList(TArray<FAssetData>& AssetContainer, TArray<FNode>& AdjacencyList);
};
