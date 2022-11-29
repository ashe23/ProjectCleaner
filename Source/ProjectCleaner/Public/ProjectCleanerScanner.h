// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UProjectCleanerScanSettings;
struct FProjectCleanerIndirectAsset;

struct FProjectCleanerScanner
{
	void Scan(const TWeakObjectPtr<UProjectCleanerScanSettings>& InScanSettings);
	void Reset();
	const TSet<FString>& GetFoldersAll() const;
	const TSet<FString>& GetFoldersEmpty() const;
	const TSet<FString>& GetFoldersForbiddenToDelete() const;
	const TSet<FString>& GetFoldersForbiddenToScan() const;
	const TArray<FAssetData>& GetAssetsAll() const;
	const TArray<FAssetData>& GetAssetsUnused() const;
private:
	void FindAssetsPrimary();
	void FindAssetsIndirect();
	void FindAssetsWithExternalRefs();
	void FindAssetsBlackListed();
	void FindAssetsExcluded();
	void FindAssetsUsed();
	void FindAssetsUnused();
	bool HasIndirectAsset(const FString& FileContent) const;

	// all folder in content folder minus forbidden folders
	TSet<FString> FoldersAll;
	TSet<FString> FoldersEmpty;
	TSet<FString> FoldersForbiddenToDelete;
	TSet<FString> FoldersForbiddenToScan;
	
	TSet<FString> FilesAll;
	TSet<FString> FilesCorrupted;
	TSet<FString> FilesNonEngine;
	
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
