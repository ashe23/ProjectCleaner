// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectCleanerTypes.h"
#include "ProjectCleanerSubsystem.generated.h"

class FAssetRegistryModule;
class FAssetToolsModule;

DECLARE_MULTICAST_DELEGATE(FProjectCleanerDelegateProjectScanned);

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

	// Blueprint exposed functions

	UFUNCTION(BlueprintCallable, Category="ProjectCleaner", meta=(Tooltip="Returns scan result as string"))
	static FString ScanResultToString(const EProjectCleanerScanResult ScanResult);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(ToolTip="Returns all primary assets in project"))
	void GetAssetsPrimary(TArray<FAssetData>& AssetsPrimary) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(ToolTip="Returns all indirect assets in project"))
	void GetAssetsIndirect(TArray<FAssetData>& AssetsIndirect) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(ToolTip="Returns all used assets in project"))
	void GetAssetsUsed(TArray<FAssetData>& AssetsUsed) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(ToolTip="Returns all unused assets in project"))
	void GetAssetsUnused(TArray<FAssetData>& AssetsUnused) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(ToolTip="Returns all indirect assets infos"))
	void GetAssetsIndirectInfo(TArray<FProjectCleanerIndirectAssetInfo>& Infos) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(ToolTip="Returns all dependency assets for given assets"))
	void GetAssetsDependencies(const TArray<FAssetData>& Assets, TArray<FAssetData>& Dependencies) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(ToolTip="Returns all assets that have external referencers outside Content folder"))
	void GetAssetsWithExternalRefs(TArray<FAssetData>& Assets) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(ToolTip="Returns all corrupted files inside Content folder"))
	void GetFilesCorrupted(TArray<FString>& FilesCorrupted) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(ToolTip="Returns all non engine files inside Content folder"))
	void GetFilesNonEngine(TArray<FString>& FilesNonEngine) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(Tooltip="Returns total size of given assets"))
	int64 GetAssetsTotalSize(const TArray<FAssetData>& Assets) const;

	UFUNCTION(BlueprintCallable, Category="ProjectCleaner", meta=(Tooltip="Returns total size of given files"))
	int64 GetFilesTotalSize(const TSet<FString>& Files) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(ToolTip="Returns asset class name"))
	FString GetAssetClassName(const FAssetData& AssetData) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(ToolTip="Returns normalized path"))
	FString PathNormalize(const FString& InPath) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(ToolTip="Returns absolute path. /Game/MyFolder => {ContentDir}/MyFolder"))
	FString PathConvertToAbs(const FString& InPath) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(ToolTip="Returns relative path. {ContentDir/MyFolder} => /Game/MyFolder"))
	FString PathConvertToRel(const FString& InPath) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(ToolTip="Returns asset class including blueprint assets."))
	UClass* GetAssetClass(const FAssetData& AssetData) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(ToolTip="Check if given file has engine file extension."))
	bool FileHasEngineExtension(const FString& InFilePath) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(ToolTip="Checks if file is corrupted. Check in AssetRegistry."))
	bool FileIsCorrupted(const FString& InFilePathAbs) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(ToolTip="Checks if folder is empty or not"))
	bool FolderIsEmpty(const FString& InFolderPath) const;

	// C++ only functions
	FProjectCleanerScanData ProjectScan(const FProjectCleanerScanSettings& ScanSettings);
	void GetAssetsByPath(const FString& InFolderPathRel, const bool bRecursive, TArray<FAssetData>& Assets) const;
	int32 GetAssetsByPathNum(const FString& InFolderPathRel, const bool bRecursive) const;
	int64 GetAssetsByPathSize(const FString& InFolderPathRel, const bool bRecursive) const;
	void GetAssetsExcluded(TArray<FAssetData>& AssetsExcluded) const;
	bool AssetIsExcluded(const FAssetData& AssetData) const;
	bool AssetRegistryWorking() const;
	static bool EditorInPlayMode();

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
	bool AssetExcludedByPath(const FAssetData& AssetData) const;
	bool AssetExcludedByClass(const FAssetData& AssetData) const;
	bool AssetExcludedByObject(const FAssetData& AssetData) const;
	void FixupRedirectors() const;

	bool bScanningProject = false;
	bool bCleaningProject = false;

	// FProjectCleanerScanData ScanData;

	IPlatformFile* PlatformFile;
	FAssetRegistryModule* ModuleAssetRegistry;
	FAssetToolsModule* ModuleAssetTools;
};
