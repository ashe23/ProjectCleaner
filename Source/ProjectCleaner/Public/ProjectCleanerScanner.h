// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectCleanerDelegates.h"
#include "AssetRegistry/AssetRegistryModule.h"

class FAssetRegistryModule;
class UProjectCleanerScanSettings;
class UProjectCleanerExcludeSettings;
struct FProjectCleanerIndirectAsset;

/**
 * @brief Responsible for scanning Content folder of project based on specified settings.  All blueprint exposed functions should get data from here
 */
struct FProjectCleanerScanner
{
	explicit FProjectCleanerScanner(UProjectCleanerScanSettings* InScanSettings, UProjectCleanerExcludeSettings* InExcludeSettings);

	void Scan();
	void CleanProject();
	void DeleteEmptyFolders();
	void GetSubFolders(const FString& InFolderPathAbs, TSet<FString>& SubFolders) const;

	EProjectCleanerScannerStatus GetStatus() const;
	
	bool IsFolderEmpty(const FString& InFolderPathAbs) const;
	bool IsFolderExcluded(const FString& InFolderPathAbs) const;

	int64 GetSizeTotal(const FString& InFolderPathAbs) const;
	int64 GetSizeUnused(const FString& InFolderPathAbs) const;
	int32 GetAssetTotalNum(const FString& InFolderPathAbs) const;
	int32 GetAssetUnusedNum(const FString& InFolderPathAbs) const;
	int32 GetFoldersTotalNum(const FString& InFolderPathAbs) const;
	int32 GetFoldersEmptyNum(const FString& InFolderPathAbs) const;

	const TSet<FString>& GetFoldersEmpty() const;
	const TSet<FString>& GetFoldersBlacklist() const;
	const TSet<FString>& GetFilesCorrupted() const;
	const TSet<FString>& GetFilesNonEngine() const;

	const TArray<FAssetData>& GetAssetsAll() const;
	const TArray<FAssetData>& GetAssetsUnused() const;
	const TArray<FAssetData>& GetAssetsIndirect() const;
	const TArray<FAssetData>& GetAssetsExcluded() const;

	const TArray<FProjectCleanerIndirectAsset>& GetAssetsIndirectAdvanced() const;

	FProjectCleanerDelegateScannerStatusChanged& OnStatusChanged();
	FProjectCleanerDelegateScanFinished& OnScanFinished();
	FProjectCleanerDelegateCleanFinished& OnCleanFinished();
	FProjectCleanerDelegateEmptyFoldersDeleted& OnEmptyFoldersDeleted();

private:
	void StatusUpdate(const EProjectCleanerScannerStatus Status);
	// void StateUpdate(const EProjectCleanerScannerState State);||
	// void DateStateUpdate(const EProjectCleanerScannerDataState DataState);
	void DataReset();
	void FindBlacklistedFoldersAndAssets();
	void FindAssetsAll();
	void FindAssetsPrimary();
	void FindAssetsIndirect();
	void FindAssetsExcluded();
	void FindAssetsWithExternalRefs();
	void FindAssetsUsed();
	void FindAssetsUnused();

	void AssetRegistryDelegatesRegister();
	void AssetRegistryDelegatesUnregister() const;

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
	TArray<FAssetData> AssetsUsed;
	TArray<FAssetData> AssetsUnused;

	UProjectCleanerScanSettings* ScanSettings;
	UProjectCleanerExcludeSettings* ExcludeSettings;
	FAssetRegistryModule& ModuleAssetRegistry;
	EProjectCleanerScannerStatus ScannerStatus = EProjectCleanerScannerStatus::NeverScanned;

	FProjectCleanerDelegateScannerStatusChanged DelegateScannerStatusChanged;
	FProjectCleanerDelegateScanFinished DelegateScanFinished;
	FProjectCleanerDelegateCleanFinished DelegateCleanFinished;
	FProjectCleanerDelegateEmptyFoldersDeleted DelegateEmptyFoldersDeleted;

	FDelegateHandle DelegateHandleAssetAdded;
	FDelegateHandle DelegateHandleAssetRemoved;
	FDelegateHandle DelegateHandleAssetRenamed;
	FDelegateHandle DelegateHandleAssetUpdated;
	FDelegateHandle DelegateHandlePathAdded;
	FDelegateHandle DelegateHandlePathRemoved;
};
