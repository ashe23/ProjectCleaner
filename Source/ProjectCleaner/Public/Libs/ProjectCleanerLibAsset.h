// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectCleanerTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ProjectCleanerLibAsset.generated.h"

UCLASS()
class UProjectCleanerLibAsset final : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static void ProjectScan(const FProjectCleanerScanSettings& ScanSettings, FProjectCleanerScanResult& ScanResult);
	static void ProjectClean();
	static void ProjectCleanEmptyFolders();
	static void FixupRedirectors();
private:
	static void GetAssetsAll(TArray<FAssetData>& AssetsAll);
	static void GetAssetsPrimary(TArray<FAssetData>& AssetsPrimary);
	static void GetAssetsIndirect(TArray<FAssetData>& AssetsIndirect);
	static void GetAssetsIndirectWithInfo(TArray<FProjectCleanerIndirectAsset>& AssetsIndirect);
	static void GetAssetsExcluded(TArray<FAssetData>& AssetsExcluded);
	static void GetAssetsWithExternalRefs(TArray<FAssetData>& AssetsWithExternalRefs);
	static void GetAssetsBlacklisted(TArray<FAssetData>& AssetsBlacklisted);
	static void GetAssetsLinked(const TArray<FAssetData>& InAssets, TArray<FAssetData>& OutLinkedAssets);
	static void GetAssetsUsed(TArray<FAssetData>& AssetsUsed);
	static void GetAssetsUnused(TArray<FAssetData>& AssetsUnused);
	static FString GetAssetClassName(const FAssetData& AssetData);
	static int64 GetAssetsTotalSize(const TArray<FAssetData>& Assets);
	static void GetFoldersBlacklisted(TSet<FString>& FoldersBlacklisted);
};
