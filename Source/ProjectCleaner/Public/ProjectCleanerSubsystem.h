﻿// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectCleanerTypes.h"
#include "ProjectCleanerDelegates.h"
#include "ProjectCleanerSubsystem.generated.h"

struct FProjectCleanerPath;
class FAssetRegistryModule;
class FAssetToolsModule;

UCLASS(Config=EditorPerProjectUserSettings)
class UProjectCleanerSubsystem final : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	UProjectCleanerSubsystem();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	void ProjectScan();
	void ProjectScan(const FProjectCleanerScanSettings& InScanSettings);
	void ProjectClean(const bool bRemoveEmptyFolders = true);
	void ProjectCleanEmptyFolders();
	const FProjectCleanerScanData& GetScanData() const;
	bool CanScanProject() const;
	bool AssetIsExcluded(const FAssetData& AssetData) const;
	bool ScanningInProgress() const;
	bool CleaningInProgress() const;
	FProjectCleanerDelegateProjectScanned& OnProjectScanned();

private:
	static FString ScanResultToString(const EProjectCleanerScanResult ScanResult);
	void FindAssetsTotal();
	void FindAssetsPrimary();
	void FindAssetsIndirect();
	void FindAssetsExcluded();
	void FindAssetsUsed();
	void FindAssetsUnused();
	void FindFilesCorrupted();
	void FindFilesNonEngine();
	void FindFolders();
	void FindFilesAndFolders();
	void ScanDataReset();
	bool AssetExcludedByPath(const FAssetData& AssetData) const;
	bool AssetExcludedByClass(const FAssetData& AssetData) const;
	bool AssetExcludedByObject(const FAssetData& AssetData) const;
	void BucketFill(TArray<FAssetData>& Bucket, const int32 BucketSize);
	bool BucketPrepare(const TArray<FAssetData>& Bucket, TArray<UObject*>& LoadedAssets) const;
	int32 BucketDelete(const TArray<UObject*>& LoadedAssets) const;

	bool bScanningProject = false;
	bool bCleaningProject = false;

	FProjectCleanerScanData ScanData;
	FProjectCleanerScanSettings ScanSettings;

	FProjectCleanerDelegateProjectScanned DelegateProjectScanned;

	IPlatformFile* PlatformFile;
	FAssetRegistryModule* ModuleAssetRegistry;
	FAssetToolsModule* ModuleAssetTools;
};
