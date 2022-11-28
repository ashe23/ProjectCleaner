// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UProjectCleanerScanSettings;
struct FProjectCleanerIndirectAsset;

struct FProjectCleanerScanner
{
	void Scan(const TWeakObjectPtr<UProjectCleanerScanSettings>& InScanSettings);
	void Reset();
private:
	void FindAssetsPrimary();
	void FindAssetsIndirect();
	void FindAssetsWithExternalRefs();
	void FindAssetsBlackListed();
	void FindAssetsExcluded();
	void FindAssetsUsed();
	void FindAssetsUnused();
	bool HasIndirectAsset(const FString& FileContent) const;
	
	TSet<FString> FilesAll;
	TSet<FString> FilesCorrupted;
	TSet<FString> FilesNonEngine;
	TSet<FString> FoldersAll;
	TSet<FString> FoldersEmpty;
	TSet<FString> FoldersIgnored;
	TArray<FAssetData> AssetsAll;
	TArray<FAssetData> AssetsUsed;
	TArray<FAssetData> AssetsUnused;
	TArray<FAssetData> AssetsExcluded;
	TArray<FAssetData> AssetsPrimary;
	TArray<FAssetData> AssetsWithExternalRefs;
	TArray<FAssetData> AssetsBlackListed;
	TArray<FProjectCleanerIndirectAsset> AssetsIndirect;
	TWeakObjectPtr<UProjectCleanerScanSettings> ScanSettings;
};
