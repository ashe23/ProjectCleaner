// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectCleanerTypes.h"

class FAssetRegistryModule;
class UProjectCleanerScanSettings;
struct FProjectCleanerIndirectAsset;

/**
 * @brief Responsible for scanning Content folder of project based on specified settings.  All blueprint exposed functions should get data from here
 */
struct FProjectCleanerScanner
{
	FProjectCleanerScanner();

	void Scan(const TWeakObjectPtr<UProjectCleanerScanSettings>& InScanSettings);

	void GetSubFolders(const FString& InFolderPathAbs, TSet<FString>& SubFolders) const;

	bool IsFolderEmpty(const FString& InFolderPathAbs) const;
	
	int32 GetFoldersTotalNum(const FString& InFolderPathAbs) const;
	int32 GetFoldersEmptyNum(const FString& InFolderPathAbs) const;
	
	const TSet<FString>& GetFoldersBlacklist() const;
	const TSet<FString>& GetFilesCorrupted() const;
	const TSet<FString>& GetFilesNonEngine() const;
	const TArray<FAssetData>& GetAssetsUnused() const;
	const TArray<FAssetData>& GetAssetsIndirect() const;
	const TArray<FProjectCleanerIndirectAsset>& GetAssetsIndirectAdvanced() const;
private:
	void DataInit();
	void DataReset();

	// TSet<FString> FoldersAll;
	TSet<FString> FoldersEmpty;
	TSet<FString> FoldersBlacklist;

	TSet<FString> FilesCorrupted;
	TSet<FString> FilesNonEngine;

	TArray<FAssetData> AssetsAll;
	TArray<FAssetData> AssetsPrimary;
	TArray<FAssetData> AssetsIndirect;
	TArray<FProjectCleanerIndirectAsset> AssetsIndirectAdvanced;
	TArray<FAssetData> AssetsWithExternalRefs;
	TArray<FAssetData> AssetsBlacklist;
	TArray<FAssetData> AssetsUnused;

	TWeakObjectPtr<UProjectCleanerScanSettings> ScanSettings;
	FAssetRegistryModule* ModuleAssetRegistry = nullptr;
};
