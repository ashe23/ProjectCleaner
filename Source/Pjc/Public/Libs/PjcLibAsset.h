// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PjcTypes.h"

struct FPjcLibAsset
{
	static bool ProjectContainsRedirectors();
	static bool AssetIsBlueprint(const FAssetData& InAssetData);
	static bool AssetIsExtReferenced(const FAssetData& InAssetData);
	static bool AssetIsMegascansBase(const FAssetData& InAssetData);
	static bool AssetClassNameInList(const FAssetData& InAssetData, const TSet<FName>& InClassNames);
	static bool AssetRegistryWorking();
	static bool IsValidClassName(const FName& InClassName);
	static void FixupRedirectorsInProject(const bool bSlowTaskEnabled);
	static void AssetRegistryUpdate();
	static void GetClassNamesPrimary(TSet<FName>& ClassNamesPrimary);
	static void GetClassNamesEditor(TSet<FName>& ClassNamesEditor);
	static void GetAssetsAll(TArray<FAssetData>& OutAssets);
	static void GetAssetsPrimary(const TArray<FAssetData>& AssetsAll, TArray<FAssetData>& OutAssets);
	static void GetAssetsEditor(const TArray<FAssetData>& AssetsAll, TArray<FAssetData>& OutAssets);
	static void GetAssetsExtReferenced(const TArray<FAssetData>& AssetsAll, TArray<FAssetData>& OutAssets);
	static void GetAssetsExcluded(const FPjcExcludeSettings& InExcludeSettings, TArray<FAssetData>& OutAssets);
	static void GetAssetsByPath(const FString& InPath, const bool bRecursive, TArray<FAssetData>& OutAssets);
	static void GetAssetsByPackagePaths(const TArray<FString>& InPackagePaths, const bool bRecursive, TSet<FAssetData>& OutAssets);
	static void GetAssetsByObjectPaths(const TArray<FName>& InObjectPaths, TSet<FAssetData>& OutAssets);
	static void GetAssetsIndirect(TArray<FAssetData>& AssetsIndirect, TArray<FPjcAssetIndirectInfo>& AssetsIndirectInfos);
	static void GetAssetDeps(const FAssetData& InAssetData, TSet<FAssetData>& OutDeps);
	static void GetAssetsDeps(const TSet<FAssetData>& Assets, TSet<FAssetData>& Dependencies);
	static void GetCachedPaths(TArray<FString>& Paths);
	static FName GetAssetClassName(const FAssetData& InAssetData);
	static FAssetData GetAssetByObjectPath(const FName& InObjectPath);
	static int64 GetAssetSize(const FAssetData& InAssetData);
	static int64 GetAssetsSize(const TArray<FAssetData>& InAssetDatas);
};
