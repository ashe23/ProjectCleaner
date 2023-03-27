// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PjcTypes.h"

struct FPjcLibAsset
{
	static bool ProjectContainsRedirectors();
	static bool AssetIsBlueprint(const FAssetData& InAssetData);
	static bool AssetRegistryWorking();
	static void FixupRedirectorsInProject(const bool bSlowTaskEnabled);
	static void GetClassNamesPrimary(TSet<FName>& ClassNamesPrimary);
	static void GetClassNamesEditor(TSet<FName>& ClassNamesEditor);
	static void GetAssetsIndirect(TMap<FAssetData, TArray<FPjcAssetUsageInfo>>& AssetsIndirect);
	static FName GetAssetClassName(const FAssetData& InAssetData);
	static FAssetData GetAssetByObjectPath(const FName& InObjectPath);
	static int64 GetAssetSize(const FAssetData& InAssetData);
	static int64 GetAssetsSize(const TArray<FAssetData>& InAssetDatas);
};
