// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

struct FAssetData;

/**
 * @brief This class responsible for filtering assets
 */
class AssetFilterManager
{
public:
	/**
	 * @brief Removes level assets from given container
	 * @param AssetContainer Container for game assets
	 */
	static void RemoveLevelAssets(TArray<FAssetData>& AssetContainer);

	/**
	 * @brief Check if given class is "level" or "level build data" asset
	 * @param ClassName Given class name to check
	 * @return bool
	 */
	static bool IsLevelAsset(const FName& ClassName);

	/**
	 * @brief Returns AssetContainer after applying difference operation with FilterSet
	 * @param AssetContainer 
	 * @param FilterSet 
	 */
	static void Difference(TArray<FAssetData>& AssetContainer, TSet<FName>& FilterSet);
};
