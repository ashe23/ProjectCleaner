// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectCleanerTypes.h"
#include "ProjectCleanerDelegates.h"
#include "AssetRegistry/AssetRegistryModule.h"

class FAssetRegistryModule;
class UProjectCleanerScanSettings;
struct FProjectCleanerIndirectAsset;

/**
 * @brief Responsible for scanning Content folder of project based on specified settings.  All blueprint exposed functions should get data from here
 */
struct FProjectCleanerScanner
{
	explicit FProjectCleanerScanner(const TWeakObjectPtr<UProjectCleanerScanSettings>& InScanSettings);
	
	void Scan();
	void CleanProject();
	void DeleteEmptyFolders();
	
	void GetSubFolders(const FString& InFolderPathAbs, TSet<FString>& SubFolders) const;
	bool IsFolderEmpty(const FString& InFolderPathAbs) const;
	bool IsFolderExcluded(const FString& InFolderPathAbs) const;

	int32 GetFoldersTotalNum(const FString& InFolderPathAbs) const;
	int32 GetFoldersEmptyNum(const FString& InFolderPathAbs) const;
	int32 GetAssetTotalNum(const FString& InFolderPathAbs) const;
	int32 GetAssetUnusedNum(const FString& InFolderPathAbs) const;
	int64 GetSizeTotal(const FString& InFolderPathAbs) const;
	int64 GetSizeUnused(const FString& InFolderPathAbs) const;

	const TSet<FString>& GetFoldersEmpty() const;
	const TSet<FString>& GetFoldersBlacklist() const;
	const TSet<FString>& GetFilesCorrupted() const;
	const TSet<FString>& GetFilesNonEngine() const;
	const TArray<FAssetData>& GetAssetsUnused() const;
	const TArray<FAssetData>& GetAssetsIndirect() const;
	const TArray<FAssetData>& GetAssetsExcluded() const;
	const TArray<FProjectCleanerIndirectAsset>& GetAssetsIndirectAdvanced() const;

	FProjectCleanerDelegateScanFinished& OnScanFinished();
private:
	void ScannerInit();
	void DataInit();
	void DataReset();

	static int32 GetNumFor(const FString& InFolderPathAbs, const TArray<FAssetData>& Assets);
	static int64 GetSizeFor(const FString& InFolderPathAbs, const TArray<FAssetData>& Assets);

	TSet<FString> FoldersEmpty;
	TSet<FString> FoldersBlacklist;

	TSet<FString> FilesCorrupted;
	TSet<FString> FilesNonEngine;

	TArray<FAssetData> AssetsAll;
	TArray<FAssetData> AssetsPrimary;
	TArray<FAssetData> AssetsIndirect;
	TArray<FAssetData> AssetsExcluded;
	TArray<FProjectCleanerIndirectAsset> AssetsIndirectAdvanced;
	TArray<FAssetData> AssetsWithExternalRefs;
	TArray<FAssetData> AssetsBlacklist;
	TArray<FAssetData> AssetsUnused;

	TWeakObjectPtr<UProjectCleanerScanSettings> ScanSettings;
	FAssetRegistryModule& ModuleAssetRegistry;

	FProjectCleanerDelegateScanFinished DelegateScanFinished;
	FProjectCleanerDelegateCleanFinished DelegateCleanFinished;
	FProjectCleanerDelegateEmptyFoldersDeleted DelegateEmptyFoldersDeleted;
};
