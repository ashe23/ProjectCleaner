// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectCleanerTypes.h"

class FAssetRegistryModule;
class FAssetToolsModule;

struct FProjectCleanerScanSettings
{
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
};

struct FProjectCleanerScanner
{
	explicit FProjectCleanerScanner(const EProjectCleanerScanMethod InScanMethod);

	void Scan(const FProjectCleanerScanSettings& ScanSettings);
	void GetScanResult(FProjectCleanerScanResult& ScanResult);

protected:
	void FindForbiddenFolders();
	void FindForbiddenAssets();

	void RunPreScanActions();
	void RunPostScanActions();

private:
	// private data
	TArray<FAssetData> AssetsForbidden;

	TSet<FString> FoldersForbidden;

	EProjectCleanerScanMethod ScanMethod;
	EProjectCleanerScanState ScanState;

	// Engine Modules
	FAssetRegistryModule& ModuleAssetRegistry;
	FAssetToolsModule& ModuleAssetTools;
	IPlatformFile& PlatformFile;
};
