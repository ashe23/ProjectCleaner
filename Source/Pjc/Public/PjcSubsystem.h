// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "PjcDelegates.h"
#include "PjcTypes.h"
#include "PjcSubsystem.generated.h"

UCLASS(Config=EditorPerProjectUserSettings, meta=(ToolTip="ProjectCleanerSubsystem"))
class UPjcSubsystem final : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category="ProjectCleaner")
	void ProjectScan(const FPjcAssetExcludeSettings& InAssetExcludeSetting, FPjcScanResult& ScanResult) const;

	void ScanAssets(const FPjcAssetExcludeSettings& InAssetExcludeSetting, FPjcScanDataAssets& ScanDataAssets) const;
	void ScanFiles(FPjcScanDataFiles& ScanDataFiles) const;

	UFUNCTION(BlueprintCallable, Category="ProjectCleaner")
	void Test();

	// void ProjectScan();
	// void ProjectClean();
	//
	// UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(AdvancedDisplay="OutScanResult"))
	// void ProjectScanBySettings(const FPjcExcludeSettings& InExcludeSettings, UPARAM(DisplayName="OutScanResult") FPjcScanResult& OutScanResult) const;
	//
	// void ScanProjectFiles(const UPjcFileScanSettings& InScanSettings, const bool bShowSlowTask, TSet<FString>& OutFilesExternal, TSet<FString>& OutFilesCorrupted) const;
	//
	// FPjcDelegateOnProjectScan& OnProjectScan();
	// const FPjcScanResult& GetLastScanResult() const;

	// UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="ProjectCleaner", Config)
	// bool bShowFilesExternal = true;
	//
	// UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="ProjectCleaner", Config)
	// bool bShowFilesCorrupted = true;
	//
	// UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="ProjectCleaner", Config)
	// bool bShowFilesExcluded = true;

	FPjcDelegateOnScanAssets& OnScanAssets();

protected:
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	bool CanScanProject(FString& ErrMsg) const;
	// void ScanAssets(const FPjcExcludeSettings& InExcludeSettings, FPjcScanResult& OutScanResult) const;
	// void ScanFiles(FPjcScanResult& OutScanResult) const;
	// void ScanFolders(FPjcScanResult& OutScanResult) const;
	// void ScanStatsUpdate(FPjcScanResult& InScanResult) const;
	//
	// // cached data
	// TSet<FString> FilesExternal;
	// TSet<FString> FilesCorrupted;
	// TSet<FString> FilesExcluded;
	//
	bool bScanningInProgress = false;
	bool bCleaningInProgress = false;

	FPjcDelegateOnScanAssets DelegateOnScanAssets;
	// FPjcScanResult LastScanResult;
	// FPjcDelegateOnProjectScan DelegateOnProjectScan;
};
