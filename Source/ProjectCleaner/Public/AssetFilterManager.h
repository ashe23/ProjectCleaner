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
};
