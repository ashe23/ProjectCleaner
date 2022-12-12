// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectCleanerTypes.h"

class FAssetRegistryModule;
class FAssetToolsModule;

struct FProjectCleanerScanSettings
{
	// bool bRecursiveScan = false; // todo:ashe23 maybe add this option? 
	FString ScanPath;
	// TArray<FString> ExcludeFolders;
	// TArray<UClass*> ExcludedClasses;
	// TArray<UObject*> ExcludedAssets;
};

struct FProjectCleanerScanResult
{
	TSet<FString> FoldersAll;
	TSet<FString> FoldersEmpty;
	TArray<FAssetData> AssetsAll;
	TArray<FAssetData> AssetsUsed;
	TArray<FAssetData> AssetsExcluded;
	TArray<FAssetData> AssetsUnused;

	void Reset()
	{
		FoldersAll.Reset();
		FoldersEmpty.Reset();
		AssetsAll.Reset();
		AssetsUsed.Reset();
		AssetsExcluded.Reset();
		AssetsUnused.Reset();
	}
};

struct FProjectCleanerScanner
{
	explicit FProjectCleanerScanner(const EProjectCleanerScanMethod InScanMethod);

	void Scan(const FProjectCleanerScanSettings& InScanSettings);
	void GetScanResult(FProjectCleanerScanResult& InScanResult);

protected:
	void FindForbiddenFolders();
	void FindForbiddenAssets();
	void FindAssetsUsed();

	void RunPreScanActions();
	void RunPostScanActions();

private:
	// private data
	TArray<FAssetData> AssetsForbidden;
	TSet<FString> FoldersForbidden;

	EProjectCleanerScanMethod ScanMethod;
	EProjectCleanerScanState ScanState;
	FProjectCleanerScanResult ScanResult;

	// Engine Modules
	FAssetRegistryModule& ModuleAssetRegistry;
	FAssetToolsModule& ModuleAssetTools;
	IPlatformFile& PlatformFile;
};
