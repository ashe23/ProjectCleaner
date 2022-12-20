// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectCleanerTypes.h"
#include "ProjectCleanerDelegates.h"
#include "ProjectCleanerSubsystem.generated.h"

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

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(ToolTip="Returns all dependency assets for given assets"))
	void GetAssetsDependencies(const TArray<FAssetData>& Assets, TArray<FAssetData>& Dependencies) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(ToolTip="Returns all referencer assets for given assets"))
	void GetAssetsReferencers(const TArray<FAssetData>& Assets, TArray<FAssetData>& Referencers) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(Tooltip="Returns total size of given assets"))
	int64 GetAssetsTotalSize(const TArray<FAssetData>& Assets) const;

	UFUNCTION(BlueprintCallable, Category="ProjectCleaner", meta=(Tooltip="Returns total size of given files"))
	static int64 GetFilesTotalSize(const TArray<FString>& Files);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(ToolTip="Returns asset class name"))
	FString GetAssetClassName(const FAssetData& AssetData) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(ToolTip="Returns asset class including blueprint assets."))
	UClass* GetAssetClass(const FAssetData& AssetData) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(ToolTip="Returns normalized path"))
	FString PathNormalize(const FString& InPath) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(ToolTip="Returns absolute path. /Game/MyFolder => {ContentDir}/MyFolder"))
	FString PathConvertToAbs(const FString& InPath) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(ToolTip="Returns relative path. {ContentDir/MyFolder} => /Game/MyFolder"))
	FString PathConvertToRel(const FString& InPath) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(ToolTip="Checks folder is empty or not"))
	static bool FolderIsEmpty(const FString& InFolderPath);

	void ProjectScan();
	void ProjectScan(const FProjectCleanerScanSettings& InScanSettings);
	const FProjectCleanerScanData& GetScanData() const;
	bool CanScanProject() const;
	bool AssetIsExcluded(const FAssetData& AssetData) const;
	bool AssetRegistryWorking() const;
	bool ScanningInProgress() const;
	bool CleaningInProgress() const;
	static bool EditorInPlayMode();
	bool FolderIsExcluded(const FString& InFolderPath) const;
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
	void ScanDataReset();
	void FixupRedirectors() const;
	bool AssetExcludedByPath(const FAssetData& AssetData) const;
	bool AssetExcludedByClass(const FAssetData& AssetData) const;
	bool AssetExcludedByObject(const FAssetData& AssetData) const;
	static bool FileHasEngineExtension(const FString& InFilePath);
	bool FileIsCorrupted(const FString& InFilePathAbs) const;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category="ProjectCleaner")
	bool bAutoCleanEmptyFolders = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category="ProjectCleaner")
	bool bShowRealtimeThumbnails = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category="ProjectCleaner")
	bool bShowTreeViewLines = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category="ProjectCleaner")
	bool bShowFoldersEmpty = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category="ProjectCleaner")
	bool bShowFoldersExcluded = true;

private:
	bool bScanningProject = false;
	bool bCleaningProject = false;

	FProjectCleanerScanData ScanData;
	FProjectCleanerScanSettings ScanSettings;
	
	FProjectCleanerDelegateProjectScanned DelegateProjectScanned;

	IPlatformFile* PlatformFile;
	FAssetRegistryModule* ModuleAssetRegistry;
	FAssetToolsModule* ModuleAssetTools;
};
