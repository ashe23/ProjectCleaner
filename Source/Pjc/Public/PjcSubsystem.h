// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "PjcDelegates.h"
#include "PjcTypes.h"
#include "PjcSubsystem.generated.h"

class UPjcSettings;
class IAssetRegistry;
class FAssetToolsModule;

UCLASS(Config=EditorPerProjectUserSettings, meta=(ToolTip="ProjectCleanerSubsystem"))
class UPjcSubsystem final : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

protected:
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:
	UPROPERTY(BlueprintReadOnly, Config, Category="ProjectCleaner")
	bool bShowPathsEmpty = true;

	UPROPERTY(BlueprintReadOnly, Config, Category="ProjectCleaner")
	bool bShowPathsExcluded = true;

	UPROPERTY(BlueprintReadOnly, Config, Category="ProjectCleaner")
	bool bShowPathsEngineGenerated = true;

	UPROPERTY(BlueprintReadOnly, Config, Category="ProjectCleaner")
	bool bShowPathsUnusedOnly = false;

	static FString PathNormalize(const FString& InPath);
	static FString PathConvertToAbs(const FString& InPath);
	static FString PathConvertToRel(const FString& InPath);
	static FName PathConvertToObjectPath(const FString& InPath);
	static FName GetAssetClassName(const FAssetData& InAssetData);

	static void GetFoldersInPath(const FString& SearchPath, const bool bSearchRecursive, TSet<FString>& OutFolders);
	static void GetFilesInPath(const FString& SearchPath, const bool bSearchRecursive, TSet<FString>& OutFiles);
	static void GetFilesInPathByExt(const FString& SearchPath, const bool bSearchRecursive, const bool bSearchInvert, const TSet<FString>& Extensions, TSet<FString>& OutFiles);
	static void GetAssetsByPackagePaths(const TArray<FName>& InPackagePaths, const bool bRecursive, TArray<FAssetData>& OutAssets);
	static void GetAssetsByClassNames(const TArray<FName>& InClassNames, TArray<FAssetData>& OutAssets);
	static void GetAssetsByObjectPaths(const TArray<FName>& InObjectPaths, TArray<FAssetData>& OutAssets);
	static void GetAssetDeps(const FAssetData& InAssetData, TArray<FAssetData>& OutAssets);
	static void GetAssetsDeps(const TSet<FAssetData>& InAssets, TSet<FAssetData>& OutAssetsDeps);
	// static void GetAssetsIndirect(TArray<FPjcAssetIndirectUsageInfo>& AssetsIndirect);
	static void FilterAssetsByClassNames(const TSet<FName>& InClassNames, const TArray<FAssetData>& InAssets, const bool bInvertedFilter, TArray<FAssetData>& OutAssets);
	static void FilterAssetsByPackagePaths(const TArray<FName>& InPackagePaths, const TArray<FAssetData>& InAssets, const bool bInvertedFilter, TArray<FAssetData>& OutAssets);

	// static bool EditorIsInPlayMode();
	// static bool AssetRegistryIsWorking();
	static bool AssetIsBlueprint(const FAssetData& InAssetData);
	// static bool AssetIsExcluded(const FPjcExcludeSettings& ExcludeSettings, const FAssetData& InAssetData);
	// static bool AssetHasExtRefs(const FAssetData& InAssetData);
	static bool PathIsEmpty(const FString& InPath);
	// static bool PathIsExcluded(const FString& InPath);
	static bool PathIsEngineGenerated(const FString& InPath);


	static void GetClassNamesPrimary(TSet<FName>& OutClassNames);
	static void GetClassNamesEditor(TSet<FName>& OutClassNames);
	static void GetClassNamesUsed(TSet<FName>& OutClassNames);


	static int64 GetFileSize(const FString& InFilePath);
	static int64 GetFilesSize(const TArray<FString>& InFilePaths);
	static int64 GetAssetSize(const FAssetData& InAssetData);
	static int64 GetAssetsSize(const TArray<FAssetData>& InAssetsDatas);

	static int32 DeleteFolders(const TArray<FString>& InFolderPaths, const bool bRecursive);
	static int32 DeleteFiles(const TArray<FString>& InFilePaths);

	static FAssetData GetAssetByObjectPath(const FName& InObjectPath);

	void ProjectScan();
	void ProjectClean() const;

	UFUNCTION(BlueprintCallable, Category="Pjc")
	void Test(const FName& Path);


	int32 GetNumAssetsAll() const;
	int32 GetNumAssetsUsed() const;
	// int32 GetNumAssetsUsedDeps() const;
	int32 GetNumAssetsExcluded() const;
	int32 GetNumAssetsPrimary() const;
	int32 GetNumAssetsIndirect() const;
	int32 GetNumAssetsEditor() const;
	int32 GetNumAssetsExtReferenced() const;
	int32 GetNumAssetsUnused() const;
	// int32 GetNumFilesAll() const;
	int32 GetNumFilesNonEngine() const;
	int32 GetNumFilesNonCorrupted() const;
	// int32 GetNumFolderAll() const;
	int32 GetNumFolderEmpty() const;

	int64 GetSizeAssetsAll() const;
	int64 GetSizeAssetsUsed() const;
	// int64 GetSizeAssetsUsedDeps() const;
	int64 GetSizeAssetsExcluded() const;
	int64 GetSizeAssetsPrimary() const;
	int64 GetSizeAssetsIndirect() const;
	int64 GetSizeAssetsEditor() const;
	int64 GetSizeAssetsUnused() const;
	int64 GetSizeAssetsExtReferenced() const;
	// int64 GetSizeFilesAll() const;
	int64 GetSizeFilesNonEngine() const;
	int64 GetSizeFilesCorrupted() const;

	EPjcScannerState GetScannerState() const;

	FPjcDelegateOnProjectScan& OnProjectScan();

private:
	bool CanScanProject(FString& ErrMsg) const;
	bool ProjectContainsRedirectors() const;
	void FixupRedirectors() const;

	void FindAssetsIndirect();
	void FindAssetsPrimary();

	// data containers
	TArray<FAssetData> AssetsAll;
	TSet<FAssetData> AssetsUsed;
	TSet<FAssetData> AssetsExcluded;
	TSet<FAssetData> AssetsPrimary;
	TSet<FAssetData> AssetsEditor;
	TSet<FAssetData> AssetsExtReferenced;
	TSet<FAssetData> AssetsUnused;
	TMap<FAssetData, TArray<FPjcAssetUsageInfo>> AssetsIndirect;
	TSet<FString> FilesNonEngine;
	TSet<FString> FilesCorrupted;
	TSet<FString> FoldersEmpty;
	TSet<FString> EngineFileExtensions;

	int64 SizeAssetsAll;
	int64 SizeAssetsUsed;
	int64 SizeAssetsUnused;
	int64 SizeAssetsEditor;
	int64 SizeAssetsPrimary;
	int64 SizeAssetsIndirect;
	int64 SizeAssetsExcluded;
	int64 SizeAssetsExtReferenced;

	EPjcScannerState ScannerState = EPjcScannerState::Idle;

	FPjcDelegateOnProjectScan DelegateOnProjectScan;

	IAssetRegistry* ModuleAssetRegistry = nullptr;
	const FAssetToolsModule* ModuleAssetTools = nullptr;
};
