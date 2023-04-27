// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "PjcTypes.h"
#include "PjcDelegates.h"
#include "PjcSubsystem.generated.h"

UCLASS(Config=EditorPerProjectUserSettings, meta=(ToolTip="ProjectCleanerSubsystem"))
class UPjcSubsystem final : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category="ProjectCleaner")
	void ScanProject(const FPjcAssetExcludeSettings& InAssetExcludeSettings);
	
	UFUNCTION(BlueprintCallable, Category="ProjectCleaner")
	const TArray<FAssetData>& GetAssetsAll() const;

	UFUNCTION(BlueprintCallable, Category="ProjectCleaner")
	const TArray<FAssetData>& GetAssetsUsed() const;

	UFUNCTION(BlueprintCallable, Category="ProjectCleaner")
	const TArray<FAssetData>& GetAssetsUnused() const;

	UFUNCTION(BlueprintCallable, Category="ProjectCleaner")
	const TArray<FAssetData>& GetAssetsEditor() const;

	UFUNCTION(BlueprintCallable, Category="ProjectCleaner")
	const TArray<FAssetData>& GetAssetsPrimary() const;

	UFUNCTION(BlueprintCallable, Category="ProjectCleaner")
	const TArray<FAssetData>& GetAssetsExcluded() const;

	UFUNCTION(BlueprintCallable, Category="ProjectCleaner")
	const TArray<FAssetData>& GetAssetsIndirect() const;

	UFUNCTION(BlueprintCallable, Category="ProjectCleaner")
	const TArray<FAssetData>& GetAssetsExtReferenced() const;

	UFUNCTION(BlueprintCallable, Category="ProjectCleaner")
	const TSet<FString>& GetFilesExternal() const;

	UFUNCTION(BlueprintCallable, Category="ProjectCleaner")
	const TSet<FString>& GetFilesCorrupted() const;

	UFUNCTION(BlueprintCallable, Category="ProjectCleaner")
	const TSet<FString>& GetFoldersEmpty() const;

	void ScanProjectAssets();
	void ScanProjectAssets(const FPjcAssetExcludeSettings& InAssetExcludeSettings);
	void ScanProjectFilesAndFolders();

	FPjcDelegateOnScanAssets& OnScanAssets();
	FPjcDelegateOnScanFiles& OnScanFiles();

protected:
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	bool CanScanProject(FString& ErrMsg) const;

	bool bScanningInProgress = false;
	bool bCleaningInProgress = false;

	TArray<FAssetData> AssetsAll;
	TArray<FAssetData> AssetsUsed;
	TArray<FAssetData> AssetsUnused;
	TArray<FAssetData> AssetsEditor;
	TArray<FAssetData> AssetsPrimary;
	TArray<FAssetData> AssetsExcluded;
	TArray<FAssetData> AssetsIndirect;
	TArray<FAssetData> AssetsExtReferenced;
	TMap<FAssetData, FPjcAssetIndirectUsageInfo> AssetsIndirectInfo;
	TSet<FString> FilesExternal;
	TSet<FString> FilesCorrupted;
	TSet<FString> FoldersEmpty;

	FPjcDelegateOnScanAssets DelegateOnScanAssets;
	FPjcDelegateOnScanFiles DelegateOnScanFiles;
};
