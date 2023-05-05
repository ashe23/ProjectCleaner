// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PjcTypes.h"
#include "AssetRegistry/AssetRegistryModule.h"

struct FPjcLibAsset
{
	static FAssetRegistryModule& GetAssetRegistry();
	static void GetAssetsInPath(const FString& InPath, const bool bRecursive, TArray<FAssetData>& OutAssets);
	static void GetAssetsInPaths(const TArray<FString>& InPaths, const bool bRecursive, TArray<FAssetData>& OutAssets);
	static void GetAssetsByObjectPaths(const TArray<FString>& InObjectPaths, TArray<FAssetData>& OutAssets);
	static void GetAssetsIndirect(TArray<FAssetData>& OutAssets);
	static void GetAssetsIndirect(TMap<FAssetData, FPjcAssetIndirectUsageInfo>& AssetsIndirectInfos);
	static void GetAssetsExcludedByPaths(TSet<FAssetData>& OutAssets);
	static void GetAssetsDeps(const TSet<FAssetData>& Assets, TSet<FAssetData>& Dependencies);
	static void LoadAssetsDependencies(TSet<FAssetData>& InAssets);
	static void FilterAssetsByPath(const TArray<FAssetData>& InAssets, const FString& InPath, TArray<FAssetData>& OutAssets);
	static int64 GetAssetsTotalSize(const TArray<FAssetData>& InAssets);
	static void GetClassNamesPrimary(TSet<FName>& OutClassNames);
	static void GetClassNamesEditor(TSet<FName>& OutClassNames);
	static void GetClassNamesExcluded(TSet<FName>& OutClassNames);
	static bool AssetIsBlueprint(const FAssetData& InAssetData);
	static bool AssetIsExtReferenced(const FAssetData& InAssetData);
	static FName GetAssetExactClassName(const FAssetData& InAssetData);
};
