// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectCleanerTypes.h"
#include "ProjectCleanerSubsystem.generated.h"

class FAssetRegistryModule;
class FAssetToolsModule;

UCLASS(Config=EditorPerProjectUserSettings)
class UProjectCleanerSubsystem final : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	UProjectCleanerSubsystem();

	UPROPERTY(EditAnywhere, Config)
	bool bAutoCleanEmptyFolders = true;

protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:
	void ProjectScan();
	void CheckEditorState();
	void GetLinkedAssets(const TArray<FAssetData>& Assets, TArray<FAssetData>& LinkedAssets) const;

	FString GetAssetClassName(const FAssetData& AssetData) const;

	const TArray<FAssetData>& GetAssetsAll() const;
	const TArray<FAssetData>& GetAssetsIndirect() const;
	const TArray<FAssetData>& GetAssetsExcluded() const;
	const TArray<FAssetData>& GetAssetsUnused() const;
	int64 GetAssetsTotalSize(const TArray<FAssetData>& Assets) const;

	const TSet<FString>& GetFilesNonEngine() const;
	const TSet<FString>& GetFilesCorrupted() const;

	const TSet<FString>& GetFoldersEmpty() const;

	EProjectCleanerEditorState GetEditorState() const;
	EProjectCleanerScanState GetScanState() const;

private:
	void FixupRedirectors() const;

	void FindAssetsAll();
	void FindAssetsPrimary();
	void FindAssetsExcluded();
	void FindAssetsWithExternalRefs();
	void FindAssetsIndirect();
	void FindAssetsBlacklisted();
	void FindAssetsUsed();
	void FindAssetsUnused();

	void ScanContentFolder();

	void ContainersReset();
	void ContainersShrink();
	void ContainersEmpty();

	EProjectCleanerEditorState EditorState = EProjectCleanerEditorState::Idle;
	EProjectCleanerScanState ScanState = EProjectCleanerScanState::Idle;

	FAssetRegistryModule* ModuleAssetRegistry;
	FAssetToolsModule* ModuleAssetTools;
	IPlatformFile* PlatformFile;

	TArray<FAssetData> AssetsAll;
	TArray<FAssetData> AssetsPrimary;
	TArray<FAssetData> AssetsExcluded;
	TArray<FAssetData> AssetsUsed;
	TArray<FAssetData> AssetsUnused;
	TArray<FAssetData> AssetsIndirect;
	TArray<FAssetData> AssetsBlacklisted;
	TArray<FAssetData> AssetsWithExternalRefs;
	TArray<FProjectCleanerIndirectAsset> IndirectAssetsInfo;

	TSet<FString> FilesNonEngine;
	TSet<FString> FilesCorrupted;

	TSet<FString> FoldersEmpty;
	TSet<FString> FoldersBlacklisted;
};
