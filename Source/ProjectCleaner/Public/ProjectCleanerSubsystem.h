// Copyright Ashot Barkhudaryan. All Rights Reserved.

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

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(ToolTip="Returns all assets in project"))
	void GetAssetsAll(TArray<FAssetData>& Assets) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(ToolTip="Returns all assets with specified classes in project"))
	void GetAssetsByClass(const TArray<FString>& ClassNames, TArray<FAssetData>& Assets) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(ToolTip="Returns assets in given search paths"))
	void GetAssetsByPath(const TArray<FString>& SearchPaths, TArray<FAssetData>& Assets, const bool bRecursive) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(ToolTip="Returns given asset class name"))
	static FString GetAssetClassName(const FAssetData& AssetData);

	// UFUNCTION(BlueprintCallable, Category="ProjectCleaner")
	// FProjectCleanerScanData ProjectScan() const;

	// UFUNCTION(BlueprintCallable, Category="ProjectCleaner")
	// void Test(const FProjectCleanerScanSettings& ScanSettings);

	// UFUNCTION(BlueprintCallable, Category="ProjectCleaner")
	// void ProjectScan(const FProjectCleanerScanSettings& ScanSettings, FProjectCleanerScanData& ScanData);
	// void ProjectScan();
	// void ProjectScan(const FProjectCleanerScanSettings& InScanSettings);
	// void ProjectClean(const bool bRemoveEmptyFolders = true);
	// void ProjectCleanEmptyFolders();
	// const FProjectCleanerScanData& GetScanData() const;
	// bool CanScanProject() const;
	// bool AssetIsExcluded(const FAssetData& AssetData) const;
	// bool ScanningInProgress() const;
	// bool CleaningInProgress() const;
	// FProjectCleanerDelegateProjectScanned& OnProjectScanned();

private:
	// static FString ScanResultToString(const EProjectCleanerScanResult ScanResult);
	static void ScanContentFolder(FProjectCleanerScanData& ScanData);
	// void FindAssetsByScanSettings(const FProjectCleanerScanSettings& ScanSettings, FProjectCleanerScanData& ScanData) const;
	// void FindAssetsPrimary();
	// void FindAssetsIndirect();
	// void FindAssetsExcluded();
	// void FindAssetsUsed();
	// void FindAssetsUnused();
	// void FindFilesCorrupted();
	// void FindFilesNonEngine();
	// void FindFolders();
	// void ScanDataReset();
	// bool AssetExcludedByPath(const FAssetData& AssetData) const;
	// bool AssetExcludedByClass(const FAssetData& AssetData) const;
	// bool AssetExcludedByObject(const FAssetData& AssetData) const;
	// void BucketFill(TArray<FAssetData>& Bucket, const int32 BucketSize);
	// bool BucketPrepare(const TArray<FAssetData>& Bucket, TArray<UObject*>& LoadedAssets) const;
	// int32 BucketDelete(const TArray<UObject*>& LoadedAssets) const;

	bool bScanningInProgress = false;
	bool bCleaningInProgress = false;

	// FProjectCleanerScanData ScanData;
	// FProjectCleanerScanSettings ScanSettings;

	// FProjectCleanerDelegateProjectScanned DelegateProjectScanned;

	IPlatformFile* PlatformFile;
	FAssetRegistryModule* ModuleAssetRegistry;
	FAssetToolsModule* ModuleAssetTools;
};
