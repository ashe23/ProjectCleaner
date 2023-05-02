// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PjcTypes.h"
#include "AssetRegistry/AssetRegistryModule.h"

struct FPjcLibAsset
{
	static FAssetRegistryModule& GetAssetRegistry();
	static void GetAssetsInPath(const FString& InPath, const bool bRecursive, TArray<FAssetData>& OutAssets);
	static void GetAssetsIndirect(TArray<FAssetData>& OutAssets);
	static void GetAssetsIndirect(TMap<FAssetData, FPjcAssetIndirectUsageInfo>& AssetsIndirectInfos);

private:
	static void GetClassNamesPrimary(TSet<FName>& OutClassNames);
	static void GetClassNamesEditor(TSet<FName>& OutClassNames);
};
