// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UProjectCleanerScanSettings;
struct FProjectCleanerIndirectAsset;
class FAssetRegistryModule;

// Responsible for scanning Content folder of project based on specified settings, all blueprint exposed functions should get data from here
struct FProjectCleanerScanner
{
	FProjectCleanerScanner();
	
	void Scan(const TWeakObjectPtr<UProjectCleanerScanSettings>& InScanSettings);
	void Reset();

	// void GetAssetsInPath(const FString& InDirPathAbs, TArray<FAssetData>& Assets);
private:
	void FindBlacklistAssets();
	void FindPrimaryAssets();
	void FindExcludedAssets();
	void FindIndirectAssets();
	void FindWithExternalRefsAssets();
	
	// All folders inside Content folder minus Forbidden folders (Engine Specific folders like Collections, WorldPartition folders that we must not scan)
	TSet<FString> FoldersAll;
	TSet<FString> FoldersEmpty;
	TSet<FString> FoldersForbiddenToScan;
	TSet<FString> FoldersForbiddenToDelete;

	TSet<FString> FilesCorrupted;
	TSet<FString> FilesNonEngine;

	TArray<FAssetData> AssetsAll;
	TArray<FAssetData> AssetsUsed;
	TArray<FAssetData> AssetsUnused;
	TArray<FAssetData> AssetsPrimary;
	TArray<FAssetData> AssetsExcluded;
	TArray<FAssetData> AssetsIndirect;
	TArray<FProjectCleanerIndirectAsset> AssetsIndirectInfo;
	TArray<FAssetData> AssetsWithExternalRefs;
	// blacklist assets are considered used (like editor utility assets)
	TArray<FAssetData> AssetsBlacklist;
	
	bool bSilentMode = false;
	TWeakObjectPtr<UProjectCleanerScanSettings> ScanSettings;
	FAssetRegistryModule* ModuleAssetRegistry = nullptr;
};
