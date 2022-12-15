// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectCleanerTypes.h"
#include "ProjectCleanerSubsystem.generated.h"

class FAssetRegistryModule;
class FAssetToolsModule;

DECLARE_MULTICAST_DELEGATE(FProjectCleanerDelegateProjectScanned)

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

	UFUNCTION(BlueprintCallable, Category="ProjectCleaner", meta=(Tooltip="Returns total size of given assets"))
	int64 GetAssetsTotalSize(const TArray<FAssetData>& Assets) const;

	UFUNCTION(BlueprintCallable, Category="ProjectCleaner", meta=(Tooltip="Returns total size of given files"))
	int64 GetFilesTotalSize(const TSet<FString>& Files) const;

	void GetLinkedAssets(const TArray<FAssetData>& Assets, TArray<FAssetData>& LinkedAssets);

	bool FileContainsIndirectAssets(const FString& FileContent);
	bool FileHasEngineExtension(const FString& Extension);
	bool FileIsCorrupted(const FString& InFilePathAbs);
	bool FolderIsExcluded(const FString& InPath);
	bool FolderIsEmpty(const FString& InPath);

	FString PathNormalize(const FString& InPath) const;
	FString PathConvertToAbs(const FString& InPath) const;
	FString PathConvertToRel(const FString& InPath) const;
	FString GetAssetClassName(const FAssetData& AssetData) const;

	UClass* GetAssetClass(const FAssetData& AssetData) const;

	const TArray<FAssetData>& GetAssetsAll() const;
	const TArray<FAssetData>& GetAssetsIndirect() const;
	const TArray<FAssetData>& GetAssetsExcluded() const;
	const TArray<FAssetData>& GetAssetsUnused() const;

	const TSet<FString>& GetFoldersTotal() const;
	const TSet<FString>& GetFoldersEmpty() const;
	const TSet<FString>& GetFilesCorrupted() const;
	const TSet<FString>& GetFilesNonEngine() const;

	bool AssetRegistryWorking() const;
	bool EditorInPlayMode() const;
	bool ScanningProject() const;
	bool CleaningProject() const;
	bool AssetExcluded(const FAssetData& AssetData) const;

	void ProjectScan();

	FProjectCleanerDelegateProjectScanned& OnProjectScanned();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category="ProjectCleaner")
	bool bAutoCleanEmptyFolders = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category="ProjectCleaner", DisplayName="Scan Developers Folder")
	bool bScanFolderDevelopers = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category="ProjectCleaner", DisplayName="Scan Collections Folder")
	bool bScanFolderCollections = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category="ProjectCleaner")
	bool bShowTreeViewLines = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category="ProjectCleaner")
	bool bShowFoldersEmpty = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category="ProjectCleaner")
	bool bShowFoldersExcluded = true;

private:
	bool CanScanProject() const;
	bool AssetExcludedByPath(const FAssetData& AssetData) const;
	bool AssetExcludedByClass(const FAssetData& AssetData) const;
	bool AssetExcludedByObject(const FAssetData& AssetData) const;
	
	void FindAssetsAll();
	void FindAssetsIndirect();
	void FindAssetsExcluded();
	void FindAssetsUsed();
	void FindAssetsUnused();
	void FindAssetsForbidden();
	void FindAssetsPrimary();
	void FindAssetsWithExternalRefs();
	void FindFoldersTotal();
	void FindFoldersEmpty();
	void FindFoldersForbidden();
	void FindFilesCorrupted();
	void FindFilesNonEngine();
	void ResetData();
	void FixupRedirectors();

private:
	TArray<FAssetData> AssetsAll;
	TArray<FAssetData> AssetsIndirect;
	TArray<FAssetData> AssetsExcluded;
	TArray<FAssetData> AssetsUsed;
	TArray<FAssetData> AssetsUnused;
	TArray<FAssetData> AssetsForbidden;
	TArray<FAssetData> AssetsPrimary;
	TArray<FAssetData> AssetsWithExternalRefs;
	TArray<FProjectCleanerIndirectAsset> AssetsIndirectInfos;
	TSet<FString> FoldersTotal;
	TSet<FString> FoldersEmpty;
	TSet<FString> FoldersForbidden;
	TSet<FString> FilesCorrupted;
	TSet<FString> FilesNonEngine;

	bool bScanningProject = false;
	bool bCleaningProject = false;

	IPlatformFile* PlatformFile;
	FAssetRegistryModule* ModuleAssetRegistry;
	FAssetToolsModule* ModuleAssetTools;

	FProjectCleanerDelegateProjectScanned DelegateProjectScanned;
};
